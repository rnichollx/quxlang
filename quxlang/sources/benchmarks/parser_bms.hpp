// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_BENCHMARKS_PARSER_BMS_HEADER_GUARD
#define QUXLANG_SOURCES_BENCHMARKS_PARSER_BMS_HEADER_GUARD

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace quxlang::benchmarks
{
    /** One generated parser benchmark input file. */
    struct generated_file;
    /** One in-memory parser benchmark corpus. */
    struct corpus;
    /** Parameters controlling a generated parser corpus. */
    struct corpus_spec;

    /** Corpus parameters for a small number of large files. */
    extern corpus_spec const large_files_spec;
    /** Corpus parameters for a large number of small files. */
    extern corpus_spec const small_files_spec;
    /** Version marker stored beside generated corpora. */
    extern std::string_view const corpus_marker_text;

    /** Returns the generated corpus root directory. */
    auto corpus_root() -> std::filesystem::path;
    /** Generates one deterministic identifier. */
    auto random_name(std::mt19937_64& rng, std::string_view prefix) -> std::string;
    /** Appends one generated source line. */
    void append_line(std::string& out, std::size_t& line_count, std::string_view line);
    /** Formats a generated function parameter list. */
    auto parameter_list(std::vector< std::string > const& args) -> std::string;
    /** Formats a generated function signature. */
    auto function_signature(std::string_view name, std::vector< std::string > const& args) -> std::string;
    /** Generates deterministic argument names. */
    auto generate_argument_names(std::mt19937_64& rng) -> std::vector< std::string >;
    /** Appends one generated function body. */
    void append_function_body(std::string& out, std::size_t& line_count, std::vector< std::string > const& args);
    /** Appends one generated function declaration. */
    void append_function_declaration(std::string& out, std::size_t& line_count, std::mt19937_64& rng);
    /** Appends one generated struct declaration. */
    void append_struct_declaration(std::string& out, std::size_t& line_count, std::mt19937_64& rng);
    /** Generates one parser benchmark source file. */
    auto generate_source_file(std::size_t target_lines, std::uint64_t seed) -> std::string;
    /** Writes one corpus file. */
    void write_file(std::filesystem::path const& path, std::string const& contents);
    /** Reads one corpus file. */
    auto read_file(std::filesystem::path const& path) -> std::string;
    /** Returns the path for one generated corpus member. */
    auto corpus_file_path(std::filesystem::path const& dir, corpus_spec spec, std::size_t index) -> std::filesystem::path;
    /** Generates a corpus when its on-disk version is stale. */
    auto ensure_corpus(corpus_spec spec) -> std::filesystem::path;
    /** Loads a generated corpus into memory. */
    auto load_corpus(corpus_spec spec) -> corpus;
    /** Parses one generated source file. */
    auto parse_file(generated_file const& file, std::size_t index) -> std::size_t;
    /** Parses every file in a corpus serially. */
    auto parse_corpus(corpus const& input) -> std::size_t;
    /** Parses a corpus with the requested worker count. */
    auto parse_corpus_multicore(corpus const& input, std::size_t thread_count) -> std::size_t;
    /** Runs one serial parser benchmark. */
    void run_parser_benchmark(benchmark::State& state, corpus_spec spec);
    /** Runs one multicore parser benchmark. */
    void run_multicore_parser_benchmark(benchmark::State& state, corpus_spec spec);
    /** Registers multicore benchmark worker counts. */
    void multicore_parser_args(benchmark::internal::Benchmark* benchmark);
    /** Benchmarks parsing large generated files. */
    void BM_ParseGeneratedLargeFiles(benchmark::State& state);
    /** Benchmarks parsing many small generated files. */
    void BM_ParseGeneratedManySmallFiles(benchmark::State& state);
    /** Benchmarks multicore parsing of large generated files. */
    void BM_ParseGeneratedLargeFilesMulticore(benchmark::State& state);
    /** Benchmarks multicore parsing of many small generated files. */
    void BM_ParseGeneratedManySmallFilesMulticore(benchmark::State& state);
} // namespace quxlang::benchmarks

#endif // QUXLANG_SOURCES_BENCHMARKS_PARSER_BMS_HEADER_GUARD
