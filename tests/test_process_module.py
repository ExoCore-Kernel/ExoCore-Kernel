import time
import importlib.util
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "mpymod" / "process" / "init.py"
spec = importlib.util.spec_from_file_location("mpy_process", MODULE_PATH)
process_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(process_module)

ProcessManager = process_module.ProcessManager
ProcessPermissionError = process_module.ProcessPermissionError
ProcessTimeout = process_module.ProcessTimeout


def test_basic_spawn_and_permissions():
    manager = ProcessManager()
    root = manager.create_kernel_process("kernel", sandboxed=False, ring=0)
    child = root.spawn_child(
        "user",
        sandboxed=True,
        ring=3,
        allowed_libraries={"math", "time"},
        max_runtime=10,
    )
    assert child.sandboxed is True
    assert child.ring == 3
    assert child.allowed_libraries == {"math", "time"}
    grandchild = child.spawn_child(
        "worker",
        sandboxed=False,
        ring=0,
        allowed_libraries={"math", "os"},
    )
    assert grandchild.sandboxed is True  # inherits parent sandbox
    assert grandchild.ring == 3  # cannot escalate above parent
    assert grandchild.allowed_libraries == {"math"}


def test_allowed_library_updates_propagate():
    manager = ProcessManager()
    root = manager.create_kernel_process("kernel", sandboxed=False)
    root.set_allowed_libraries({"math", "time", "json"})
    child = root.spawn_child(
        "user",
        sandboxed=True,
        ring=3,
        allowed_libraries={"math", "time"},
    )
    module = child.load_library("math")
    assert module.__name__ == "math"
    with pytest.raises(ProcessPermissionError):
        child.load_library("json")
    root.set_allowed_libraries({"time"})
    assert child.allowed_libraries == {"time"}
    child.add_allowed_library("random", by=root)
    # parent's restriction still applies
    assert child.allowed_libraries == {"time"}


def test_runtime_timeout_can_be_tightened():
    manager = ProcessManager()
    root = manager.create_kernel_process("kernel", sandboxed=False)
    child = root.spawn_child("worker", max_runtime=0.5)
    root.set_max_runtime(1.0)
    child.set_max_runtime(0.2, by=root)

    def busy(proc):
        while True:
            time.sleep(0.05)
            proc.check_runtime()

    with pytest.raises(ProcessTimeout):
        child.run(busy)


def test_parent_can_kill_descendant():
    manager = ProcessManager()
    root = manager.create_kernel_process("kernel", sandboxed=False)
    child = root.spawn_child("user")
    grandchild = child.spawn_child("worker")
    assert grandchild in child.children.values()
    grandchild.terminate(by=root)
    assert grandchild.state == process_module.ProcessState.TERMINATED
    assert grandchild.pid not in child.children


def test_child_promotion_when_allowed():
    manager = ProcessManager()
    root = manager.create_kernel_process("kernel", sandboxed=False)
    primary = root.spawn_child("primary", sandboxed=True)
    sibling = root.spawn_child("sibling", sandboxed=True)
    secondary = primary.spawn_child("secondary")
    primary.set_can_kill_parent(True, by=root)
    with pytest.raises(ProcessPermissionError):
        secondary.request_parent_termination()
    primary.request_parent_termination()
    assert root.state == process_module.ProcessState.TERMINATED
    assert manager.root is primary
    assert sibling.parent is primary
    assert secondary.parent is primary
    assert sibling.pid in primary.children
    assert secondary.pid in primary.children

