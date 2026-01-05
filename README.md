# FastGraph
FastGraph: A high-performance C++ graph preprocessing library with Python bindings, designed to accelerate graph operations.




## Quick Start

 - Build and install dependencies, and compile the pybind11 extension:

```bash
./installation.sh
```

 - Run the provided example to extract a maximum clique from a `.clq` file:

```bash
source .venv/bin/activate
python scripts/find_max_clique.py test/p_hat1000-2_G46.clq --time 5
```

This uses the `mc_brb_module` extension built from `Cpplibs/MaxClique/mc_brb.cpp`.


