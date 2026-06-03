#!/usr/bin/env python3
"""
Ferramenta de varredura (sweep) para Q4 — Estudo de escalabilidade.
Executa o binario matrix_multiply com varios numeros de threads,
coleta tempos medios e gera tabela + grafico.

Uso:
    python3 tools/plot.py [--n N] [--runs R] [--threads T1,T2,...]

Exemplo:
    python3 tools/plot.py --n 1200 --runs 6 --threads 1,2,4,6,8,10,12

Requisitos para grafico: matplotlib (`pip install matplotlib`)
Sem matplotlib, gera apenas CSV e tabela em texto.
"""

import subprocess
import sys
import os
import re

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BINARY = os.path.join(PROJECT_DIR, "matrix_multiply")


def run_benchmark(n, num_threads, num_runs):
    cmd = [BINARY, str(n), str(num_threads), str(num_runs)]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if proc.returncode != 0:
        print(f"Erro ao executar {cmd}: {proc.stderr}", file=sys.stderr)
        return None

    output = proc.stdout

    ver_match = re.search(r"VERIFICACAO:\s*(OK|FALHA)", output)
    if not ver_match or ver_match.group(1) != "OK":
        print(f"FALHA na verificacao para T={num_threads}", file=sys.stderr)
        return None

    seq_match = re.search(r"T_seq\s*=\s*([\d.]+)\s*s", output)
    par_match = re.search(r"T_par\s*\(?\s*\d+T?\s*\)?\s*=\s*([\d.]+)\s*s", output)
    spd_match = re.search(r"Speedup\s*=\s*([\d.]+)x", output)
    eff_match = re.search(r"Eficiencia\s*=\s*([\d.]+)", output)

    if not all([seq_match, par_match, spd_match, eff_match]):
        print(f"Erro ao parsear saida para T={num_threads}", file=sys.stderr)
        print(output, file=sys.stderr)
        return None

    return {
        "threads": num_threads,
        "t_seq": float(seq_match.group(1)),
        "t_par": float(par_match.group(1)),
        "speedup": float(spd_match.group(1)),
        "efficiency": float(eff_match.group(1)),
    }


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Sweep de escalabilidade para Q4")
    parser.add_argument("--n", type=int, default=1200,
                        help="Tamanho da matriz n x n (default: 1200)")
    parser.add_argument("--runs", type=int, default=6,
                        help="Repeticoes totais por ponto (default: 6; 1 aquecimento + 5 medicoes)")
    parser.add_argument("--threads", type=str, default="1,2,4,6,8,10,12",
                        help="Lista de threads separada por virgula (default: 1,2,4,6,8,10,12)")
    args = parser.parse_args()

    thread_list = [int(t.strip()) for t in args.threads.split(",")]

    if not os.path.isfile(BINARY):
        print(f"Binario nao encontrado: {BINARY}", file=sys.stderr)
        print("Execute 'make' primeiro.", file=sys.stderr)
        sys.exit(1)

    print(f"=== Sweep de escalabilidade ===")
    print(f"Matriz: {args.n}x{args.n}  |  repeticoes: {args.runs}")
    print(f"Threads: {thread_list}")
    print()

    results = []
    t_seq_ref = None

    for nt in thread_list:
        print(f"  T={nt:3d} ... ", end="", flush=True)
        res = run_benchmark(args.n, nt, args.runs)
        if res is None:
            print("FALHA")
            sys.exit(1)

        if t_seq_ref is None:
            t_seq_ref = res["t_seq"]

        results.append(res)

    # recalcula speedup e eficiencia usando t_seq_ref unificado
    for r in results:
        r["t_seq"] = t_seq_ref
        r["speedup"] = t_seq_ref / r["t_par"] if r["t_par"] > 0 else 0.0
        r["efficiency"] = r["speedup"] / r["threads"]

    for r in results:
        print(f"T_par={r['t_par']:.4f}s  speedup={r['speedup']:.3f}x  "
              f"eficiencia={r['efficiency']:.4f}")

    print()
    print("=== TABELA (para README) ===")
    print(f"| Threads | Tempo (s) | Speedup  | Eficiencia |")
    print(f"|---------|-----------|----------|------------|")
    print(f"| 1 (seq) | {t_seq_ref:9.4f} | {1:8.3f}x | {1:10.4f} |")
    for r in results:
        if r["threads"] == 1:
            continue
        print(f"| {r['threads']:7d} | {r['t_par']:9.4f} | "
              f"{r['speedup']:8.3f}x | {r['efficiency']:10.4f} |")

    csv_path = os.path.join(PROJECT_DIR, "benchmark_results.csv")
    with open(csv_path, "w") as f:
        f.write("threads,tempo,speedup,eficiencia\n")
        f.write(f"1,{t_seq_ref:.6f},1.0000,1.0000\n")
        for r in results:
            if r["threads"] == 1:
                continue
            f.write(f"{r['threads']},{r['t_par']:.6f},{r['speedup']:.4f},{r['efficiency']:.4f}\n")
    print(f"\nResultados salvos em: {csv_path}")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("\nmatplotlib nao instalado. Instale com: pip install matplotlib")
        print("Para gerar o grafico manualmente, use o CSV acima.")
        return

    threads = [r["threads"] for r in results]
    speedups = [r["speedup"] for r in results]
    efficiencies = [r["efficiency"] for r in results]

    fig, ax1 = plt.subplots(figsize=(8, 5))

    color_speedup = "#2196F3"
    ax1.set_xlabel("Numero de Threads")
    ax1.set_ylabel("Speedup", color=color_speedup)
    ax1.plot(threads, speedups, "o-", color=color_speedup, linewidth=2,
             markersize=8, label="Speedup")
    ax1.plot([threads[0], threads[-1]], [threads[0], threads[-1]],
             "--", color="gray", alpha=0.5, label="Speedup ideal (linear)")
    ax1.tick_params(axis="y", labelcolor=color_speedup)

    ax2 = ax1.twinx()
    color_eff = "#4CAF50"
    ax2.set_ylabel("Eficiencia", color=color_eff)
    ax2.plot(threads, efficiencies, "s--", color=color_eff, linewidth=2,
             markersize=8, label="Eficiencia")
    ax2.tick_params(axis="y", labelcolor=color_eff)
    ax2.set_ylim(0, 1.1)

    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc="upper right")

    ax1.set_title(f"Escalabilidade — Multiplicacao de Matrizes {args.n}x{args.n}")

    img_path = os.path.join(PROJECT_DIR, "images", "speedup.png")
    os.makedirs(os.path.dirname(img_path), exist_ok=True)
    plt.savefig(img_path, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"Grafico salvo em: {img_path}")


if __name__ == "__main__":
    main()
