"""Графики по результатам бенчмарка ЛР-2 (блок 7).

Использование:
    python scripts/plot_benchmark.py benchmark.csv [папка_для_картинок]

Для каждого атрибута строит три картинки (build / find / range): по горизонтали n,
по вертикали время, по одной панели на порядок входных данных (random / sorted / reversed),
по одной линии на структуру. Нужен только matplotlib:  pip install matplotlib
python scripts/plot_benchmark.py cmake-build-release/benchmark.csv plots
"""
import csv
import os
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")  # без окна - сразу в файлы
import matplotlib.pyplot as plt

ORDERS = ["random", "sorted", "reversed"]
METRICS = [
    ("build_ms", "Построение индекса, мс"),
    ("find_us", "Точный поиск, мкс на запрос"),
    ("range_us", "Поиск по диапазону, мкс на запрос"),
]


def load(path):
    # data[attribute][metric][order][structure] -> список (n, value)
    data = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(list))))
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            if row["skipped"] == "1":
                continue
            for metric, _ in METRICS:
                if metric == "build_ms" and row["structure"] == "Linear":
                    continue  # у перебора нет построения
                data[row["attribute"]][metric][row["order"]][row["structure"]].append(
                    (int(row["n"]), float(row[metric])))
    return data


def safe_name(text):
    return "".join(c if c.isalnum() else "_" for c in text).strip("_")


def plot(data, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    saved = []
    for attribute, metrics in data.items():
        for metric, title in METRICS:
            fig, axes = plt.subplots(1, 3, figsize=(15, 4.5), sharey=True)
            for ax, order in zip(axes, ORDERS):
                for structure, points in sorted(metrics[metric][order].items()):
                    points.sort()
                    ax.plot([p[0] for p in points], [p[1] for p in points], marker="o", label=structure)
                ax.set_title(order)
                ax.set_xlabel("n")
                ax.set_yscale("log")
                ax.grid(True, which="both", alpha=0.3)
            axes[0].set_ylabel(title)
            axes[-1].legend()
            fig.suptitle(f"{attribute}: {title}")
            fig.tight_layout()
            path = os.path.join(out_dir, f"{safe_name(attribute)}_{metric}.png")
            fig.savefig(path, dpi=120)
            plt.close(fig)
            saved.append(path)
    return saved


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    csv_path = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(csv_path)[0] + "_plots"
    for path in plot(load(csv_path), out_dir):
        print("saved", path)


if __name__ == "__main__":
    main()
