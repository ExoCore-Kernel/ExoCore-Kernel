print("modrunner module loaded")
from env import env
import modrunner_native as _native

run = _native.run


def run_many(names):
    if not isinstance(names, (list, tuple)):
        raise TypeError('expected list or tuple of module names')
    results = []
    for name in names:
        result = _native.run(str(name))
        results.append(result)
    return results

env['modules'] = {
    'run': run,
    'run_many': run_many,
}
