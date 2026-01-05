#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <set>
#include <unordered_set> 
#include <algorithm>
#include <random>
#include <chrono>
#include <numeric>
#include <thread>
#include <future>
#include <atomic>

#if defined(__GNUC__) || defined(__clang__)
#define popcount(x) __builtin_popcountll(x)
#define ctz(x) __builtin_ctzll(x)
#else
#include <intrin.h>
#define popcount(x) __popcnt64(x)
#define ctz(x) _tzcnt_u64(x)
#endif

namespace py = pybind11;
using namespace std;

constexpr int WORD_SIZE = 64;

class BitSet {
public:
    vector<uint64_t> words;
    int n_nodes;
    int current_size; 

    BitSet(int n = 0) : n_nodes(n), current_size(0) {
        words.resize((n + WORD_SIZE - 1) / WORD_SIZE, 0);
    }
    
    BitSet(const BitSet& other) = default;
    BitSet& operator=(const BitSet& other) = default;

    inline void set(int i) {
        uint64_t& word = words[i / WORD_SIZE];
        uint64_t mask = (1ULL << (i % WORD_SIZE));
        if (!(word & mask)) { 
            word |= mask;
            current_size++;
        }
    }

    inline void clear(int i) {
        uint64_t& word = words[i / WORD_SIZE];
        uint64_t mask = (1ULL << (i % WORD_SIZE));
        if (word & mask) {
            word &= ~mask;
            current_size--;
        }
    }

    inline bool get(int i) const {
        return (words[i / WORD_SIZE] >> (i % WORD_SIZE)) & 1;
    }

    inline int count() const {
        return current_size;
    }
    
    inline int intersect_count(const BitSet& other) const {
        int total = 0;
        const uint64_t* p1 = words.data();
        const uint64_t* p2 = other.words.data();
        size_t size = words.size();
        for (size_t i = 0; i < size; ++i) {
            total += popcount(p1[i] & p2[i]);
        }
        return total;
    }
};


vector<BitSet> _mk_bitsets(int n, const vector<unordered_set<int>>& adjacency_list) {
    vector<BitSet> bitsets(n, BitSet(n));
    for (int v = 0; v < n; v++) {
        for (int u : adjacency_list[v]) {
            if (u < n && adjacency_list[u].count(v)) {
                bitsets[v].set(u);
            }
        }
    }
    return bitsets;
}

vector<int> _colour_order(int n, const vector<unordered_set<int>>& adjacency_list, const vector<BitSet>& bitsets) {
    vector<int> colour(n, 0);
    vector<int> all_nodes(n);
    iota(all_nodes.begin(), all_nodes.end(), 0);
    
    sort(all_nodes.begin(), all_nodes.end(), [&](int a, int b){
        return bitsets[a].count() > bitsets[b].count(); 
    });
    
    vector<bool> used_colours_vec(n, false);
    
    for (int v : all_nodes) {
        const BitSet& neighbors = bitsets[v];
        const uint64_t* neighbor_words = neighbors.words.data();
        size_t words_size = neighbors.words.size();

        for (size_t i = 0; i < words_size; ++i) {
            uint64_t word = neighbor_words[i];
            if (word == 0) continue;
            while (word > 0) {
                int u = i * WORD_SIZE + ctz(word);
                used_colours_vec[colour[u]] = true; 
                word &= word - 1; 
            }
        }
        
        int c = 0;
        while (c < n && used_colours_vec[c]) { 
            c++; 
        }
        colour[v] = c;

        for (size_t i = 0; i < words_size; ++i) {
            uint64_t word = neighbor_words[i];
            if (word == 0) continue;
            while (word > 0) {
                int u = i * WORD_SIZE + ctz(word);
                used_colours_vec[colour[u]] = false;
                word &= word - 1; 
            }
        }
    }

    vector<int> order(n);
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b) { return colour[a] > colour[b]; });
    return order;
}

// Helper function to check if time limit has been exceeded
inline bool check_time_exceeded(double optional_time_limit, const chrono::steady_clock::time_point& start_time) {
    if (optional_time_limit < 0) return false; // No time limit
    return chrono::duration<double>(chrono::steady_clock::now() - start_time).count() >= optional_time_limit;
}

vector<int> _get_largest_clique(
    int number_of_nodes,
    const vector<unordered_set<int>>& adjacency_list,
    int seed,
    double optional_time_limit = -1.0, // Negative means no time limit
    int tabu_tenure = -1,
    double tabu_phase_ratio = 0.99
) {
    if (number_of_nodes == 0) return {};

    mt19937 rng(seed);
    auto start_time = chrono::steady_clock::now();
    
    // Set end times only if time limit is specified
    double end_time_sec = (optional_time_limit >= 0) ? optional_time_limit * 0.985 : numeric_limits<double>::max();
    double tabu_end_sec = (optional_time_limit >= 0) ? end_time_sec * tabu_phase_ratio : numeric_limits<double>::max();

    vector<int> nodes(number_of_nodes);
    iota(nodes.begin(), nodes.end(), 0);

    auto bitsets = _mk_bitsets(number_of_nodes, adjacency_list);
    
    BitSet best_clique(number_of_nodes);
    
    vector<int> order_deg = nodes;
    sort(order_deg.begin(), order_deg.end(), [&](int a, int b) {
        return bitsets[a].count() > bitsets[b].count(); 
    });
    
    vector<int> order_col = _colour_order(number_of_nodes, adjacency_list, bitsets);
    
    for (const auto& order : {order_deg, order_col}) {
        BitSet candidate_clique(number_of_nodes);
        for (int v : order) {
            if (candidate_clique.count() == 0 || candidate_clique.intersect_count(bitsets[v]) == candidate_clique.count()) {
                candidate_clique.set(v);
            }
        }
        if (candidate_clique.count() > best_clique.count()) {
            best_clique = candidate_clique;
        }
    }
    
    BitSet current_clique = best_clique;
    vector<int> tabu_list(number_of_nodes, 0);
    int iteration = 0;
    int no_improvement_streak = 0;
    size_t words_size = current_clique.words.size();

    while (!check_time_exceeded(tabu_end_sec, start_time)) {
        iteration++;
        
        int best_move_u = -1;
        int min_conflicts = number_of_nodes + 1;

        shuffle(nodes.begin(), nodes.end(), rng);
        for (int u : nodes) {
            if (current_clique.get(u) || tabu_list[u] > iteration) continue;
            
            int c_size = current_clique.count();
            int intersect_size = current_clique.intersect_count(bitsets[u]);
            int conflict_count = c_size - intersect_size;

            if (conflict_count < min_conflicts) {
                min_conflicts = conflict_count;
                best_move_u = u;
                if (min_conflicts <= 1) break;
            }
        }

        if (best_move_u != -1) {
            uint64_t* current_words = current_clique.words.data();
            const uint64_t* neighbor_words = bitsets[best_move_u].words.data();
            int tenure = (tabu_tenure > 0) ? tabu_tenure : (rng() % 10) + 7;
            
            for(size_t i = 0; i < words_size; ++i){
                uint64_t conflicts = current_words[i] & ~neighbor_words[i];
                if(conflicts > 0){
                    int num_removed = popcount(conflicts);
                    current_clique.current_size -= num_removed;

                    uint64_t temp_conflicts = conflicts;
                    while(temp_conflicts > 0){
                        int v = i * WORD_SIZE + ctz(temp_conflicts);
                        tabu_list[v] = iteration + tenure;
                        temp_conflicts &= temp_conflicts - 1;
                    }
                    current_words[i] &= ~conflicts;
                }
            }
            current_clique.set(best_move_u);
            tabu_list[best_move_u] = iteration + tenure;

            if (current_clique.count() > best_clique.count()) {
                best_clique = current_clique;
                no_improvement_streak = 0;
            } else {
                no_improvement_streak++;
            }
        } else {
            no_improvement_streak++;
        }
        
        if (no_improvement_streak > 200 && current_clique.count() > 1) { 
            vector<int> members;
            members.reserve(current_clique.count());
            for (size_t i = 0; i < words_size; ++i) {
                if (current_clique.words[i] == 0) continue;
                uint64_t word = current_clique.words[i];
                while (word > 0) {
                    int u = i * WORD_SIZE + ctz(word);
                    members.push_back(u);
                    word &= word - 1;
                }
            }
            
            shuffle(members.begin(), members.end(), rng);
            if(!members.empty()) current_clique.clear(members[0]);
            no_improvement_streak = 0;
        }
    }
    
    int perturb_k = max(1, best_clique.count() / 5);
    while (!check_time_exceeded(end_time_sec, start_time)) {
        BitSet clique_to_rebuild = best_clique;
        if(clique_to_rebuild.count() > 3){
            vector<int> members;
            members.reserve(clique_to_rebuild.count());
            for (size_t i = 0; i < words_size; ++i) {
                if (clique_to_rebuild.words[i] == 0) continue;
                uint64_t word = clique_to_rebuild.words[i];
                while (word > 0) {
                    int u = i * WORD_SIZE + ctz(word);
                    members.push_back(u);
                    word &= word - 1;
                }
            }
            
            shuffle(members.begin(), members.end(), rng);
            int k = min(perturb_k, (int)members.size());
            for(int i=0; i < k; ++i) clique_to_rebuild.clear(members[i]);
        }
        
        for(int v : order_col){
            if(clique_to_rebuild.get(v)) continue;
            if(clique_to_rebuild.count() == 0 || clique_to_rebuild.intersect_count(bitsets[v]) == clique_to_rebuild.count()){
                clique_to_rebuild.set(v);
            }
        }

        if(clique_to_rebuild.count() > best_clique.count()){
            best_clique = clique_to_rebuild;
            perturb_k = max(1, best_clique.count() / 5);
        } else {
            perturb_k = min(perturb_k + 1, max(2, best_clique.count() / 2));
        }
    }

    bool extended = true;
    while (extended) {
        extended = false;
        shuffle(nodes.begin(), nodes.end(), rng);
        for (int v : nodes) {
            if (best_clique.get(v)) continue;
            if (best_clique.count() == 0 || best_clique.intersect_count(bitsets[v]) == best_clique.count()) {
                best_clique.set(v);
                extended = true;
            }
        }
    }

    vector<int> result;
    result.reserve(best_clique.count());
    for (size_t i = 0; i < words_size; ++i) {
        if (best_clique.words[i] == 0) continue;
        uint64_t word = best_clique.words[i];
        while (word > 0) {
            int u = i * WORD_SIZE + ctz(word);
            result.push_back(u);
            word &= word - 1;
        }
    }
    sort(result.begin(), result.end());
    return result;
}

vector<int> get_max_clique_parallel(
    int number_of_nodes,
    const vector<unordered_set<int>>& adjacency_list,
    int seed = 77701,
    double optional_time_limit = -1.0, // Negative means no time limit
    int tabu_tenure = -1,
    double tabu_phase_ratio = 0.99,
    int cpu_count = 1
) {
    if (number_of_nodes == 0) return {};
    
    if (cpu_count <= 1) {
        return _get_largest_clique(number_of_nodes, adjacency_list, seed, optional_time_limit, tabu_tenure, tabu_phase_ratio);
    }
    
    cpu_count = min(cpu_count, static_cast<int>(thread::hardware_concurrency()));
    vector<future<vector<int>>> futures;
    atomic<int> best_size(0);
    vector<int> best_clique;
    mutex result_mutex;
    
    for (int i = 0; i < cpu_count; ++i) {
        futures.push_back(async(launch::async, [&, i]() {
            int thread_seed = seed + i * 1000;
            auto result = _get_largest_clique(number_of_nodes, adjacency_list, thread_seed, 
                                            optional_time_limit, tabu_tenure, tabu_phase_ratio);
            
            lock_guard<mutex> lock(result_mutex);
            if (result.size() > best_size.load()) {
                best_size = result.size();
                best_clique = result;
            }
            return result;
        }));
    }
    
    for (auto& future : futures) {
        future.get();
    }
    
    return best_clique;
}

vector<int> get_max_clique(
    int number_of_nodes,
    const vector<unordered_set<int>>& adjacency_list,
    int seed = 77701,
    double optional_time_limit = -1.0, // Negative means no time limit
    int tabu_tenure = -1,
    double tabu_phase_ratio = 0.99
) {
    if (number_of_nodes == 0) return {};
    return _get_largest_clique(number_of_nodes, adjacency_list, seed, optional_time_limit, tabu_tenure, tabu_phase_ratio);
}

PYBIND11_MODULE(max_clique_module, m) {
    m.doc() = "High-Performance and Correct Maximum Clique Finder with Optional Time Limit";

    m.def("get_max_clique", [](
          int number_of_nodes,
          const vector<unordered_set<int>>& adjacency_list, 
          int seed,
          double optional_time_limit,
          int tabu_tenure,
          double tabu_phase_ratio,
          int cpu_count
          ) {
              if (cpu_count > 1) {
                  return get_max_clique_parallel(number_of_nodes, adjacency_list, seed, optional_time_limit, tabu_tenure, tabu_phase_ratio, cpu_count);
              } else {
                  return get_max_clique(number_of_nodes, adjacency_list, seed, optional_time_limit, tabu_tenure, tabu_phase_ratio);
              }
          },
          "Finds maximum clique using a hybrid Tabu Search and Iterated Greedy algorithm. "
          "Set optional_time_limit to a negative value for no time limit.",
          py::arg("number_of_nodes"),
          py::arg("adjacency_list"),
          py::arg("seed") = 77701,
          py::arg("optional_time_limit") = -1.0, // Negative = no time limit
          py::arg("tabu_tenure") = -1,
          py::arg("tabu_phase_ratio") = 0.99,
          py::arg("cpu_count") = 1);
}