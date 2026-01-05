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

def run_with_timeout(func, args=(), timeout=20):
    """Run func(args) in a subprocess with timeout (seconds). Returns (ok, result_or_error, timed_out)."""
    from multiprocessing import Process, Queue

    def worker(q, args):
        try:
            res = func(*args)
            q.put((True, res))
        except Exception as e:
            q.put((False, str(e)))

    q = Queue()
    p = Process(target=worker, args=(q, args))
    p.start()
    p.join(timeout)
    if p.is_alive():
        p.terminate()
        p.join()
        return False, "timed out", True
    if q.empty():
        return False, "no result", False
    ok, val = q.get()
    return ok, val, False


def main():
    print("SMOKE: start", flush=True)
    print("SMOKE: cwd=", os.getcwd(), flush=True)
    print("SMOKE: PYTHON=", sys.executable, flush=True)
    # Try to import both modules; if SKIP_MISSING_MODULES is set, treat missing modules as warnings
    skip_missing = os.environ.get('SKIP_MISSING_MODULES', '0') not in ('0', '')
    have_mc = False
    have_max = False

    try:
        print("SMOKE: importing mc_brb_module", flush=True)
        import mc_brb_module
        have_mc = True
        print("SMOKE: imported mc_brb_module", flush=True)
    except Exception as e:
        print('SMOKE: mc_brb_module import failed:', e, file=sys.stderr, flush=True)
        if not skip_missing:
            sys.exit(2)

    try:
        print("SMOKE: importing max_clique_module", flush=True)
        import max_clique_module
        have_max = True
        print("SMOKE: imported max_clique_module", flush=True)
    except Exception as e:
        print('SMOKE: max_clique_module import failed:', e, file=sys.stderr, flush=True)
        if not skip_missing:
            sys.exit(2)

    # Tiny graph: triangle
    n = 3
    adjacency_list = [set() for _ in range(n)]
    edges = [(0,1),(1,2),(2,0)]
    for u, v in edges:
        adjacency_list[u].add(v)
        adjacency_list[v].add(u)

    # Run each call with a timeout so CI can't hang forever
    res1 = None
    res2 = None
    to1 = to2 = False
    ok1 = ok2 = False

    if have_mc:
        print("SMOKE: running mc_brb_module.max__clique with timeout", flush=True)
        ok1, res1, to1 = run_with_timeout(mc_brb_module.max__clique, args=(n, adjacency_list, 0.0, True), timeout=20)
        print("SMOKE: mc_brb ok, timed_out?", ok1, to1, "res=", (res1 if isinstance(res1, str) else ('list(len=%d)'%len(res1))), flush=True)
    else:
        print('SMOKE: skipping mc_brb_module call (module not available)', flush=True)

    if have_max:
        print("SMOKE: running max_clique_module.get_max_clique with timeout (passing short internal time limit)", flush=True)
        # pass a small optional_time_limit to the C++ function so it returns quickly
        ok2, res2, to2 = run_with_timeout(max_clique_module.get_max_clique, args=(n, adjacency_list, 77701, 1.0), timeout=20)
        print("SMOKE: max_clique ok, timed_out?", ok2, to2, "res=", (res2 if isinstance(res2, str) else ('list(len=%d)'%len(res2))), flush=True)
    else:
        print('SMOKE: skipping max_clique_module call (module not available)', flush=True)

    if to1 or to2:
        print('SMOKE: One of the module calls timed out', file=sys.stderr, flush=True)
        sys.exit(4)

    if (have_mc and not ok1) or (have_max and not ok2):
        print('SMOKE: One of the module calls failed', file=sys.stderr, flush=True)
        sys.exit(5)

    # If both modules available, ensure they found the triangle; otherwise pass with warning
    if have_mc and have_max:
        if len(res1) != 3 or len(res2) != 3:
            print('SMOKE: unexpected clique sizes', len(res1), len(res2), file=sys.stderr, flush=True)
            sys.exit(3)
        print('SMOKE: passed: both modules found triangle clique', flush=True)
    elif have_mc or have_max:
        print('SMOKE: partial pass: one module available and ran successfully', flush=True)
    else:
        print('SMOKE: no modules available to test', file=sys.stderr, flush=True)
        sys.exit(2)

if __name__ == '__main__':
    main()
