print("process module loaded")
import time

__all__ = [
    "ProcessManager",
    "Process",
    "ProcessError",
    "ProcessPermissionError",
    "ProcessStateError",
    "ProcessNotFound",
    "ProcessTerminated",
    "ProcessTimeout",
]

try:
    _monotonic = time.monotonic
except AttributeError:  # pragma: no cover - MicroPython fallback
    _monotonic = time.time


def _now():
    return _monotonic()


class ProcessError(Exception):
    """Base class for all process related errors."""


class ProcessPermissionError(ProcessError):
    """Raised when a process attempts an operation without permission."""


class ProcessStateError(ProcessError):
    """Raised when a process is in an invalid state for the requested action."""


class ProcessNotFound(ProcessError):
    """Raised when attempting to look up an unknown process."""


class ProcessTerminated(ProcessError):
    """Raised when a process is terminated."""


class ProcessTimeout(ProcessTerminated):
    """Raised when a process exceeds its allotted runtime."""


class ProcessState:
    READY = "ready"
    RUNNING = "running"
    TERMINATED = "terminated"


class ProcessManager(object):
    """Create and manage a hierarchy of cooperative kernel level processes."""

    def __init__(self):
        self._processes = {}
        self._pid_counter = 1
        self._root = None

    @property
    def root(self):
        return self._root

    def _next_pid(self):
        pid = self._pid_counter
        self._pid_counter += 1
        return pid

    def _register(self, process):
        self._processes[process.pid] = process

    def _unregister(self, process):
        self._processes.pop(process.pid, None)

    def create_kernel_process(
        self,
        name,
        sandboxed=False,
        ring=0,
        allowed_libraries=None,
        max_runtime=None,
        can_spawn=True,
        max_children=None,
    ):
        """Create the top level kernel process."""
        if self._root is not None:
            raise ProcessError("Kernel process already exists")
        process = Process(
            manager=self,
            name=name,
            parent=None,
            sandboxed=sandboxed,
            ring=ring,
            allowed_libraries=allowed_libraries,
            max_runtime=max_runtime,
            can_spawn=can_spawn,
            max_children=max_children,
        )
        self._root = process
        return process

    def get_process(self, pid):
        process = self._processes.get(pid)
        if process is None:
            raise ProcessNotFound("Process %s not found" % pid)
        return process

    def assert_control(self, controller, target):
        if controller is target:
            return
        if controller is None or target is None:
            raise ProcessPermissionError("Invalid controller/target")
        node = target
        while node is not None:
            if node is controller:
                return
            node = node.parent
        raise ProcessPermissionError("Controller does not own target")

    def iter_processes(self):
        return list(self._processes.values())


class Process(object):
    def __init__(
        self,
        manager,
        name,
        target=None,
        parent=None,
        sandboxed=False,
        ring=0,
        allowed_libraries=None,
        max_runtime=None,
        can_spawn=True,
        max_children=None,
    ):
        if ring not in (0, 3):
            raise ValueError("Ring must be 0 or 3")
        self.manager = manager
        self.pid = manager._next_pid()
        self.name = name
        self.parent = parent
        self.children = {}
        self.state = ProcessState.READY
        self._target = target
        self._start_time = None
        self._deadline = None
        self._termination_reason = None

        self._requested_sandboxed = bool(sandboxed)
        self._requested_allowed_libraries = _normalise_library_set(allowed_libraries)
        self._requested_max_runtime = _normalise_runtime(max_runtime)
        self._requested_can_spawn = bool(can_spawn)
        self._requested_max_children = _normalise_max_children(max_children)
        self._requested_ring = ring
        self._can_kill_parent = False

        self.sandboxed = False
        self.allowed_libraries = None
        self.max_runtime = None
        self.can_spawn = True
        self.max_children = None
        self.ring = ring

        manager._register(self)
        self._refresh_permissions(propagate=False)

    # ------------------------------------------------------------------
    # Permission handling helpers
    # ------------------------------------------------------------------
    def _refresh_permissions(self, propagate=True):
        parent = self.parent
        # sandboxed flag - once sandboxed in ancestry, it sticks
        if parent is None:
            self.sandboxed = bool(self._requested_sandboxed)
        else:
            self.sandboxed = parent.sandboxed or bool(self._requested_sandboxed)

        # allowed libraries propagate as an intersection
        parent_allowed = None if parent is None else parent.allowed_libraries
        requested_allowed = self._requested_allowed_libraries
        if parent_allowed is None:
            if requested_allowed is None:
                self.allowed_libraries = None
            else:
                self.allowed_libraries = set(requested_allowed)
        else:
            if requested_allowed is None:
                self.allowed_libraries = set(parent_allowed)
            else:
                self.allowed_libraries = set(parent_allowed) & set(requested_allowed)

        # runtime limits propagate as minima
        parent_runtime = None if parent is None else parent.max_runtime
        requested_runtime = self._requested_max_runtime
        if parent_runtime is None:
            self.max_runtime = requested_runtime
        else:
            if requested_runtime is None:
                self.max_runtime = parent_runtime
            else:
                self.max_runtime = min(parent_runtime, requested_runtime)

        # spawn permissions
        parent_can_spawn = True if parent is None else parent.can_spawn
        self.can_spawn = parent_can_spawn and bool(self._requested_can_spawn)

        parent_max_children = None if parent is None else parent.max_children
        requested_max_children = self._requested_max_children
        if parent_max_children is None:
            self.max_children = requested_max_children
        else:
            if requested_max_children is None:
                self.max_children = parent_max_children
            else:
                self.max_children = min(parent_max_children, requested_max_children)

        parent_ring = 0 if parent is None else parent.ring
        requested_ring = self._requested_ring
        if requested_ring not in (0, 3):
            requested_ring = 3
        self.ring = requested_ring
        if self.ring < parent_ring:
            self.ring = parent_ring

        if self.state == ProcessState.RUNNING:
            self._update_deadline()

        if propagate:
            for child in self.children.values():
                child._refresh_permissions(True)

    def _update_deadline(self):
        if self.max_runtime is None:
            self._deadline = None
            return
        if self._start_time is None:
            return
        self._deadline = self._start_time + self.max_runtime

    def _ensure_controlled_by(self, controller):
        self.manager.assert_control(controller, self)

    # ------------------------------------------------------------------
    # Public permission management APIs
    # ------------------------------------------------------------------
    def set_allowed_libraries(self, libraries, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        self._requested_allowed_libraries = _normalise_library_set(libraries)
        self._refresh_permissions(propagate=True)

    def add_allowed_library(self, library, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        lib_set = self._requested_allowed_libraries
        if lib_set is None:
            lib_set = set()
        else:
            lib_set = set(lib_set)
        lib_set.add(library)
        self._requested_allowed_libraries = lib_set
        self._refresh_permissions(propagate=True)

    def remove_allowed_library(self, library, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        if self._requested_allowed_libraries is None:
            raise ProcessPermissionError("No explicit library restrictions to remove from")
        lib_set = set(self._requested_allowed_libraries)
        lib_set.discard(library)
        if not lib_set:
            self._requested_allowed_libraries = None
        else:
            self._requested_allowed_libraries = lib_set
        self._refresh_permissions(propagate=True)

    def set_max_runtime(self, seconds, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        self._requested_max_runtime = _normalise_runtime(seconds)
        self._refresh_permissions(propagate=True)

    def set_sandboxed(self, sandboxed, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        self._requested_sandboxed = bool(sandboxed)
        self._refresh_permissions(propagate=True)

    def set_can_spawn(self, can_spawn, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        self._requested_can_spawn = bool(can_spawn)
        self._refresh_permissions(propagate=True)

    def set_max_children(self, max_children, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        self._requested_max_children = _normalise_max_children(max_children)
        self._refresh_permissions(propagate=True)

    def set_ring_level(self, ring, by=None):
        if ring not in (0, 3):
            raise ValueError("Ring must be 0 or 3")
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        self._requested_ring = ring
        self._refresh_permissions(propagate=True)

    def set_can_kill_parent(self, allow, by=None):
        if self.parent is None:
            raise ProcessPermissionError("Root process cannot configure parent kill")
        controller = self if by is None else by
        if controller is self:
            raise ProcessPermissionError("Child processes cannot self-authorise parent termination")
        self._ensure_controlled_by(controller)
        self._can_kill_parent = bool(allow)

    # ------------------------------------------------------------------
    # Lifecycle management
    # ------------------------------------------------------------------
    def spawn_child(
        self,
        name,
        target=None,
        sandboxed=None,
        ring=None,
        allowed_libraries=None,
        max_runtime=None,
        can_spawn=True,
        max_children=None,
    ):
        if not self.can_spawn:
            raise ProcessPermissionError("Spawning new processes is not permitted")
        active_children = [child for child in self.children.values() if child.state != ProcessState.TERMINATED]
        if self.max_children is not None and len(active_children) >= self.max_children:
            raise ProcessPermissionError("Maximum child processes reached")
        requested_sandboxed = self.sandboxed if sandboxed is None else bool(sandboxed)
        requested_ring = self.ring if ring is None else ring
        child = Process(
            manager=self.manager,
            name=name,
            target=target,
            parent=self,
            sandboxed=requested_sandboxed,
            ring=requested_ring,
            allowed_libraries=allowed_libraries,
            max_runtime=max_runtime,
            can_spawn=can_spawn,
            max_children=max_children,
        )
        self.children[child.pid] = child
        child._refresh_permissions(propagate=True)
        return child

    def load_library(self, name):
        if self.allowed_libraries is not None and name not in self.allowed_libraries:
            raise ProcessPermissionError("Library '%s' is not permitted" % name)
        module = __import__(name)
        return module

    def run(self, target=None, *args, **kwargs):
        if self.state == ProcessState.TERMINATED:
            raise ProcessStateError("Process already terminated")
        if target is not None:
            self._target = target
        if self._target is None:
            raise ProcessStateError("No target callable provided")
        self.state = ProcessState.RUNNING
        self._start_time = _now()
        if self.max_runtime is not None:
            self._deadline = self._start_time + self.max_runtime
        else:
            self._deadline = None
        try:
            return self._target(self, *args, **kwargs)
        finally:
            if self.state != ProcessState.TERMINATED:
                self.state = ProcessState.READY
                self._start_time = None
                self._deadline = None

    def terminate(self, reason=None, by=None):
        controller = self if by is None else by
        self._ensure_controlled_by(controller)
        self._terminate(reason)

    def _terminate(self, reason=None):
        if self.state == ProcessState.TERMINATED:
            return
        for child in list(self.children.values()):
            child._terminate(ProcessTerminated("Parent %s terminated" % self.pid))
        self.children.clear()
        if self.parent is not None:
            self.parent.children.pop(self.pid, None)
        else:
            if self.manager.root is self:
                self.manager._root = None
        self.manager._unregister(self)
        self.state = ProcessState.TERMINATED
        if reason is None:
            reason = ProcessTerminated("Process %s terminated" % self.pid)
        self._termination_reason = reason
        self._start_time = None
        self._deadline = None

    def check_runtime(self):
        if self.state != ProcessState.RUNNING:
            return
        if self._deadline is None:
            return
        if _now() > self._deadline:
            self._terminate(ProcessTimeout("Process %s exceeded runtime" % self.pid))
            raise ProcessTimeout("Process %s exceeded runtime" % self.pid)

    def remaining_time(self):
        if self._deadline is None:
            return None
        remaining = self._deadline - _now()
        if remaining < 0:
            return 0
        return remaining

    def kill_child(self, child_pid, by=None):
        child = self.children.get(child_pid)
        if child is None:
            raise ProcessNotFound("Child process %s not found" % child_pid)
        controller = self if by is None else by
        child._ensure_controlled_by(controller)
        child._terminate(ProcessTerminated("Terminated by parent %s" % controller.pid))

    def request_parent_termination(self):
        parent = self.parent
        if parent is None:
            raise ProcessPermissionError("No parent to terminate")
        if not self._can_kill_parent:
            raise ProcessPermissionError("Permission to terminate parent denied")
        parent._promote_child(self)

    def _promote_child(self, child):
        if child.pid not in self.children:
            raise ProcessNotFound("Process %s is not a child of %s" % (child.pid, self.pid))
        grandparent = self.parent
        # Detach promoted child from current parent
        self.children.pop(child.pid)
        child.parent = grandparent
        if grandparent is None:
            self.manager._root = child
        else:
            grandparent.children.pop(self.pid, None)
            grandparent.children[child.pid] = child
        # Adopt siblings under the promoted child
        for sibling in list(self.children.values()):
            self.children.pop(sibling.pid)
            child.children[sibling.pid] = sibling
            sibling.parent = child
        # Refresh restrictions as tree shape changed
        child._refresh_permissions(propagate=True)
        for adopted in child.children.values():
            adopted._refresh_permissions(propagate=True)
        child._can_kill_parent = False
        # Terminate the old parent (which has no children now)
        self._terminate(ProcessTerminated("Parent replaced by child %s" % child.pid))

    # ------------------------------------------------------------------
    # Inspection helpers
    # ------------------------------------------------------------------
    def info(self):
        return {
            "pid": self.pid,
            "name": self.name,
            "state": self.state,
            "sandboxed": self.sandboxed,
            "ring": self.ring,
            "allowed_libraries": None if self.allowed_libraries is None else set(self.allowed_libraries),
            "max_runtime": self.max_runtime,
            "can_spawn": self.can_spawn,
            "max_children": self.max_children,
            "parent": None if self.parent is None else self.parent.pid,
            "children": [child.pid for child in self.children.values()],
        }

    def descendants(self):
        stack = list(self.children.values())
        result = []
        while stack:
            node = stack.pop(0)
            result.append(node)
            for child in node.children.values():
                stack.append(child)
        return result


def _normalise_library_set(libraries):
    if libraries is None:
        return None
    if isinstance(libraries, str):
        return {libraries}
    result = set()
    for item in libraries:
        result.add(item)
    return result


def _normalise_runtime(value):
    if value is None:
        return None
    if value <= 0:
        raise ValueError("Runtime must be positive")
    return float(value)


def _normalise_max_children(value):
    if value is None:
        return None
    if value < 1:
        raise ValueError("max_children must be at least 1")
    return int(value)

