# Huffman Data Compression Lab
A data compression library for compressing/decompressing text files. Compares a naive implementation to a performance enhanced implementation.

# How to run

For now, the goal is to develop a basic implementation that establishes correct functionality. The `main.cpp` program processes a fixed set of `.txt` files whose paths are currently hard-coded for correctness and performance testing.

Build the executable and create the directory used to store it:

```bash
make build
```

Run the executable:

```bash
make run
```

The program reports the execution time and corresponding throughput for each stage.

You can also run its test cases with:

```bash
make tests
```

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

First, let's discuss a constant baseline template for how we will measure if performance actually improved. I use the make command

```bash
make fileperf     FILE=data/corpus/complete_project_gutenberg_works_of_george_meredith.txt     BENCH_ARGS="--benchmark_repetitions=30 --benchmark_report_aggregates_only=true"~
```

We use Linux perf tools to simulate over 2000 CPU clock samples, and an obvious bottleneck occurred in our naive implementation of frequency counting. It consumed 95.1% of the sampled CPU time. In particular, we notice these major performance issues.

- 32.1% of that was contributed to a hotspot that occurred due to the per byte extraction obtained using ```std::istream::get```, meaning that reading one character at a time became extremely expensive.
- 12.78% of consumption was attributed to ```std::istream::sentry``` every time we used the ```get()``` call.
- Some noticeable issues are that worth observing are those concerning caches. In particular, using an ```unordered_map<char, uint64_t>``` data structure for storing frequencies is not cache friendly, or trivial to compute. We take a ```char```, hash it, retrieve it from an arbitrary array index, and then perform pointer chasing if collisions exist. Thus, we are not leveraging our cache properly.

\[Soon to write about optimizing frequency counting.\]

## Acknowledgments

Early versions of my Huffman Coding file were inspired by a C++ implementation done by GeeksforGeeks (https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/?_x_tr_hist=true); I also used their tutorial to read from files using C++ (https://www.w3schools.com/cpp/cpp_files.asp).
