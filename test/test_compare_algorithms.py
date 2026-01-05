import os
import sys

# Ensure project root is importable
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

import mc_brb_module
import max_clique_module


def is_clique(adj_list, clique):
    s = set(clique)
    for u in clique:
        for v in clique:
            if u == v:
                continue
            if v not in adj_list[u]:
                return False
    return True


def test_both_find_triangle():
    n = 3
    adjacency_list = [set() for _ in range(n)]
    edges = [(0, 1), (1, 2), (2, 0)]
    for u, v in edges:
        adjacency_list[u].add(v)
        adjacency_list[v].add(u)

    clique1 = mc_brb_module.max__clique(n, adjacency_list, 0.0, True)
    clique2 = max_clique_module.get_max_clique(n, adjacency_list, optional_time_limit=1.0)

    assert len(clique1) == 3
    assert len(clique2) == 3
    assert is_clique(adjacency_list, clique1)
    assert is_clique(adjacency_list, clique2)


def test_both_on_small_random_graph():
    # small graph where brute-force should be fine
    n = 8
    adjacency_list = [set() for _ in range(n)]
    edges = [(0,1),(1,2),(2,3),(3,0),(0,2),(4,5),(5,6),(6,4)]
    for u,v in edges:
        adjacency_list[u].add(v)
        adjacency_list[v].add(u)

    clique1 = mc_brb_module.max__clique(n, adjacency_list, 0.0, True)
    clique2 = max_clique_module.get_max_clique(n, adjacency_list, optional_time_limit=1.0)

    assert is_clique(adjacency_list, clique1)
    assert is_clique(adjacency_list, clique2)
    # ensure sizes are equal (both should find size 3 here)
    assert len(clique1) == len(clique2)
