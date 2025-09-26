print("ExoCore init starting")
print("MicroPython environment ready")
print("Running NOT library diagnostics...")

results = []


def record(name, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    line = "[{}] {}".format(status, name)
    if detail:
        line += " - {}".format(detail)
    print(line)
    results.append((name, ok, detail))


# consolectl
try:
    import consolectl
    colors = getattr(consolectl, "COLORS", {})
    fg = colors.get("light_cyan") if hasattr(colors, "get") else None
    if fg is None:
        for value in colors.values() if hasattr(colors, "values") else []:
            fg = value
            break
    if fg is None:
        fg = 15
    bg = colors.get("black") if hasattr(colors, "get") else None
    if bg is None:
        bg = 0
    consolectl.set_attr(fg, bg)
    consolectl.write("consolectl OK\n")
    consolectl.scroll(0)
    consolectl.backspace(0)
    consolectl.clear()
    record("consolectl", True, "colors={} fg={} bg={}".format(len(colors) if hasattr(colors, "__len__") else 0, fg, bg))
except Exception as e:
    record("consolectl", False, "error: {}".format(e))


# debugview
try:
    import debugview
    available = debugview.is_available()
    buffer_len = None
    try:
        data = debugview.buffer()
    except MemoryError:
        data = None
        buffer_len = -1
    if buffer_len is None:
        buffer_len = len(data) if data is not None else 0
    debugview.flush()
    debugview.save_file()
    debugview.dump_console()
    detail = "available={}".format(available)
    if buffer_len >= 0:
        detail += " buffer_len={}".format(buffer_len)
    else:
        detail += " buffer_len=OOM"
    record("debugview", True, detail)
except Exception as e:
    record("debugview", False, "error: {}".format(e))


# fsbridge
try:
    import fsbridge
    mounted = fsbridge.is_mounted()
    capacity = fsbridge.capacity()
    payload = b"NOTLIBCHK"
    offset = 0
    if capacity and capacity > len(payload):
        offset = capacity - len(payload)
    written = fsbridge.write(offset, payload)
    data = fsbridge.read(offset, len(payload))
    ok = (written == len(payload)) and (data == payload)
    detail = "mounted={} capacity={} written={} read_len={}".format(mounted, capacity, written, len(data) if data is not None else 0)
    if ok:
        record("fsbridge", True, detail)
    else:
        record("fsbridge", False, detail + " payload_mismatch")
except Exception as e:
    record("fsbridge", False, "error: {}".format(e))


# hwinfo
try:
    import hwinfo
    tsc = hwinfo.rdtsc()
    vendor = hwinfo.cpuid(0)
    port_val = hwinfo.inb(0x60)
    hwinfo.outb(0x80, 0x55)
    detail = "rdtsc=0x{:x} vendor={} inb=0x{:02x}".format(tsc, vendor, port_val & 0xFF)
    record("hwinfo", True, detail)
except Exception as e:
    record("hwinfo", False, "error: {}".format(e))


# keyinput
try:
    import keyinput
    has_char = hasattr(keyinput, "read_char")
    has_code = hasattr(keyinput, "read_code")
    ok = has_char and has_code
    detail = "read_char={} read_code={}".format(has_char, has_code)
    if ok:
        detail += " (input check skipped)"
    record("keyinput", ok, detail)
except Exception as e:
    record("keyinput", False, "error: {}".format(e))


# memstats
try:
    import memstats
    free_before = memstats.heap_free()
    app_id = memstats.register_app(1)
    ok = app_id > 0
    handle = -1
    used = None
    loaded = None
    if ok:
        payload = b"memstats"
        handle = memstats.save_block(app_id, payload)
        used = memstats.app_used(app_id)
        loaded = memstats.load_block(app_id, handle)
        ok = (handle >= 0) and (loaded == payload)
    detail = "app_id={} handle={} used={} free_before={}".format(app_id, handle, used, free_before)
    if loaded is not None:
        detail += " loaded_len={}".format(len(loaded))
    if ok:
        record("memstats", True, detail)
    else:
        record("memstats", False, detail + " payload_mismatch")
except Exception as e:
    record("memstats", False, "error: {}".format(e))


# modrunner
try:
    import modrunner
    rc = modrunner.run("nonexistent-module")
    record("modrunner", True, "run('nonexistent-module') returned {}".format(rc))
except Exception as e:
    record("modrunner", False, "error: {}".format(e))


# runstatectl
try:
    import runstatectl
    prog_before = runstatectl.current_program()
    runstatectl.set_current_program("notlib-test")
    prog_after = runstatectl.current_program()
    user_before = runstatectl.current_user_app()
    runstatectl.set_current_user_app(42)
    user_after = runstatectl.current_user_app()
    debug_before = runstatectl.is_debug_mode()
    runstatectl.set_debug_mode(True)
    debug_after = runstatectl.is_debug_mode()
    vga_before = runstatectl.is_vga_output()
    runstatectl.set_vga_output(True)
    vga_after = runstatectl.is_vga_output()
    ok = (prog_after == "notlib-test") and (user_after == 42) and debug_after and vga_after
    detail = "prog_before={} prog_after={} user_before={} user_after={} debug_before={} debug_after={} vga_before={} vga_after={}".format(
        prog_before, prog_after, user_before, user_after, debug_before, debug_after, vga_before, vga_after)
    record("runstatectl", ok, detail)
except Exception as e:
    record("runstatectl", False, "error: {}".format(e))


# serialctl
try:
    import serialctl
    serialctl.write("serialctl OK\n")
    serialctl.write_hex(0xABCD)
    serialctl.write_dec(1234)
    serialctl.raw_write("RAW")
    record("serialctl", True, "write/hex/dec/raw invoked")
except Exception as e:
    record("serialctl", False, "error: {}".format(e))


# tscript
try:
    import tscript
    tscript.run("2 3 + print 65 char exit")
    record("tscript", True, "stack operations executed")
except Exception as e:
    record("tscript", False, "error: {}".format(e))


# vga
try:
    import vga
    vga.enable(True)
    vga.enable(False)
    record("vga", True, "enable toggled")
except Exception as e:
    record("vga", False, "error: {}".format(e))


# vgademo
try:
    import env as env_module
    from env import env as _env
    import vga as _vga
    try:
        _vga.enable(False)
    except Exception:
        pass
    env_module.mpyrun("vgademo")
    vga_state = None
    if _env is not None:
        try:
            vga_state = _env["vga_enabled"]
        except Exception:
            try:
                vga_state = _env.get("vga_enabled") if hasattr(_env, "get") else None
            except Exception:
                vga_state = None
    ok = bool(vga_state)
    detail = "env_vga_enabled={}".format(vga_state)
    record("vgademo", ok, detail)
except Exception as e:
    record("vgademo", False, "error: {}".format(e))


passed = 0
for item in results:
    if item[1]:
        passed += 1
total = len(results)
print("NOT library diagnostics complete: {}/{} passed".format(passed, total))
if passed != total:
    print("One or more tests failed; check logs above")
else:
    print("All NOT library tests passed successfully")
