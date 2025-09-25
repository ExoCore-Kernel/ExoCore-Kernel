print("hwinfo module loaded")
from env import env
import hwinfo_native as _native

rdtsc = _native.rdtsc
cpuid = _native.cpuid
inb = _native.inb
outb = _native.outb

env['hwinfo'] = {
    'rdtsc': rdtsc,
    'cpuid': cpuid,
    'inb': inb,
    'outb': outb,
}
