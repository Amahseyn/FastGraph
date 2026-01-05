#!/usr/bin/env bash
set -euo pipefail

# Simple installer to build the pybind11 C++ extension and install Python deps
# Usage: ./installation.sh

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
PYTHON=${PYTHON:-python3}
VENV_DIR="$ROOT_DIR/.venv"

echo "Creating virtualenv at $VENV_DIR (using $PYTHON)"
$PYTHON -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"

echo "Upgrading pip and installing Python requirements"
pip install --upgrade pip
if [ -f "$ROOT_DIR/requirements.txt" ]; then
	pip install -r "$ROOT_DIR/requirements.txt"
fi

echo "Building C++ extension (pybind11) in-place"
cd "$ROOT_DIR"
"$PYTHON" setup.py build_ext --inplace

# Copy the built extension shared object to the project root for easy import
## Find built extension shared objects for the modules and copy them to project root
BUILT_SO_FILES=$(python - <<'PY'
import sys
from pathlib import Path
bd = Path('build')
paths = []
if bd.exists():
	paths += [str(p) for p in bd.rglob('mc_brb_module*.so')]
	paths += [str(p) for p in bd.rglob('max_clique_module*.so')]
# also check common lib dirs created by build_ext
bd2 = Path('lib.linux-x86_64-3.10')
if bd2.exists():
	paths += [str(p) for p in bd2.rglob('mc_brb_module*.so')]
	paths += [str(p) for p in bd2.rglob('max_clique_module*.so')]
print('\n'.join(paths))
PY
)

if [ -n "$BUILT_SO_FILES" ]; then
	echo "Found built extension files:" 
	echo "$BUILT_SO_FILES"
	while IFS= read -r so; do
		if [ -n "$so" ]; then
			echo "Copying built extension $so to project root"
			cp "$so" "$ROOT_DIR/"
		fi
	done <<< "$BUILT_SO_FILES"
else
	echo "No built extension .so files found in build directories."
fi

echo
echo "Build complete. To run the example:"
echo "  source $VENV_DIR/bin/activate"
echo "  python scripts/find_max_clique.py test/p_hat1000-2_G46.clq --time 5"
echo
echo "If you need to change the Python binary, set the PYTHON environment variable before running this script."
