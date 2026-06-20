import matplotlib.pyplot as plt
import numpy as np

def parse_results(filename):
    ns, ks, times_base, times_opt = [], [], [], []
    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            if line.startswith('N') or line.startswith('-') or not line.strip():
                continue
            parts = line.split()
            if len(parts) >= 4:
                ns.append(int(parts[0]))
                ks.append(int(parts[1]))
                times_base.append(float(parts[2]))
                times_opt.append(float(parts[3]))
    return np.array(ns), np.array(ks), np.array(times_base), np.array(times_opt)

def plot_graphs(ns, ks, times_base, times_opt):
    n_safe = ns + 1 
    
    # Вычисляем нормализованное время для строгой проверки асимптотики
    norm_base = times_base / (n_safe ** 2)
    norm_opt = times_opt / (n_safe * np.log2(n_safe))
    
    # Масштабируем теоретические кривые под реальные данные для Графика 1
    if len(times_base) > 1:
        scale_base = times_base[-1] / (ns[-1] ** 2)
        curve_n2 = (ns ** 2) * scale_base
        
        scale_opt = times_opt[-1] / (ns[-1] * np.log2(ns[-1]))
        curve_nlogn = (ns * np.log2(ns)) * scale_opt

    plt.style.use('seaborn-v0_8-whitegrid')
    fig, axs = plt.subplots(2, 1, figsize=(10, 12))

    # =================== ГРАФИК 1: Абсолютное время ===================
    axs[0].plot(ns, times_base, 'o-', color='#E63946', linewidth=2.5, markersize=7, label='Базовый алгоритм $O(k \cdot n)$')
    axs[0].plot(ns, times_opt, 's-', color='#457B9D', linewidth=2.5, markersize=7, label='Оптимизированный $O(n \log n)$')
    
    if len(times_base) > 1:
        axs[0].plot(ns, curve_n2, '--', color='#E63946', alpha=0.4, linewidth=2, label='Аппроксимация $c \cdot N^2$')
        axs[0].plot(ns, curve_nlogn, '--', color='#457B9D', alpha=0.4, linewidth=2, label='Аппроксимация $c \cdot N \log N$')

    axs[0].set_xlabel('Количество вершин (N)', fontsize=13)
    axs[0].set_ylabel('Время работы (мс)', fontsize=13)
    axs[0].set_title('Сравнение времени работы алгоритмов поиска всплеска', fontsize=15, fontweight='bold')
    axs[0].legend(fontsize=12)
    axs[0].ticklabel_format(style='plain', axis='x')

    # Добавляем ось X сверху для показа k
    ax2 = axs[0].twiny()
    ax2.set_xlim(axs[0].get_xlim())
    ax2.set_xticks(ns)
    ax2.set_xticklabels(ks, fontsize=9, color='gray')
    ax2.set_xlabel('Кол-во уникальных глубин (k)', fontsize=12, color='gray')

    # =================== ГРАФИК 2: Верификация асимптотики ===================
    axs[1].plot(ns, norm_base, 'o-', color='#E63946', linewidth=2.5, markersize=7, label='Базовый: $T_{баз} / N^2$')
    axs[1].plot(ns, norm_opt, 's-', color='#457B9D', linewidth=2.5, markersize=7, label='Оптимизированный: $T_{опт} / N \log N$')

    axs[1].set_xlabel('Количество вершин (N)', fontsize=13)
    axs[1].set_ylabel('Нормализованное время', fontsize=13)
    axs[1].set_title('Верификация асимптотических оценок (стремление к константе)', fontsize=15, fontweight='bold')
    axs[1].legend(fontsize=12)
    axs[1].ticklabel_format(style='plain', axis='x')

    plt.tight_layout()
    plt.savefig('burst_complexity_analysis.png', dpi=300)
    print("Графики успешно сохранены в файл 'burst_complexity_analysis.png'!")

if __name__ == '__main__':
    filename = 'experiment_results.txt'
    try:
        ns, ks, times_base, times_opt = parse_results(filename)
        plot_graphs(ns, ks, times_base, times_opt)
    except FileNotFoundError:
        print(f"Ошибка: Файл '{filename}' не найден. Сначала запустите C++ программу.")