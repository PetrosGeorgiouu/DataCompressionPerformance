#pragma once

#include <cstdint>
#include <string>

using namespace std;

void findFrequenciesConcurrent(const string &txtPath, uint64_t* frequencies, int numThreads);