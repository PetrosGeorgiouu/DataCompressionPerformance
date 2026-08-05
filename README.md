# Huffman Data Compression Lab
A data compression library for compressing/decompressing text files. Compares a naive implementation to a performance enhanced implementation.

# How to run

For now, the goal is to develop a basic implementation that establishes correct functionality. The `huff.cpp` program processes a fixed `.txt` file whose path is currently hard-coded for correctness and performance testing.

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

# How I Improved Performance

I'd also like to take some time to explain some of my optimizations and the actual statistics as to how my new model compares to the naive model.

We use Linux perf tools to simulate over 2000 CPU clock samples, and an obvious bottleneck occurred in our naive implementation of frequency counting. It consumed 95.1% of the sampled CPU time. In particular, we notice these major performance issues.

- 32.1% of that was contributed to a hotspot that occurred due to the per byte extraction obtained using ```std::istream::get```, meaning that reading one character at a time became extremely expensive.
- 12.78% of consumption was attributed to ```std::istream::sentry``` every time we used the ```get()``` call.
- Some noticeable issues are that worth observing are those concerning caches. In particular, using an ```unordered_map<char, uint64_t>``` data structure for storing frequencies is not cache friendly, or trivial to compute. We take a ```char```, hash it, retrieve it from an arbitrary array index, and then perform pointer chasing if collisions exist. Thus, we are not leveraging our cache properly.

\[Soon to write about optimizing frequency counting.\]

## Acknowledgments

Early versions of my Huffman Coding file were inspired by a C++ implementation done by GeeksforGeeks (https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/?_x_tr_hist=true); I also used their tutorial to read from files using C++ (https://www.w3schools.com/cpp/cpp_files.asp).
