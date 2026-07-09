# Makefile — EnergyAware-RR Scheduler
# Usage:
#   make              - build
#   make run          - build + run with sample workload
#   make custom       - build + run with custom input
#   make experiment   - build + run Pareto experiment
#   make clean        - remove build artifacts

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
TARGET  = energyaware
SRC     = src/main.c src/process.c src/battery.c \
          src/dvfs.c src/scheduler.c src/metrics.c src/logger.c

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p data
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -lm
	@echo "Build successful -> ./$(TARGET)"

run: all
	./$(TARGET)

custom: all
	./$(TARGET) custom

experiment: all
	./$(TARGET) experiment

clean:
	rm -f $(TARGET)
	rm -f data/schedule_log.csv

.PHONY: all run custom experiment clean
