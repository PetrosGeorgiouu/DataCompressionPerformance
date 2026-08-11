#include <benchmark/benchmark.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "huffman/Compression.hpp" // change to your actual header

static std::string g_inputPath;

static void BM_CompressFile(benchmark::State &state)
{
  const std::string outputPath = "/tmp/huffman_benchmark.huff";

  if (!std::filesystem::exists(g_inputPath))
  {
    state.SkipWithError("Input file does not exist");
    return;
  }

  const auto fileSize = std::filesystem::file_size(g_inputPath);

  for (auto _ : state)
  {
    // This is the operation we actually care about timing.
    compressor(g_inputPath, outputPath);
  }

  // Tell Google Benchmark how much data we processed.
  state.SetBytesProcessed(
      static_cast<int64_t>(state.iterations()) *
      static_cast<int64_t>(fileSize));

  // Cleanup is NOT part of compression performance.
  std::filesystem::remove(outputPath);
}

BENCHMARK(BM_CompressFile)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

int main(int argc, char **argv)
{
  // Our custom argument:
  //
  // ./fileperf data/corpus/myfile.txt
  //
  // Remove it before Google Benchmark parses its own arguments.

  benchmark::MaybeReenterWithoutASLR(argc, argv);

  if (argc < 2)
  {
    std::cerr
        << "Usage: " << argv[0]
        << " <input-file> [benchmark options]\n";

    return 1;
  }

  g_inputPath = argv[1];

  // Shift arguments left so Google Benchmark doesn't see
  // the filename as an unknown argument.
  for (int i = 1; i < argc - 1; ++i)
  {
    argv[i] = argv[i + 1];
  }

  --argc;

  benchmark::Initialize(&argc, argv);

  if (benchmark::ReportUnrecognizedArguments(argc, argv))
  {
    return 1;
  }

  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();

  return 0;
}