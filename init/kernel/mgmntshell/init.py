#mpyexo
"""Management shell profile for ExoCore.

This module is packaged only with the management shell boot option.  It
preloads a curated set of MicroPython helpers and exposes metadata used by
``init/kernel/init.py`` to present a tailored experience.
"""

from env import env, mpyrun

MANAGEMENT_MODULES = (
    "consolectl",
    "modrunner",
    "memstats",
    "runstatectl",
    "serialctl",
    "debugview",
    "hwinfo",
)


def bootstrap():
    loaded = {}
    for name in MANAGEMENT_MODULES:
        try:
            mod = mpyrun(name)
        except Exception as exc:  # pragma: no cover - runtime diagnostics
            print("[mgmnt] failed to load %s: %s" % (name, exc))
            continue
        if mod is None:
            print("[mgmnt] module %s unavailable" % name)
            continue
        loaded[name] = mod
    env['shell_state'] = {
        'loaded': tuple(sorted(loaded.keys())),
        'last_boot': None,
    }
    return loaded


def module_list():
    return MANAGEMENT_MODULES


env['shell'] = {
    'profile': 'management',
    'modules': MANAGEMENT_MODULES,
    'bootstrap': bootstrap,
    'module_list': module_list,
}
