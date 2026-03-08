#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <set>
#include <cmath>
#include <iomanip>

using namespace std;

// ============================================================================
// Структура данных (Листинг 5)
// ============================================================================
struct Tree {
    int n;
    vector<vector<int>> children;
    vector<vector<double>> cap;
    vector<double> incoming_cap;
    vector<int> depth;

    Tree(int size) : n(size),
        children(size + 1),
        cap(size + 1),
        incoming_cap(size + 1, 0.0),
        depth(size + 1, 0) {
    }

    void add_edge(int u, int v, double capacity) {
        children[u].push_back(v);
        cap[u].push_back(capacity);
        incoming_cap[v] = capacity;
    }
};

// ============================================================================
// 1. BFS-нумерация и вычисление глубин
// ============================================================================
void bfs_with_depth(Tree& tree, vector<int>& order) {
    queue<int> q;
    q.push(1);

    tree.depth[1] = 0; // Инициализируем глубину корня

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        order.push_back(u);

        for (int v : tree.children[u]) {
            tree.depth[v] = tree.depth[u] + 1;
            q.push(v);
        }
    }
}

// ============================================================================
// 2. УНИВЕРСАЛЬНЫЙ Bottom-Up DP (Объединяет Листинг 7 и исправленный 8)
// target_depth = -1 : вычисляет обычный F_max (все листья активны)
// target_depth >= 0 : вычисляет F^(d)_max (активны только листья глубины d)
// ============================================================================
double bottom_up_dp(const Tree& tree, const vector<int>& order, int target_depth = -1) {
    vector<double> dp(tree.n + 1, 0.0);

    // Обратный порядок (от листьев к корню)
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        int u = *it;

        if (tree.children[u].empty()) {
            // Базис индукции: Лист
            // Если target_depth == -1 (все листья) ИЛИ глубина совпадает
            if (target_depth == -1 || tree.depth[u] == target_depth) {
                dp[u] = 1e18; // Активный лист (бесконечность)
            }
            else {
                dp[u] = 0.0;  // Неактивный лист (блокируем поток)
            }
        }
        else {
            // Индукционный шаг: Промежуточная вершина (Формула 1 и 9)
            double total = 0.0;
            for (size_t i = 0; i < tree.children[u].size(); ++i) {
                int v = tree.children[u][i];
                double c_uv = tree.cap[u][i];
                total += min(c_uv, dp[v]);
            }
            dp[u] = total;
        }
    }

    return dp[1];
}

// ============================================================================
// 3. Вычисление F^(d)_max для всех глубин
// ============================================================================
map<int, double> compute_f_max_by_depth(const Tree& tree, const vector<int>& order) {
    map<int, double> depth_flow;
    set<int> unique_depths;

    // Сбор уникальных глубин листьев
    for (int u = 1; u <= tree.n; ++u) {
        if (tree.children[u].empty()) {
            unique_depths.insert(tree.depth[u]);
        }
    }

    // Для каждой глубины запускаем универсальный DP
    for (int d : unique_depths) {
        depth_flow[d] = bottom_up_dp(tree, order, d);
    }

    return depth_flow;
}

// ============================================================================
// 4. Проверка сбалансированности (Листинг 9)
// ============================================================================
bool check_balanced(const Tree& tree) {
    for (int u = 1; u <= tree.n; ++u) {
        if (u == 1 || tree.children[u].empty()) continue;

        double sum_in = tree.incoming_cap[u];
        double sum_out = 0.0;
        for (size_t i = 0; i < tree.children[u].size(); ++i) {
            sum_out += tree.cap[u][i];
        }

        if (std::abs(sum_in - sum_out) > 1e-9) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// 5. Основной алгоритм (Листинг 10)
// ============================================================================
struct BurstResult {
    double v_burst;
    int t_burst;
    bool has_burst;
};

BurstResult find_burst_magnitude(Tree& tree) {
    BurstResult result;
    vector<int> order;
    bfs_with_depth(tree, order);

    // 1. Вычисление F_max (target_depth = -1)
    double f_max = bottom_up_dp(tree, order, -1);

    // 2. Вычисление F^(d)_max для всех глубин
    auto depth_flow = compute_f_max_by_depth(tree, order);

    // 3. Статистика
    int k = static_cast<int>(depth_flow.size());
    int l_max = depth_flow.empty() ? 0 : depth_flow.rbegin()->first;

    // 4. Сумма пропускных способностей листьев
    double sum_leaf_capacity = 0.0;
    for (int u = 1; u <= tree.n; ++u) {
        if (tree.children[u].empty()) {
            sum_leaf_capacity += tree.incoming_cap[u];
        }
    }

    // 5. Сумма F^(d)_max
    double sum_f_max_by_depth = 0.0;
    for (const auto& pair : depth_flow) {
        sum_f_max_by_depth += pair.second;
    }

    // 6. Проверка условий (Теорема 4)
    bool condition_async = (k > 1);
    bool condition_capacity = (sum_leaf_capacity > f_max);
    bool condition_balanced = !check_balanced(tree);

    result.has_burst = condition_async && condition_capacity && condition_balanced;

    // 7. Результат (Теорема 7)
    if (result.has_burst) {
        result.v_burst = sum_f_max_by_depth;
        result.t_burst = l_max;
    }
    else {
        result.v_burst = f_max;
        result.t_burst = 1;
    }

    return result;
}

// ============================================================================
// 6. Пример использования
// ============================================================================
int main() {
    setlocale(LC_ALL, "Russian");

    // ПРИМЕР: Дерево, где всплеск = 2
    // Структура:
    //      1 (root)
    //      | cap=10
    //      2
    //     / \
    // cap=1  cap=10
    //   /       \
    //   3         4
    //             | cap=1
    //             5
    // Листья: 3 (глубина 2), 5 (глубина 3)

    Tree tree(6);
    tree.add_edge(1, 2, 1.0); // Корень не ограничивает
    tree.add_edge(2, 3, 1.0);  // Лист глубины 2 (поток 1.0)
    tree.add_edge(2, 6, 1.0);  // Лист глубины 2 (поток 1.0)
    tree.add_edge(2, 4, 1.0); // Промежуточная
    tree.add_edge(4, 5, 1.0);  // Лист глубины 3 (поток 1.0)

    // Поиск всплеска
    BurstResult burst = find_burst_magnitude(tree);

    // Вывод
    cout << fixed << setprecision(2);
    cout << "=== Результаты анализа ===" << endl;
    cout << "Наличие всплеска: " << (burst.has_burst ? "Да" : "Нет") << endl;
    cout << "V_burst (величина): " << burst.v_burst << endl;
    cout << "T_burst (момент): " << burst.t_burst << endl;
    cout << endl;

    // Детализация по глубинам
    cout << "=== Вклад по глубинам (F^(d)_max) ===" << endl;
    vector<int> order;
    bfs_with_depth(tree, order);
    auto depth_flow = compute_f_max_by_depth(tree, order);

    double sum_check = 0;
    for (const auto& pair : depth_flow) {
        cout << "Глубина " << pair.first << ": " << pair.second << endl;
        sum_check += pair.second;
    }
    cout << "Сумма: " << sum_check << endl;

    return 0;
}