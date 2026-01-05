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
BUILTDIR=$(python - <<'PY'
import sys, glob
from pathlib import Path
bd = Path('build')
paths = list(bd.rglob('mc_brb_module*.so'))
print(paths[0] if paths else '')
PY
)
if [ -n "$BUILTDIR" ]; then
	echo "Copying built extension $BUILTDIR to project root"
	cp "$BUILTDIR" "$ROOT_DIR/"
fi

echo
echo "Build complete. To run the example:"
echo "  source $VENV_DIR/bin/activate"
echo "  python scripts/find_max_clique.py test/p_hat1000-2_G46.clq --time 5"
echo
echo "If you need to change the Python binary, set the PYTHON environment variable before running this script."
