#mpyexo
"""Host wrapper for the ExoDraw UI harness inside mpymod."""

import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
MPYMOD_PATH = os.path.join(ROOT, "mpymod")
if MPYMOD_PATH not in sys.path:
    sys.path.insert(0, MPYMOD_PATH)

from exodraw.ui import main


if __name__ == "__main__":
    main()
