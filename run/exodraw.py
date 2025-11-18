#mpyexo
"""Host wrapper to run the ExoDraw library from the mpymod bundle.

This keeps the development entry point aligned with the MicroPython
module that ships inside the ISO.
"""

import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
MPYMOD_PATH = os.path.join(ROOT, "mpymod")
if MPYMOD_PATH not in sys.path:
    sys.path.insert(0, MPYMOD_PATH)

from exodraw import *  # noqa: F401,F403  # re-export for compatibility
