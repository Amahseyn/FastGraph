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

## Comparison with NetworkX

You can compare the compiled `mc_brb_module` against NetworkX's maximum-clique
implementation using the included script:

```bash
source .venv/bin/activate
python scripts/compare_with_networkx.py test/p_hat1000-2_G46.clq --time 5 --nx-timeout 30
```

The script reports clique sizes and runtimes for both implementations and can
optionally timeout the NetworkX run to avoid very long computations on large graphs.

## Git hooks (pre-push checks)

To ensure both C++ extensions build and pass a quick smoke test before pushing,
this repository includes a pre-push hook in `.githooks/pre-push`. To enable it locally run:

```bash
git config core.hooksPath .githooks
```

The hook builds the pybind11 extensions and runs `scripts/smoke_test.py`. If the
build or test fails the push will be aborted. You can skip the hook by using
`git push --no-verify` when necessary.




