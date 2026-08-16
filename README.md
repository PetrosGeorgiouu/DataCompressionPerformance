# Huffman Data Compression Lab
A data compression library for compressing/decompressing text files. Compares a naive implementation to a performance enhanced implementation.

# How to run

For now, the goal is to develop a basic implementation that establishes correct functionality. The `main.cpp` program processes a fixed set of `.txt` files whose paths are currently hard-coded for correctness and performance testing.

Build the executable and create the directory used to store it:

```bash
make build
```
Here's how to use the compressor.

```bash
make compress \
  INPUT= [input file path] \
  OUTPUT= [output file path]
```
This takes a txt file from the input file path, compresses it, and stores it in the output file path. Note: both must exist for the compressor to succeed.

You can also run its test cases with:

```bash
make tests
```
After each implementation improvement, I use

```bash
make profile BENCH_ARGS="--benchmark_filter=BM_CompressFile --benchmark_repetitions=30"
```
to run the compressor 30 times on "data/corpus/complete_project_gutenberg_works_of_george_meredith.txt" to measure performance. We will discuss this later.

After testing, remove the generated build directory and its contents:

```bash
make clean
```

# Performance Analysis

I'd also like to take some time to explain some of my optimizations and the actual statistics as to how my new model compares to the naive model.

I want to specify the original machine specification I used to measure performance in my original implementation. I used a Ubuntu Linux Virtual Machine.

```text
Environment:
  Ubuntu: Ubuntu 24.04.4 LTS running inside UTM
  Linux kernel: 6.8.0-136-generic
  Architecture: x86_64
  Virtualization: apple, as reported by systemd-detect-virt

CPU:
  Guest-visible CPU model: Intel(R) Core(TM) i5-1038NG7 CPU @ 2.00GHz
  Virtual CPUs: 4
  Physical cores visible to guest: 4
  Threads per core: 1
  L1d cache: 192 KiB (4 instances)
  L1i cache: 128 KiB (4 instances)
  L2 cache: 2 MiB (4 instances)
  L3 cache: 6 MiB (1 instance)

Memory:
  Configured VM memory: 4 GB
  Guest-visible memory: 3.8 GiB

Toolchain:
  Compiler: Ubuntu clang version 18.1.3 (1ubuntu1)
  C++ standard: C++17
  Standard library: libstdc++
  Build mode: Release
  Compiler flags: -std=c++17 -O2 -Wall -Wextra -Wpedantic
```

You can reference this as

| Component | Value |
|---|---:|
| CPU cores | 4 |
| CPU frequency | ~1996.65 MHz |
| L1 Data Cache | 48 KiB × 4 |
| L1 Instruction Cache | 32 KiB × 4 |
| L2 Cache | 512 KiB × 4 |
| L3 Cache | 6144 KiB shared |

### Baseline Results

First, let's discuss a constant baseline template for how we will measure if performance actually improved. I use the make command listed above to calculate the latencies on each run.

This is our official, first implementation report on the latencies found.

| Metric | Result |
|---|---:|
| Median wall-clock latency | **803.674 ms** |
| Mean wall-clock latency | **801.442 ms** |
| Best observed latency | **778.999 ms** |
| Standard deviation | **9.525 ms** |
| Coefficient of variation | **1.19%** |
| Median CPU time | **801.668 ms** |
| Mean CPU time | **799.470 ms** |
| Median throughput | **17.750 MiB/s** |
| Mean throughput | **17.802 MiB/s** |
| Peak observed throughput | **18.312 MiB/s** |

## Acknowledgments

Early versions of my Huffman Coding file were inspired by a C++ implementation done by GeeksforGeeks (https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/?_x_tr_hist=true); I also used their tutorial to read from files using C++ (https://www.w3schools.com/cpp/cpp_files.asp).
