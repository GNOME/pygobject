# Remove the .git folder from pythoncapi-compat
# Workaround for https://github.com/mesonbuild/meson/issues/11750

import shutil
import os
from pathlib import Path

shutil.rmtree(Path(os.environ["MESON_DIST_ROOT"]) / "subprojects" / "pythoncapi-compat" / ".git")
