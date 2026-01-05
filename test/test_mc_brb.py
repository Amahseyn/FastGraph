import os
import sys

# Ensure project root is importable
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

import mc_brb_module


def test_small_triangle_clique():
    # Graph: 0-1-2 form a triangle, 3 connected only to 2
    n = 4
    adjacency_list = [set() for _ in range(n)]
    edges = [(0, 1), (1, 2), (2, 0), (2, 3)]
    for u, v in edges:
        adjacency_list[u].add(v)
        adjacency_list[v].add(u)

    clique = mc_brb_module.max__clique(n, adjacency_list, 0.0, True)
    assert isinstance(clique, list)
    assert len(clique) == 3
    assert set(clique) == {0, 1, 2}


def test_disconnected_nodes():
    # No edges -> max clique size 1 (single vertex)
    n = 5
    adjacency_list = [set() for _ in range(n)]
    clique = mc_brb_module.max__clique(n, adjacency_list, 0.0, True)
    assert len(clique) == 1
