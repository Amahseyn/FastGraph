#!/usr/bin/env python3
"""Compare `mc_brb_module` maximum clique against NetworkX's max clique.

Usage:
  python scripts/compare_with_networkx.py path/to/graph.clq --time 5 --nx-timeout 30

This runs the C++ pybind11 extension (optionally with a time limit) and
runs NetworkX's maximal-cliques search (with an optional timeout).
"""
import argparse
import time
import sys
import os
from collections import defaultdict


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
        nodes = set(adj.keys())
        for s in adj.values():
            nodes |= s
        n = max(nodes) + 1 if nodes else 0

    adjacency_list = [set() for _ in range(n)]
    for u, s in adj.items():
        adjacency_list[u] = s
    return n, adjacency_list


def run_networkx_max_clique(G, timeout=None):
    # Run in a subprocess to allow timeout
    from multiprocessing import Process, Queue

    def worker(q):
        import networkx as nx
        # find_cliques yields maximal cliques; take the largest
        maxc = []
        for c in nx.find_cliques(G):
            if len(c) > len(maxc):
                maxc = c
        q.put(maxc)

    q = Queue()
    p = Process(target=worker, args=(q,))
    p.start()
    p.join(timeout)
    if p.is_alive():
        p.terminate()
        p.join()
        return None, True
    if q.empty():
        return None, False
    res = q.get()
    return res, False


def main():
    parser = argparse.ArgumentParser(description="Compare mc_brb vs NetworkX maximum clique")
    parser.add_argument("clq")
    parser.add_argument("--time", type=float, default=0.0, help="Time limit for mc_brb (seconds, 0 = unlimited)")
    parser.add_argument("--nx-timeout", type=float, default=30.0, help="Time limit for NetworkX (seconds)")
    parser.add_argument("--approx", action="store_true", help="Run mc_brb in approximate (non-exact) mode")
    args = parser.parse_args()

    # ensure project root on path so compiled module is importable
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    if project_root not in sys.path:
        sys.path.insert(0, project_root)

    try:
        import mc_brb_module
    except Exception as e:
        print("Failed to import mc_brb_module:", e, file=sys.stderr)
        sys.exit(2)

    import networkx as nx

    n, adjacency_list = read_clq(args.clq)
    if n == 0:
        print("Empty graph")
        return

    # build networkx graph
    G = nx.Graph()
    G.add_nodes_from(range(n))
    for u in range(n):
        for v in adjacency_list[u]:
            if u < v:
                G.add_edge(u, v)

    print(f"Nodes: {n}, edges: {G.number_of_edges()}")

    # Run mc_brb
    mc_start = time.time()
    clique_mc = mc_brb_module.max__clique(n, adjacency_list, args.time, not args.approx)
    mc_time = time.time() - mc_start

    print("mc_brb_module: size=", len(clique_mc), "time=", f"{mc_time:.3f}s")

    # Run NetworkX (with timeout)
    nx_start = time.time()
    nx_clique, timed_out = run_networkx_max_clique(G, timeout=args.nx_timeout)
    nx_time = time.time() - nx_start
    if timed_out:
        print(f"NetworkX: timed out after {args.nx_timeout}s (partial time: {nx_time:.3f}s)")
    else:
        print("NetworkX: size=", len(nx_clique), "time=", f"{nx_time:.3f}s")

    # Basic comparison
    if nx_clique is None:
        print("NetworkX result: none (timed out or failed)")
    else:
        same = set(clique_mc) == set(nx_clique)
        print("Cliques identical:", same)


if __name__ == '__main__':
    main()
