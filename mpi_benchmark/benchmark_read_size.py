#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys
import re
import argparse
import statistics
from datetime import datetime
from typing import Optional
import matplotlib.pyplot as plt


# Path to executable script
EXEC_PATH: Path = Path(__file__).parent.parent / "run.sh"
EXEC_VERSION: str = "opt"

# Range of number of processes to benchmark
MIN_PROCS: int = 2 # Inclusive
MAX_PROCS: int = 12 # Exclusive

MIN_KBC = 1
MAX_KBC = 16

NUM_RUNS: int = 10 # Number of runs used to average benchmark

# Path to directory to store benchmark plots
PLOT_DIR: Path = Path(__file__).parent.parent / "plots"

def set_read_size(new_value: int):
    script_dir: Path = Path(__file__).resolve().parent
    config_path: Path = (script_dir / "../include/config.h").resolve()

    if not config_path.exists():
        raise FileNotFoundError(f"Config file not found: {config_path}")

    text = config_path.read_text()

    # Convert KB → Bytes
    byte_value = new_value * 1024

    pattern = r"(#define\s+MAX_CHUNK_SIZE\s+)(\d+)"
    replacement = rf"\g<1>{byte_value}"

    if re.search(pattern, text):
        new_text = re.sub(pattern, replacement, text)
    else:
        new_text = text.rstrip() + f"\n#define MAX_CHUNK_SIZE {byte_value}\n"

    config_path.write_text(new_text)
    print(f"Updated MAX_CHUNK_SIZE to {byte_value} bytes (from {new_value} KB) in {config_path}")


def run_benchmark(num_processes: int, file_name: str) -> float:
    exec_cmd: list[str] = [str(EXEC_PATH), str(num_processes), file_name, EXEC_VERSION]

    # Execute program as subprocess
    result = subprocess.run(
        exec_cmd,
        capture_output=True,
        text=True
    )

    # Split stdout into tokens and iterate backwards
    tokens = result.stdout.split()
    wall_time: Optional[float] = None
    for token in reversed(tokens):
        try:
            wall_time = float(token)
            break
        except ValueError:
            continue

    # DEBUG ******
    #print(f"last Token stdout: {result.stdout.split(' ')[-1]}")
    # Take last word in stdout as wall-clock benchmark time
    #wall_time: float = float(result.stdout.strip().split(' ')[-1])
    if wall_time: 
        print(f"Executing with {' '.join(exec_cmd)}:\t\tResult: {wall_time}")
    else: 
        print(f"Executing with {' '.join(exec_cmd)}:\t\tResult: No Float found in output")

    if result.stderr:
        print(f"Error: {result.stderr}")
    
    # Return last word from program execution stdout as wall-clock benchmark time
    return wall_time


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



def plot_benchmark_multi(results: dict, processes: list[int]):
    plt.figure()

    for input_file, exec_times in results.items():
        plt.plot(processes, exec_times, marker='o', label=input_file.split('/')[-1])

    plt.xlabel('Number of Processes')
    plt.ylabel('Average Execution Time (s)')
    plt.title('Counting Title-Cased Words in Parallel')
    plt.grid(True)
    plt.legend()

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    plot_path = PLOT_DIR / f'benchmark_results_{timestamp}.png'
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
        '-rs', 
        '--read-size',
        nargs=1, 
        help='Size of chunks file is read in (Bytes)'
    )

    args: argparse.Namespace = parser.parse_args() # Parse arguments

    # Set read chunk size


    # Run benchmarks
    # benchmark_results: list[list[float]] = [run_benchmarks(MIN_PROCS, MAX_PROCS, NUM_RUNS, file) for file in args.input_files]
    
    results: dict[str, list[float]] = {}
    read_sizes_kb: list[int] = [256, 128, 64, 32, 16]

    for size in read_sizes_kb: # Populate map with benchmark results
        set_read_size(size);
        results[f"{size} KB"] = run_benchmarks(MIN_PROCS, MAX_PROCS, NUM_RUNS, args.input_files[0])

    # Create plot
    plot_benchmark_multi(results, range(MIN_PROCS, MAX_PROCS))

if __name__ == "__main__":
    main()
