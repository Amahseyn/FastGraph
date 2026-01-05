#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <queue>
#include <bitset>
#include <chrono>
#include <cstring>
#include <cassert>
#include <set>
#include <iostream>
#include <random>

namespace py = pybind11;
using namespace std;

// Timer class
class Timer {
private:
    chrono::steady_clock::time_point m_start;

public:
    Timer() { m_start = chrono::steady_clock::now(); }
    void restart() { m_start = chrono::steady_clock::now(); }
    long long elapsed() { 
        auto now = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::microseconds>(now - m_start).count();
    }
};

// Linear Heap for degeneracy ordering
class ListLinearHeap {
private:
    unsigned int n;
    unsigned int key_cap;
    unsigned int min_key, max_key;
    unsigned int *key_s;
    unsigned int *head_s;
    unsigned int *pre_s;
    unsigned int *next_s;

public:
    ListLinearHeap(unsigned int _n, unsigned int _key_cap) {
        n = _n;
        key_cap = _key_cap;
        min_key = max_key = key_cap;
        head_s = key_s = pre_s = next_s = nullptr;
    }

    ~ListLinearHeap() {
        if(head_s) delete[] head_s;
        if(pre_s) delete[] pre_s;
        if(next_s) delete[] next_s;
        if(key_s) delete[] key_s;
    }

    void init(unsigned int _n, unsigned int _key_cap, unsigned int *_id_s, unsigned int *_key_s) {
        if(key_s == nullptr) key_s = new unsigned int[n];
        if(pre_s == nullptr) pre_s = new unsigned int[n];
        if(next_s == nullptr) next_s = new unsigned int[n];
        if(head_s == nullptr) head_s = new unsigned int[key_cap+1];

        assert(_key_cap <= key_cap);
        min_key = max_key = _key_cap;
        for(unsigned int i = 0; i <= _key_cap; i++) head_s[i] = n;

        for(unsigned int i = 0; i < _n; i++) {
            unsigned int id = _id_s[i];
            unsigned int key = _key_s[id];
            assert(id < n && key <= _key_cap);

            key_s[id] = key;
            pre_s[id] = n;
            next_s[id] = head_s[key];
            if(head_s[key] != n) pre_s[head_s[key]] = id;
            head_s[key] = id;

            if(key < min_key) min_key = key;
        }
    }

    bool pop_min(unsigned int &id, unsigned int &key) {
        while(min_key <= max_key && head_s[min_key] == n) ++min_key;
        if(min_key > max_key) return false;

        id = head_s[min_key];
        key = min_key;
        assert(key_s[id] == key);

        head_s[min_key] = next_s[id];
        if(head_s[min_key] != n) pre_s[head_s[min_key]] = n;
        return true;
    }

    unsigned int decrement(unsigned int id, unsigned int dec) {
        assert(key_s[id] >= dec);

        if(pre_s[id] == n) {
            assert(head_s[key_s[id]] == id);
            head_s[key_s[id]] = next_s[id];
            if(next_s[id] != n) pre_s[next_s[id]] = n;
        } else {
            unsigned int pid = pre_s[id];
            next_s[pid] = next_s[id];
            if(next_s[id] != n) pre_s[next_s[id]] = pid;
        }

        unsigned int &key = key_s[id];
        key -= dec;
        pre_s[id] = n;
        next_s[id] = head_s[key];
        if(head_s[key] != n) pre_s[head_s[key]] = id;
        head_s[key] = id;

        if(key < min_key) min_key = key;
        return key;
    }
};

// Main MC-BRB Solver
class MCBRBSolver {
private:
    // Graph data
    int n;
    vector<vector<int>> adj;
    // NOTE: This bitset size is fixed. 
    // If you have more than 50,000 nodes, this must be increased.
    vector<bitset<50000>> adj_bitset; 
    vector<int> degree;
    vector<int> core;
    vector<int> color;
    vector<int> peel_sequence;
    
    // Algorithm state
    vector<int> max_clique;
    vector<int> current_clique;
    vector<bool> visited;
    
    // Time management
    chrono::steady_clock::time_point start_time;
    double time_limit;
    bool time_limit_enabled;
    
    inline bool is_time_up() {
        if (!time_limit_enabled) return false;
        auto current = chrono::steady_clock::now();
        return chrono::duration<double>(current - start_time).count() >= time_limit;
    }
    
    // Build graph from adjacency list
    void build_graph(int number_of_nodes, const vector<unordered_set<int>>& adjacency_list) {
        n = number_of_nodes;
        if (n > 50000) {
            throw std::runtime_error("Number of nodes exceeds bitset limit (50000).");
        }
        
        adj.resize(n);
        adj_bitset.resize(n);
        degree.resize(n);
        visited.assign(n, false);
        
        // Build adjacency lists and bitsets
        for (int i = 0; i < n; i++) {
            // Check if adjacency_list[i] is valid
            if (i >= adjacency_list.size()) {
                 throw std::runtime_error("Adjacency list size mismatch.");
            }
            for (int j : adjacency_list[i]) {
                if (j < n) {
                    adj[i].push_back(j);
                    adj_bitset[i].set(j);
                }
            }
            degree[i] = adj[i].size();
        }
    }
    
    // Degeneracy ordering using linear heap
    void degeneracy_ordering() {
        if (n == 0) return;
        
        vector<unsigned int> id_s(n);
        vector<unsigned int> deg_s(n);
        for (int i = 0; i < n; i++) {
            id_s[i] = i;
            deg_s[i] = degree[i];
        }
        
        ListLinearHeap heap(n, n-1);
        heap.init(n, n-1, id_s.data(), deg_s.data());
        
        peel_sequence.resize(n);
        core.resize(n);
        
        unsigned int max_core_value = 0;
        for (int i = 0; i < n; i++) {
            unsigned int u, key;
            if (!heap.pop_min(u, key)) break;
            
            if (key > max_core_value) max_core_value = key;
            core[u] = max_core_value;
            peel_sequence[i] = u;
            visited[u] = true;
            
            // Update neighbors
            for (int v : adj[u]) {
                if (!visited[v]) {
                    heap.decrement(v, 1);
                }
            }
        }
        
        // Reset visited
        fill(visited.begin(), visited.end(), false);
    }
    
    // Greedy clique for initial solution
    vector<int> greedy_clique() {
        vector<int> clique;
        vector<bool> used(n, false);
        
        // Use degeneracy ordering for better greedy solution
        for (int i = 0; i < n; i++) {
            int v = peel_sequence[i];
            if (used[v]) continue;
            
            bool can_add = true;
            for (int u : clique) {
                if (!adj_bitset[v][u]) {
                    can_add = false;
                    break;
                }
            }
            
            if (can_add) {
                clique.push_back(v);
                used[v] = true;
                
                // Try to extend greedily
                for (int j = i + 1; j < n; j++) {
                    int w = peel_sequence[j];
                    if (used[w]) continue;
                    
                    bool adjacent_to_all = true;
                    for (int u : clique) {
                        if (!adj_bitset[w][u]) {
                            adjacent_to_all = false;
                            break;
                        }
                    }
                    
                    if (adjacent_to_all) {
                        clique.push_back(w);
                        used[w] = true;
                    }
                }
            }
        }
        
        return clique;
    }
    
    // Color the vertices for pruning
    int coloring(vector<int>& vertices) {
        if (vertices.empty()) return 0;
        
        int max_color = 0;
        color.assign(n, -1);
        
        // Sort vertices by degree (descending)
        sort(vertices.begin(), vertices.end(), [&](int a, int b) {
            return degree[a] > degree[b];
        });
        
        for (int v : vertices) {
            vector<bool> used(n + 1, false);
            
            // Mark colors of adjacent vertices
            for (int u : vertices) {
                if (u == v) continue;
                if (adj_bitset[v][u] && color[u] != -1) {
                    used[color[u]] = true;
                }
            }
            
            // Find smallest available color
            int c = 0;
            while (used[c]) c++;
            color[v] = c;
            max_color = max(max_color, c);
        }
        
        return max_color + 1;
    }
    
    // Recursive expansion with pruning - MORE AGGRESSIVE VERSION
    void expand(vector<int>& candidates, int level) {
        if (is_time_up()) return;
        
        if (candidates.empty()) {
            if (current_clique.size() > max_clique.size()) {
                max_clique = current_clique;
            }
            return;
        }
        
        // Color-based pruning
        int color_bound = coloring(candidates);
        if (current_clique.size() + color_bound <= max_clique.size()) {
            return;
        }
        
        // Sort candidates by color in reverse order
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            return color[a] > color[b];
        });
        
        // More aggressive: process more candidates even if bound suggests pruning
        int processed = 0;
        int max_to_process = min(static_cast<int>(candidates.size()), 1000); // Process more
        
        while (!candidates.empty() && processed < max_to_process) {
            if (is_time_up()) return;
            
            int v = candidates.back();
            candidates.pop_back();
            processed++;
            
            // Get neighbors of v that are in candidates
            vector<int> new_candidates;
            for (int u : candidates) {
                if (adj_bitset[v][u]) {
                    new_candidates.push_back(u);
                }
            }
            
            current_clique.push_back(v);
            expand(new_candidates, level + 1);
            current_clique.pop_back();
        }
    }
    
    // Kernelization using k-core reduction
    void kernelization() {
        if (max_clique.empty()) return;
        
        int k = max_clique.size();
        vector<int> deg = degree;
        vector<bool> active(n, true);
        queue<int> q;
        
        // Mark vertices with degree less than k
        for (int i = 0; i < n; i++) {
            if (deg[i] < k) {
                q.push(i);
                active[i] = false;
            }
        }
        
        // Propagate removal
        while (!q.empty()) {
            int v = q.front();
            q.pop();
            
            for (int u : adj[v]) {
                if (active[u] && --deg[u] < k) {
                    active[u] = false;
                    q.push(u);
                }
            }
        }
        
        // Update peel_sequence to only include active vertices
        vector<int> new_peel_sequence;
        for (int v : peel_sequence) {
            if (active[v]) {
                new_peel_sequence.push_back(v);
            }
        }
        peel_sequence = new_peel_sequence;
    }
    
    // Improved greedy extension
    vector<int> grow_clique(const vector<int>& clique) {
        if (clique.empty()) return clique;
        
        vector<bool> in_clique(n, false);
        for (int v : clique) in_clique[v] = true;
        
        // Start with intersection of all neighbors
        bitset<50000> candidates;
        candidates.set();
        for (int v : clique) {
            candidates &= adj_bitset[v];
        }
        
        // Remove vertices already in clique
        for (int v : clique) {
            candidates.reset(v);
        }
        
        // Add vertices in degeneracy order
        for (int v : peel_sequence) {
            if (!candidates[v]) continue;
            if (is_time_up()) break;
            
            bool can_add = true;
            for (int u : clique) {
                if (!adj_bitset[v][u]) {
                    can_add = false;
                    break;
                }
            }
            
            if (can_add) {
                in_clique[v] = true;
                // Update candidates
                candidates &= adj_bitset[v];
            }
        }
        
        // Rebuild clique
        vector<int> new_clique = clique;
        for (int v : peel_sequence) {
            if (in_clique[v] && find(new_clique.begin(), new_clique.end(), v) == new_clique.end()) {
                new_clique.push_back(v);
            }
            if (is_time_up()) break;
        }
        
        return new_clique;
    }
    
    // Multi-start search to use more time
    void multi_start_search() {
        if (is_time_up()) return;
        
        mt19937 rng(42);
        int num_starts = min(n, 50);
        
        for (int start = 0; start < num_starts; start++) {
            if (is_time_up()) break;
            
            // Try different starting points
            int start_vertex;
            if (start < num_starts / 2) {
                if (peel_sequence.empty()) break; // Safety check
                start_vertex = peel_sequence[start % peel_sequence.size()];
            } else {
                uniform_int_distribution<int> dist(0, n-1);
                start_vertex = dist(rng);
            }
            
            if (degree[start_vertex] < (int)max_clique.size()) continue;
            
            vector<int> local_candidates;
            for (int v : peel_sequence) {
                if (v != start_vertex && adj_bitset[start_vertex][v] && degree[v] >= (int)max_clique.size()) {
                    local_candidates.push_back(v);
                }
            }
            
            if (local_candidates.size() + 1 <= max_clique.size()) continue;
            
            current_clique = {start_vertex};
            expand(local_candidates, 0);
        }
    }

public:
    vector<int> solve(int number_of_nodes, 
                     const vector<unordered_set<int>>& adjacency_list,
                     double time_limit_sec = 0.0,
                     bool exact = true) {
        
        start_time = chrono::steady_clock::now();
        time_limit_enabled = (time_limit_sec > 0.0);
        time_limit = time_limit_sec;
        
        if (number_of_nodes == 0) return {};
        
        // Build graph
        build_graph(number_of_nodes, adjacency_list);
        
        // Phase 1: Initial solution using degeneracy ordering
        degeneracy_ordering();
        max_clique = greedy_clique();
        max_clique = grow_clique(max_clique);
        
        if (is_time_up()) {
            sort(max_clique.begin(), max_clique.end());
            return max_clique;
        }
        
        if (!exact) {
            // For non-exact mode, just return the greedy solution
            sort(max_clique.begin(), max_clique.end());
            return max_clique;
        }
        
        // Phase 2: Kernelization
        kernelization();
        
        // Phase 3: Main branch-and-bound with coloring
        vector<int> initial_candidates;
        for (int v : peel_sequence) {
            if (static_cast<int>(degree[v]) >= static_cast<int>(max_clique.size())) {
                initial_candidates.push_back(v);
            }
        }
        
        if (!initial_candidates.empty()) {
            current_clique.clear();
            expand(initial_candidates, 0);
        }
        
        // Phase 4: Multi-start search to use remaining time
        if (!is_time_up()) {
            multi_start_search();
        }
        
        // Final improvement
        if (!is_time_up()) {
            max_clique = grow_clique(max_clique);
        }
        
        sort(max_clique.begin(), max_clique.end());
        return max_clique;
    }
};

// Python module
PYBIND11_MODULE(mc_brb_module, m) {
    m.doc() = "MC-BRB Maximum Clique Algorithm with Exact/Approximate modes";
    
    py::class_<MCBRBSolver>(m, "MCBRBSolver")
        .def(py::init<>())
        .def("solve", &MCBRBSolver::solve,
            "MC-BRB algorithm for maximum clique",
            py::arg("number_of_nodes"),
            py::arg("adjacency_list"), 
            py::arg("time_limit") = 0.0,
            py::arg("exact") = true);
    
    m.def("max__clique", [](int number_of_nodes,
                           const vector<unordered_set<int>>& adjacency_list,
                           double time_limit = 0.0,
                           bool exact = true) {
        MCBRBSolver solver;
        return solver.solve(number_of_nodes, adjacency_list, time_limit, exact);
    }, "Find maximum clique using MC-BRB algorithm",
    py::arg("number_of_nodes"),
    py::arg("adjacency_list"),
    py::arg("time_limit") = 0.0,
    py::arg("exact") = true);
}