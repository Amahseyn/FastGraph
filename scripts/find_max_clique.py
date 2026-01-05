#!/usr/bin/env python3
"""Read a DIMACS .clq file, call the mc_brb pybind11 module, and print the maximum clique."""
import argparse
from collections import defaultdict
import sys
import os

# Ensure project root is on sys.path so the compiled extension (placed in project root)
# can be imported even when this script is executed from `scripts/` directory.
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

def read_clq(path):
    n = 0
    adj = defaultdict(set)
    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line[0] == 'c':
                continue
            if line[0] == 'p':
                parts = line.split()
                if len(parts) >= 3:
                    try:
                        n = int(parts[2])
                    except ValueError:
                        pass
                continue
            if line[0] == 'e':
                parts = line.split()
                if len(parts) >= 3:
                    u = int(parts[1]) - 1
                    v = int(parts[2]) - 1
                    if u >= 0 and v >= 0:
                        adj[u].add(v)
                        adj[v].add(u)

    if n == 0:
        # infer n from edges
        nodes = set(adj.keys())
        for s in adj.values():
            nodes |= s
        n = max(nodes) + 1 if nodes else 0

    adjacency_list = [set() for _ in range(n)]
    for u, s in adj.items():
        adjacency_list[u] = s
    return n, adjacency_list

def main():
    parser = argparse.ArgumentParser(description="Find maximum clique from .clq file using MC-BRB extension")
    parser.add_argument("clq", help="Path to .clq file")
    parser.add_argument("--time", type=float, default=0.0, help="Time limit in seconds (0 for none)")
    parser.add_argument("--approx", action="store_true", help="Run approximate (non-exact) mode")
    args = parser.parse_args()

    try:
        import mc_brb_module
    except Exception as e:
        import glob, os
        so_files = glob.glob("mc_brb_module*.so")
        print("Failed to import mc_brb_module:", e, file=sys.stderr)
        print("Python executable:", sys.executable, file=sys.stderr)
        print("Found built extension files:", so_files, file=sys.stderr)
        print("Try one of:", file=sys.stderr)
        print("  source .venv/bin/activate", file=sys.stderr)
        print("  .venv/bin/python scripts/find_max_clique.py <file> --time 5", file=sys.stderr)
        print("  or rebuild the extension for the current Python: PYTHON=python ./installation.sh", file=sys.stderr)
        sys.exit(2)

    n, adjacency_list = read_clq(args.clq)
    if n == 0:
        print("No nodes found in file.")
        return

    # mc_brb_module.max__clique(number_of_nodes, adjacency_list, time_limit, exact)
    exact = not args.approx
    clique = mc_brb_module.max__clique(n, adjacency_list, args.time, exact)
    # returned indices are 0-based
    clique_1based = [v + 1 for v in clique]
    print("Maximum clique size:", len(clique_1based))
    print("Vertices (1-based):", " ".join(map(str, clique_1based)))

if __name__ == '__main__':
    main()
