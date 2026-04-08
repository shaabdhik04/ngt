"""
Plot all 10 D2D Content Offloading simulation results.

Usage:  python plot_results.py
        (reads CSV files from results/ and saves PNG plots to results/)
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')           # non-interactive backend
import matplotlib.pyplot as plt

plt.rcParams.update({
    'figure.figsize': (8, 5),
    'axes.grid': True,
    'grid.alpha': 0.3,
    'font.size': 11,
    'lines.linewidth': 2,
    'lines.markersize': 6,
})

RESULTS = 'results'


def load(name):
    return np.genfromtxt(os.path.join(RESULTS, name), delimiter=',',
                         names=True, dtype=None, encoding='utf-8')


# ── Plot 1 ──────────────────────────────────────────────────────────────────
def plot1():
    d = load('plot1_backhaul_load.csv')
    fig, ax = plt.subplots()
    ax.plot(d['ClusterSize'], d['BackhaulReduction_percent'],
            marker='o', color='#2563eb')
    ax.set_xlabel('Cluster Size (number of UEs)')
    ax.set_ylabel('Backhaul Load Reduction (%)')
    ax.set_title('Plot 1: Backhaul Load Reduction vs. Cluster Size')
    ax.set_ylim(0, 100)
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot1_backhaul_load.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 1')


# ── Plot 2 ──────────────────────────────────────────────────────────────────
def plot2():
    d = load('plot2_multicast_throughput.csv')
    fig, ax = plt.subplots()
    ax.plot(d['GroupDiameter_m'], d['Throughput_Mbps'],
            marker='s', color='#dc2626')
    ax.set_xlabel('Group Diameter (m)')
    ax.set_ylabel('Multicast Throughput (Mbps)')
    ax.set_title('Plot 2: Multicast Throughput vs. Group Diameter')
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot2_multicast_throughput.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 2')


# ── Plot 3 ──────────────────────────────────────────────────────────────────
def plot3():
    d = load('plot3_pdr_distance.csv')
    fig, ax = plt.subplots()
    ax.plot(d['Distance_m'], d['PDR'], marker='^', color='#16a34a')
    ax.set_xlabel('Distance from Cluster Head (m)')
    ax.set_ylabel('Packet Delivery Ratio (PDR)')
    ax.set_title('Plot 3: PDR vs. Distance from Cluster Head')
    ax.set_ylim(0, 1.05)
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot3_pdr_distance.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 3')


# ── Plot 4 ──────────────────────────────────────────────────────────────────
def plot4():
    d = load('plot4_completion_time.csv')
    fig, ax = plt.subplots()
    ax.plot(d['NumRetransmissions'], d['CompletionTime_s'],
            marker='D', color='#9333ea')
    ax.set_xlabel('Number of Re-transmissions')
    ax.set_ylabel('Download Completion Time (s)')
    ax.set_title('Plot 4: Download Completion Time vs. Re-transmissions')
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot4_completion_time.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 4')


# ── Plot 5 ──────────────────────────────────────────────────────────────────
def plot5():
    d = load('plot5_energy_saved.csv')
    fig, ax = plt.subplots()
    ax.plot(d['D2D_Traffic_Percent'], d['EnergySaved_Percent'],
            marker='o', color='#ea580c')
    ax.set_xlabel('D2D Traffic Volume (%)')
    ax.set_ylabel('Energy Saved at gNB (%)')
    ax.set_title('Plot 5: Energy Saved at gNB vs. D2D Traffic Volume')
    ax.set_ylim(0, None)
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot5_energy_saved.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 5')


# ── Plot 6 ──────────────────────────────────────────────────────────────────
def plot6():
    d = load('plot6_sinr_feedback.csv')
    fig, ax = plt.subplots()
    ax.plot(d['FeedbackRate_per_s'], d['SINR_dB'],
            marker='v', color='#0891b2')
    ax.set_xlabel('Multicast Feedback Rate (feedback/s)')
    ax.set_ylabel('SINR (dB)')
    ax.set_title('Plot 6: SINR vs. Multicast Feedback Rate')
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot6_sinr_feedback.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 6')


# ── Plot 7 ──────────────────────────────────────────────────────────────────
def plot7():
    d = load('plot7_fairness_position.csv')
    fig, ax1 = plt.subplots()
    ax1.bar(d['NormalizedPosition'], d['Throughput_Mbps'],
            width=0.05, color='#2563eb', alpha=0.7, label='Throughput')
    ax1.set_xlabel('Normalized User Position (0 = centre, 1 = edge)')
    ax1.set_ylabel('Throughput (Mbps)', color='#2563eb')

    ax2 = ax1.twinx()
    fi = d['FairnessIndex'][0]
    ax2.axhline(fi, color='#dc2626', linestyle='--', linewidth=2,
                label=f"Jain's Fairness = {fi:.3f}")
    ax2.set_ylabel("Jain's Fairness Index", color='#dc2626')
    ax2.set_ylim(0, 1.05)

    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper right')
    ax1.set_title('Plot 7: Fairness & Throughput vs. User Position')
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot7_fairness_position.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 7')


# ── Plot 8 ──────────────────────────────────────────────────────────────────
def plot8():
    d = load('plot8_throughput_bw.csv')
    fig, ax = plt.subplots()
    ax.plot(d['SidelinkBW_MHz'], d['Throughput_Mbps'],
            marker='s', color='#059669')
    ax.set_xlabel('Sidelink Bandwidth (MHz)')
    ax.set_ylabel('Throughput (Mbps)')
    ax.set_title('Plot 8: Throughput vs. Sidelink Bandwidth')
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot8_throughput_bw.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 8')


# ── Plot 9 ──────────────────────────────────────────────────────────────────
def plot9():
    d = load('plot9_blockage_throughput.csv')
    fig, ax = plt.subplots()
    ax.plot(d['BlockageProbability'], d['ClusterThroughput_Mbps'],
            marker='X', color='#b91c1c')
    ax.set_xlabel('Human Blockage Probability')
    ax.set_ylabel('Cluster Throughput (Mbps)')
    ax.set_title('Plot 9: Impact of Human Blockage on Cluster Throughput')
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot9_blockage_throughput.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 9')


# ── Plot 10 ─────────────────────────────────────────────────────────────────
def plot10():
    d = load('plot10_buffer_delay.csv')
    fig, ax = plt.subplots()
    ax.plot(d['PopularityRank'], d['BufferDelay_ms'],
            marker='o', color='#7c3aed')
    ax.axvline(x=10, color='gray', linestyle=':', label='Cache boundary')
    ax.set_xlabel('Content Popularity Rank (1 = most popular)')
    ax.set_ylabel('Buffer Delay (ms)')
    ax.set_title('Plot 10: Buffer Delay vs. Content Popularity')
    ax.legend()
    fig.tight_layout()
    fig.savefig(os.path.join(RESULTS, 'plot10_buffer_delay.png'), dpi=150)
    plt.close(fig)
    print('  [OK] Plot 10')


# ── Main ────────────────────────────────────────────────────────────────────
if __name__ == '__main__':
    print('Generating plots from simulation data…\n')
    plot1()
    plot2()
    plot3()
    plot4()
    plot5()
    plot6()
    plot7()
    plot8()
    plot9()
    plot10()
    print('\nAll 10 plots saved as PNG in', RESULTS + '/')
