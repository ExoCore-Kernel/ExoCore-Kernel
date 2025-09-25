print("tscript module loaded")
from env import env
import tscript_native as _native

run = _native.run


def run_lines(lines):
    if not isinstance(lines, (list, tuple)):
        raise TypeError('expected list or tuple of strings')
    program = '\n'.join(str(line) for line in lines)
    _native.run(program)

env['tscript'] = {
    'run': run,
    'run_lines': run_lines,
}
