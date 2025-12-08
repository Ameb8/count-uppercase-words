#!/usr/bin/env python3

from pathlib import Path
from math import ceil
import subprocess
import argparse
import statistics
from typing import Optional

import matplotlib.pyplot as plt


# Path to executable script
EXEC_PATH: Path = Path(__file__).parent.parent / "run.sh"
EXEC_VERSION: str = "opt"

# Range of number of processes to benchmark
MIN_PROCS: int = 2 # Inclusive
MAX_PROCS: int = 11 # Exclusive

NUM_RUNS = 10

# Path to compilation target directory
BIN_DIR: Path = Path(__file__).parent.parent / "bin"

# Path to directory to store benchmark plots
PLOT_DIR: Path = Path(__file__).parent.parent / "plots"


def run_benchmark(num_processes: int, file_name: str) -> float:
    # Execute program as subprocess
    result = subprocess.run(
        [str(EXEC_PATH), file_name, num_processes, EXEC_VERSION],
        capture_output=True,
        text=True
    )

    # Return last word from program execution stdout as wall-clock benchmark time
    return float(result.stdout.split(' ')[-1])

def avg_benchmark(num_procs: int, num_runs: int, file_name: str) -> float:
    """
    Computes average wall-clock runtime with num_procs processors
    Average is calculated over num_runs iterations
    """
    return statistics.mean([run_benchmark(num_procs, file_name) for i in range(0, num_runs)])

def run_benchmarks(min_procs: int, max_procs: int, num_runs: int, file_name: str) -> list[float]:
    """
    Benchmarks average execution time for given range of processors
    Each processor result is averaged over num_runs executions
    """
    return [avg_benchmark(p, num_runs, file_name) for p in range(min_procs, max_procs)]


def run_bench(src_path: Path, processes: list[int], prgm_args: list[str], mpicc_args) -> tuple[list[float], list[float]]:
    # Define path to executable target
    target_path: Path = BIN_DIR / src_path.stem

    # Compile Program
    if not mpicc_compile(src_path, target_path, mpicc_args):
        return ([], []) # Compilation failed
    
    avg: list[float] = [] # Holds result of each run with p for averaging

    for p in processes: # Return list of benchmark results for each process amount
        # Benchmark average runtime
        runtimes: list[float] = [mpicc_benchmark(target_path, p, prgm_args) for _ in range(5)]
        avg.append(statistics.mean(runtimes)) # Take average of 5 runs

    return (avg, basic_pred_runtime(processes, avg[0]))


def plot_benchmark(
    processes: list[int], 
    exec_times: list[float], 
    prgm_name: str, 
    theoretical_times: Optional[list[float]] = None
) -> None:
    plot_path: Path = PLOT_DIR / f'{prgm_name}_benchmark_results.png'
    plt.plot(processes, exec_times, marker='o', label='Measured Runtime')

    if theoretical_times:
        plt.plot(processes, theoretical_times, marker='x', label='Theoretical Runtime')

    plt.xlabel('Average Execution Time (s)')
    plt.ylabel('Number of Processes')

    plt.title(f'MPI Benchmark of {prgm_name}')
    plt.grid(True)
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')


def plot_benchmark_multi(results: dict, processes: list[int]):
    plt.figure()

    for input_file, exec_times in results.items():
        plt.plot(processes, exec_times, marker='o', label=f'{input_file}')

    plt.xlabel('Number of Processes')
    plt.ylabel('Average Execution Time (s)')
    plt.title('Counting Title-Cased Words in Parallel')
    plt.grid(True)
    plt.legend()

    plot_path = PLOT_DIR / "combined_benchmark_results.png"
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')
    print(f"Saved plot to {plot_path}")

        
def main():
    # Create parse for program arguments
    parser: argparse.ArgumentParser = argparse.ArgumentParser(
        description="CLI for Benchmarking count-title-words MPI program"
    )

    # Define program arguments
    parser.add_argument(
        'input_files', 
        nargs='+', 
        help='Path to input text files'
    )

    parser.add_argument( # Flags for program execution
        '-rs', '--read-size' 
        nargs=1, 
        help='Size of chunks file is read in (Bytes)'
    )

    args: argparse.Namespace = parser.parse_args() # Parse arguments

    # Set read chunk size


    # Run benchmarks
    benchmark_results: list[list[float]] = [run_benchmarks(MIN_PROCS, MAX_PROCS, NUM_RUNS, file) for file in args.input_files]
    
    results: dict[str, list[float]] = {}

    for file in args.input_files: # Populate map with benchmark results
        results[file] = run_benchmarks(MIN_PROCS, MAX_PROCS, NUM_RUNS, file)

    # Create plot
    plot_benchmark_multi(results, range(MIN_PROCS, MAX_PROCS))

if __name__ == "__main__":
    main()
