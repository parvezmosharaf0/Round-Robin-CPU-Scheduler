#!/usr/bin/env python3
"""
plot_pareto.py — EnergyAware-RR Pareto Curve Plotter
Run: python3 plot_pareto.py
"""

import csv, os, sys, subprocess


def install_matplotlib():
    attempts = [
        [sys.executable, "-m", "pip", "install", "matplotlib", "--break-system-packages", "-q"],
        [sys.executable, "-m", "pip", "install", "matplotlib", "-q"],
        ["pip3", "install", "matplotlib", "--break-system-packages", "-q"],
        ["pip3", "install", "matplotlib", "-q"],
    ]
    print("  Installing matplotlib...")
    for cmd in attempts:
        try:
            r = subprocess.run(cmd, capture_output=True)
            if r.returncode == 0:
                print("  matplotlib installed successfully.")
                return True
        except FileNotFoundError:
            continue
    return False


try:
    import matplotlib
except ImportError:
    if not install_matplotlib():
        print("\n  ERROR: Could not install matplotlib automatically.")
        print("  Please run manually:")
        print("    sudo apt install python3-matplotlib")
        print("  or:")
        print("    sudo apt install python3-pip && pip3 install matplotlib")
        sys.exit(1)
    import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.ticker as ticker
import numpy as np

csv_path = "data/pareto.csv"
if not os.path.exists(csv_path):
    print(f"  ERROR: {csv_path} not found.")
    print("  Run first: ./energyaware experiment")
    sys.exit(1)

labels, bat_starts, tats, drains, epis = [], [], [], [], []
with open(csv_path) as f:
    for row in csv.DictReader(f):
        labels.append(row['label'])
        bat_starts.append(float(row['bat_start']))
        tats.append(float(row['avg_tat']))
        drains.append(float(row['total_drain']))
        epis.append(float(row['epi']))


def get_color(b):
    if b > 70: return '#2E8B22'
    if b > 30: return '#E08A00'
    return '#CC2222'


def get_mode(b):
    if b > 70: return 'Performance'
    if b > 30: return 'Balanced'
    return 'Survival'


colors = [get_color(b) for b in bat_starts]
modes  = [get_mode(b)  for b in bat_starts]

os.makedirs("data", exist_ok=True)


# Figure 1 — Pareto Scatter
fig, ax = plt.subplots(figsize=(10, 6.5))
fig.patch.set_facecolor('#1E1E2E')
ax.set_facecolor('#1E1E2E')

si = sorted(range(len(tats)), key=lambda i: tats[i])
ax.plot([tats[i] for i in si], [drains[i] for i in si],
        '--', color='#555577', linewidth=1.5, zorder=1)

for i in range(len(tats)):
    ax.scatter(tats[i], drains[i], color=colors[i],
               s=200, zorder=4, edgecolors='white', linewidths=1.5)
    offset_x = 0.15
    offset_y = 0.5 if i % 2 == 0 else -0.9
    ax.annotate(
        f"bat={bat_starts[i]:.0f}%\n({modes[i]})",
        xy=(tats[i], drains[i]),
        xytext=(tats[i] + offset_x, drains[i] + offset_y),
        fontsize=8.5, color=colors[i], fontweight='bold',
        bbox=dict(boxstyle='round,pad=0.2', facecolor='#2A2A3E',
                  edgecolor=colors[i], alpha=0.85),
    )

for i in range(len(drains)):
    ax.annotate(f"{drains[i]:.1f}%",
                xy=(tats[i], drains[i]),
                xytext=(-28, 0),
                textcoords='offset points',
                fontsize=7.5, color='#AAAACC', va='center')

ax.annotate('', xy=(min(tats)-0.3, min(drains)),
            xytext=(min(tats)+0.5, min(drains)),
            arrowprops=dict(arrowstyle='<-', color='#CC2222', lw=1.5))
ax.text(min(tats)-0.3, min(drains)-0.8,
        'Less drain\n(saves battery)', color='#CC2222', fontsize=8, ha='center')

ax.annotate('', xy=(max(tats), max(drains)+0.5),
            xytext=(max(tats), max(drains)+2),
            arrowprops=dict(arrowstyle='<-', color='#2E8B22', lw=1.5))
ax.text(max(tats), max(drains)+2.5,
        'Higher drain\n(faster CPU)', color='#2E8B22', fontsize=8, ha='center')

perf = mpatches.Patch(color='#2E8B22', label='Performance mode (bat > 70%)')
bal  = mpatches.Patch(color='#E08A00', label='Balanced mode   (30-70%)')
surv = mpatches.Patch(color='#CC2222', label='Survival mode   (bat < 30%)')
ax.legend(handles=[perf, bal, surv], loc='upper left',
          fontsize=9, facecolor='#2A2A3E', edgecolor='#555577',
          labelcolor='white', framealpha=0.9)

ax.set_xlabel('Average Turnaround Time (ms)', fontsize=11, color='#CCCCDD', labelpad=10)
ax.set_ylabel('Total Battery Drain (%)',       fontsize=11, color='#CCCCDD', labelpad=10)
ax.set_title('EnergyAware-RR — Pareto Frontier\nTurnaround Time  vs.  Battery Drain',
             fontsize=13, fontweight='bold', color='white', pad=14)
ax.tick_params(colors='#AAAACC')
ax.grid(True, linestyle='--', alpha=0.2, color='#666688')
for spine in ax.spines.values():
    spine.set_edgecolor('#444466')

plt.tight_layout()
o1 = "data/pareto_curve.png"
plt.savefig(o1, dpi=150, bbox_inches='tight', facecolor='#1E1E2E')
plt.close()
print(f"  [1] Pareto scatter  -> {o1}")


# Figure 2 — Bar charts: Drain + EPI
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5.5))
fig.patch.set_facecolor('#1E1E2E')
short = [f"{b:.0f}%" for b in bat_starts]

for ax in (ax1, ax2):
    ax.set_facecolor('#252535')
    ax.tick_params(colors='#CCCCDD')
    ax.xaxis.label.set_color('#CCCCDD')
    ax.yaxis.label.set_color('#CCCCDD')
    ax.title.set_color('white')
    ax.grid(axis='y', linestyle='--', alpha=0.2, color='#666688')
    for spine in ax.spines.values():
        spine.set_edgecolor('#444466')

bars1 = ax1.bar(short, drains, color=colors, edgecolor='#1E1E2E', linewidth=1, width=0.6)
for bar, val in zip(bars1, drains):
    ax1.text(bar.get_x()+bar.get_width()/2, bar.get_height()+0.3,
             f'{val:.1f}%', ha='center', va='bottom', fontsize=9,
             fontweight='bold', color='white')
ax1.axvline(1.5, color='#2E8B22', ls=':', lw=1.2, alpha=0.7)
ax1.axvline(3.5, color='#E08A00', ls=':', lw=1.2, alpha=0.7)
ax1.text(0.5, max(drains)*0.92, 'Perf',     color='#2E8B22', fontsize=8, ha='center')
ax1.text(2.5, max(drains)*0.92, 'Balanced', color='#E08A00', fontsize=8, ha='center')
ax1.text(5.0, max(drains)*0.92, 'Survival', color='#CC2222', fontsize=8, ha='center')
ax1.set_xlabel('Battery Start Level', fontsize=10)
ax1.set_ylabel('Total Battery Drain (%)', fontsize=10)
ax1.set_title('Battery Drain per Configuration', fontsize=11, fontweight='bold')

bars2 = ax2.bar(short, epis, color=colors, edgecolor='#1E1E2E', linewidth=1, width=0.6)
for bar, val in zip(bars2, epis):
    ax2.text(bar.get_x()+bar.get_width()/2, bar.get_height()+0.003,
             f'{val:.3f}', ha='center', va='bottom', fontsize=9,
             fontweight='bold', color='white')
ax2.axvline(1.5, color='#2E8B22', ls=':', lw=1.2, alpha=0.7)
ax2.axvline(3.5, color='#E08A00', ls=':', lw=1.2, alpha=0.7)
ax2.set_xlabel('Battery Start Level', fontsize=10)
ax2.set_ylabel('EPI (drain% per ms burst)', fontsize=10)
ax2.set_title('Energy Per Instruction (EPI)\n(Lower = More Efficient)', fontsize=11, fontweight='bold')

fig.suptitle('EnergyAware-RR — Quantitative Analysis', fontsize=13,
             fontweight='bold', color='white', y=1.01)
plt.tight_layout()
o2 = "data/analysis_charts.png"
plt.savefig(o2, dpi=150, bbox_inches='tight', facecolor='#1E1E2E')
plt.close()
print(f"  [2] Analysis charts -> {o2}")


# Figure 3 — TAT vs EPI dual-axis line
fig, ax1 = plt.subplots(figsize=(10, 5.5))
fig.patch.set_facecolor('#1E1E2E')
ax1.set_facecolor('#252535')
ax2t = ax1.twinx()
ax2t.set_facecolor('#252535')

x = np.arange(len(short))
l1, = ax1.plot(x, tats,  'o-',  color='#5599FF', lw=2.2, ms=9, label='Avg TAT (ms)',  zorder=3)
l2, = ax2t.plot(x, epis, 's--', color='#FF6655', lw=2.2, ms=9, label='EPI',           zorder=3)

ax1.axvspan(-0.5, 1.5, alpha=0.08, color='#2E8B22')
ax1.axvspan(1.5,  3.5, alpha=0.08, color='#E08A00')
ax1.axvspan(3.5,  6.5, alpha=0.08, color='#CC2222')
ax1.text(0.5,  max(tats)+0.05, 'Performance', color='#4ABB44', fontsize=8, ha='center')
ax1.text(2.5,  max(tats)+0.05, 'Balanced',    color='#FFAA33', fontsize=8, ha='center')
ax1.text(5.0,  max(tats)+0.05, 'Survival',    color='#FF6655', fontsize=8, ha='center')

ax1.set_xticks(x)
ax1.set_xticklabels(short, color='#CCCCDD')
ax1.set_xlabel('Battery Start Level', fontsize=11, color='#CCCCDD', labelpad=8)
ax1.set_ylabel('Average TAT (ms)',    fontsize=11, color='#5599FF', labelpad=8)
ax2t.set_ylabel('EPI (drain%/ms)',    fontsize=11, color='#FF6655', labelpad=8)
ax1.tick_params(axis='y', colors='#5599FF')
ax2t.tick_params(axis='y', colors='#FF6655')
ax1.tick_params(axis='x', colors='#CCCCDD')
ax1.grid(axis='y', ls='--', alpha=0.2, color='#666688')
ax1.set_title('TAT vs EPI Trade-off across Battery Levels',
              fontsize=13, fontweight='bold', color='white', pad=12)
for spine in ax1.spines.values():  spine.set_edgecolor('#444466')
for spine in ax2t.spines.values(): spine.set_edgecolor('#444466')

ax1.legend([l1, l2], ['Avg TAT (ms)', 'EPI'], loc='upper right',
           facecolor='#2A2A3E', edgecolor='#555577', labelcolor='white',
           fontsize=9, framealpha=0.9)

plt.tight_layout()
o3 = "data/tat_vs_epi.png"
plt.savefig(o3, dpi=150, bbox_inches='tight', facecolor='#1E1E2E')
plt.close()
print(f"  [3] TAT vs EPI     -> {o3}")

print("\n  All 3 graphs generated in data/ folder.\n")
