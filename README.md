# EnergyAware-RR

**Battery-State-Driven Round Robin CPU Scheduler Simulator**

A **C-based CPU scheduling simulator** that extends the traditional Round Robin algorithm with battery-aware scheduling and Dynamic Voltage and Frequency Scaling (DVFS). The project also generates Pareto trade-off graphs using Python for performance and energy analysis.

---

## Features

- Round Robin CPU Scheduling
- Battery-State-Driven Scheduling
- Dynamic Voltage and Frequency Scaling (DVFS)
- Performance, Balanced and Survival Modes
- Waiting Time and Turnaround Time Analysis
- Energy Consumption Analysis
- Pareto Trade-off Graph Generation (Python)

---

## Project Structure

```text
├── src/
├── include/
├── data/
├── Makefile
├── energyaware
├── plot_pareto.py
└── README.md
```

---

## Run

```bash
make
./energyaware
python3 plot_pareto.py
```

---

## Output

- Waiting Time
- Turnaround Time
- Energy Consumption
- Battery Status
- Pareto Trade-off Graphs

---

## Author

**Parvez Mosharaf**

Department of Computer Science and Engineering

Rangamati Science and Technology University
