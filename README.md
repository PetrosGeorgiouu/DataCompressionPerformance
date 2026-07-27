# Huffman Data Compression Lab
A data compression library for compressing/decompressing text files. Compares a naive implementation to a performance enhanced implementation.

# How to run

For now, my goal is to focus on a base implementation to enforce functionality. The huff.cpp file runs on a fixed .txt file hard coded into the file to test correctness and performance. Run 'make build' to create the executable, along with the directorty to organize them. Then use 'make run' to actually run it. Once done, use 'make clean' to clear the directory for wasted space. When 'make run' is ran, it compiles the time it took for each step to complete along with corresponding throughput.

# How I Improved Performance

I'd also like to take some time to explain some of my optimizations and the actual statistics as to how my new model compares to the naive model.

## Acknowledgments

Early versions of my Huffman Coding file were inspired by a C++ implementation done by GeeksforGeeks (https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/?_x_tr_hist=true); I also used their tutorial to read from files using C++ (https://www.w3schools.com/cpp/cpp_files.asp).
