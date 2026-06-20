#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <limits>

using namespace std;
using namespace std::chrono;

const double INF = numeric_limits<double>::infinity();

struct Tree {
    int n;
    vector<vector<int>> children;
    vector<vector<double>> cap;
    vector<double> incoming_cap;
    vector<int> depth;

    Tree(int size) : n(size), children(size + 1), cap(size + 1),
                     incoming_cap(size + 1, 0.0), depth(size + 1, 0) {
        for(int i=0; i<=n; ++i) {
            children[i].reserve(2); 
            cap[i].reserve(2);
        }
    }

    void add_edge(int u, int v, double c) {
        children[u].push_back(v);
        cap[u].push_back(c);
        incoming_cap[v] = c;
    }
};

void calculate_depths(Tree& tree) {
    if (tree.n < 1) return;
    queue<int> q; q.push(1);
    tree.depth[1] = 0;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : tree.children[u]) {
            tree.depth[v] = tree.depth[u] + 1;
            q.push(v);
        }
    }
}

bool check_balanced(const Tree& tree) {
    for (int u = 1; u <= tree.n; ++u) {
        if (u == 1 || tree.children[u].empty()) continue;
        double sum_in = tree.incoming_cap[u];
        double sum_out = 0.0;
        for (size_t i = 0; i < tree.children[u].size(); ++i)
            sum_out += tree.cap[u][i];
        if (abs(sum_in - sum_out) > 1e-9) return false;
    }
    return true;
}

vector<int> get_bottom_up_order(const Tree& tree) {
    vector<int> order;
    order.reserve(tree.n);
    queue<int> q; 
    if (tree.n >= 1) q.push(1);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        order.push_back(u);
        for (int v : tree.children[u]) q.push(v);
    }
    reverse(order.begin(), order.end());
    return order;
}

// =====================================================================
// 1. БАЗОВЫЙ АЛГОРИТМ (O(k * n))
// =====================================================================

double iterative_max_flow_finder(const Tree& tree, const vector<int>& order, vector<double>& dp) {
    for (int u : order) {
        if (tree.children[u].empty()) {
            dp[u] = INF;
        } else {
            double total = 0.0;
            for (size_t i = 0; i < tree.children[u].size(); ++i) {
                total += min(tree.cap[u][i], dp[tree.children[u][i]]);
            }
            dp[u] = total;
        }
    }
    return dp[1];
}

double compute_flow_to_depth(const Tree& tree, const vector<int>& order, int target_depth) {
    vector<double> dp(tree.n + 1, 0.0);
    for (int u : order) {
        if (tree.children[u].empty()) {
            dp[u] = (tree.depth[u] == target_depth) ? INF : 0.0;
        } else {
            double total = 0.0;
            for (size_t i = 0; i < tree.children[u].size(); ++i) {
                total += min(tree.cap[u][i], dp[tree.children[u][i]]);
            }
            dp[u] = total;
        }
    }
    return dp[1];
}

double baseline_burst(Tree& tree) {
    calculate_depths(tree);
    vector<int> order = get_bottom_up_order(tree);
    vector<double> dp(tree.n + 1, 0);
    double f_max = iterative_max_flow_finder(tree, order, dp);
    
    unordered_map<int, double> depth_flow;
    for (int u : order) {
        if (tree.children[u].empty()) {
            depth_flow[tree.depth[u]] = 0.0;
        }
    }
    
    double sum_f_max_by_depth = 0.0;
    for (auto& [d, val] : depth_flow) {
        sum_f_max_by_depth += compute_flow_to_depth(tree, order, d);
    }

    double sum_leaf_cap = 0.0;
    for (int u : order) {
        if (tree.children[u].empty()) sum_leaf_cap += tree.incoming_cap[u];
    }

    bool has_burst = (depth_flow.size() > 1) && (sum_leaf_cap > f_max) && !check_balanced(tree);
    return has_burst ? sum_f_max_by_depth : f_max;
}

// =====================================================================
// 2. ОПТИМИЗИРОВАННЫЙ АЛГОРИТМ (Small-to-Large, unordered_map + reserve)
// =====================================================================

unordered_map<int, double> compute_optimized_iterative(const Tree& tree, const vector<int>& order) {
    vector<unordered_map<int, double>> M(tree.n + 1);

    for (int u : order) {
        if (tree.children[u].empty()) {
            M[u].reserve(1); //
            M[u][tree.depth[u]] = INF;
        } else {
            int best_i = 0;
            for (size_t i = 1; i < tree.children[u].size(); ++i) {
                if (M[tree.children[u][i]].size() > M[tree.children[u][best_i]].size()) {
                    best_i = i;
                }
            }

            int v_max = tree.children[u][best_i];
            M[u] = move(M[v_max]);

            double c_vmax = tree.cap[u][best_i];
            for (auto& [d, val] : M[u]) {
                val = min(c_vmax, val);
            }

            for (size_t i = 0; i < tree.children[u].size(); ++i) {
                if ((int)i == best_i) continue;
                
                int v = tree.children[u][i];
                double c_v = tree.cap[u][i];

                M[u].reserve(M[u].size() + M[v].size());
                
                for (auto& [d, val] : M[v]) {
                    double term = min(c_v, val);
                    auto it = M[u].find(d);
                    if (it != M[u].end()) {
                        it->second += term;
                    } else {
                        M[u][d] = term;
                    }
                }
                M[v].clear(); 
            }
        }
    }
    return M[1];
}

double optimized_burst(Tree& tree) {
    calculate_depths(tree);
    vector<int> order = get_bottom_up_order(tree);
    vector<double> dp(tree.n + 1, 0);
    double f_max = iterative_max_flow_finder(tree, order, dp);
    
    unordered_map<int, double> M_s = compute_optimized_iterative(tree, order);
    
    double sum_f_max_by_depth = 0.0;
    for (auto& [d, val] : M_s) {
        sum_f_max_by_depth += val;
    }

    double sum_leaf_cap = 0.0;
    for (int u : order) {
        if (tree.children[u].empty()) sum_leaf_cap += tree.incoming_cap[u];
    }

    bool has_burst = (M_s.size() > 1) && (sum_leaf_cap > f_max) && !check_balanced(tree);
    return has_burst ? sum_f_max_by_depth : f_max;
}

// =====================================================================
// 3. ГЕНЕРАТОР И ЭКСПЕРИМЕНТ
// =====================================================================

Tree generate_burst_tree(int n) {
    if (n < 4) n = 4; 
    Tree tree(n);
    int spine_len = n / 2;

    for (int i = 1; i < spine_len; ++i) {
        tree.add_edge(i, i + 1, 10.0);
    }

    for (int i = 1; i <= spine_len; ++i) {
        int leaf_id = spine_len + i;
        if (leaf_id <= n) {
            tree.add_edge(i, leaf_id, 100.0);
        }
    }

    for (int i = 2 * spine_len + 1; i <= n; ++i) {
        tree.add_edge(1, i, 100.0);
    }

    return tree;
}

int get_unique_depths_count(Tree& tree) {
    calculate_depths(tree);
    unordered_map<int, int> depths;
    for (int u = 1; u <= tree.n; ++u) {
        if (tree.children[u].empty()) {
            depths[tree.depth[u]]++;
        }
    }
    return depths.size();
}

void run_experiment() {
    ofstream out("experiment_results.txt");
    if (!out.is_open()) {
        cerr << "Ошибка создания файла результатов!" << endl;
        return;
    }

    out << left << setw(10) << "N" 
        << setw(25) << "Unique_Depths(k)" 
        << setw(25) << "Baseline_Time(ms)" 
        << setw(25) << "Optimized_Time(ms)" 
        << endl;
    out << string(85, '-') << endl;

    cout << "Эксперимент запущен (unordered_map + reserve)..." << endl;

    for (int n = 1000; n <= 128000; n *= 2) {
        cout << "Тестируем N = " << n << "..." << flush;
        
        Tree tree = generate_burst_tree(n);
        int k = get_unique_depths_count(tree);

        auto start_base = high_resolution_clock::now();
        double v_base = baseline_burst(tree);
        auto stop_base = high_resolution_clock::now();
        auto duration_base = duration_cast<milliseconds>(stop_base - start_base);

        auto start_opt = high_resolution_clock::now();
        double v_opt = optimized_burst(tree);
        auto stop_opt = high_resolution_clock::now();
        auto duration_opt = duration_cast<milliseconds>(stop_opt - start_opt);

        if (abs(v_base - v_opt) > 1e-6) {
            cerr << " РАСХОЖДЕНИЕ!" << endl;
        } else {
            cout << " OK" << endl;
        }

        out << left << setw(10) << n 
            << setw(25) << k 
            << setw(25) << duration_base.count() 
            << setw(25) << duration_opt.count() 
            << endl;
        out.flush();
    }

    out.close();
    cout << "Эксперимент завершен!" << endl;
}

int main() {
    setlocale(LC_ALL, "Russian");
    run_experiment();
    return 0;
}