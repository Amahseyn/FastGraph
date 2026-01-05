#!/usr/bin/env python3
"""Lightweight smoke test: import built extensions and run tiny checks.

This script is intentionally small and fast so CI can verify basic functionality
without running the full pytest suite.
"""
import sys
import os

# Ensure project root is importable when running from CI working directory
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

def main():
    try:
        import mc_brb_module
        import max_clique_module
    except Exception as e:
        print('Import failed:', e, file=sys.stderr)
        sys.exit(2)

    # Tiny graph: triangle
    n = 3
    adjacency_list = [set() for _ in range(n)]
    edges = [(0,1),(1,2),(2,0)]
    for u, v in edges:
        adjacency_list[u].add(v)
        adjacency_list[v].add(u)

    c1 = mc_brb_module.max__clique(n, adjacency_list, 0.0, True)
    c2 = max_clique_module.get_max_clique(n, adjacency_list)

    if len(c1) != 3 or len(c2) != 3:
        print('Smoke test failed: unexpected clique sizes', len(c1), len(c2))
        sys.exit(3)

    print('Smoke test passed: both modules imported and found triangle clique')

if __name__ == '__main__':
    main()
