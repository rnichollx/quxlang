// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/context.hpp>
#include <quxlang/parsers/parse_file.hpp>

#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#ifndef QUXLANG_PARSER_BENCHMARK_CORPUS_DIR
#define QUXLANG_PARSER_BENCHMARK_CORPUS_DIR "parser_benchmark_corpus"
#endif

namespace
{
    struct generated_file
    {
        std::string name;
        std::string contents;
    };

    struct corpus
    {
        std::vector< generated_file > files;
        std::uint64_t byte_count = 0;
    };

    struct corpus_spec
    {
        std::string_view name;
        std::size_t file_count;
        std::size_t target_lines_per_file;
    };

    constexpr corpus_spec large_files_spec{.name = "large", .file_count = 10, .target_lines_per_file = 50'000};
    constexpr corpus_spec small_files_spec{.name = "small", .file_count = 2'500, .target_lines_per_file = 500};
    constexpr std::string_view corpus_marker_text = "quxlang parser benchmark corpus v1\n";

    auto corpus_root() -> std::filesystem::path
    {
        return std::filesystem::path(QUXLANG_PARSER_BENCHMARK_CORPUS_DIR);
    }

    auto random_name(std::mt19937_64& rng, std::string_view prefix) -> std::string
    {
        static constexpr std::array< char, 26 > letters{
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

        std::uniform_int_distribution< std::size_t > letter_dist(0, letters.size() - 1);

        std::string result(prefix);
        result.push_back('_');
        for (std::size_t index = 0; index != 10; ++index)
        {
            result.push_back(letters[letter_dist(rng)]);
        }
        return result;
    }

    auto append_line(std::string& out, std::size_t& line_count, std::string_view line) -> void
    {
        out += line;
        out.push_back('\n');
        ++line_count;
    }

    auto parameter_list(std::vector< std::string > const& args) -> std::string
    {
        std::string result = "(";

        for (std::size_t index = 0; index != args.size(); ++index)
        {
            if (index != 0)
            {
                result += ", ";
            }
            result += "%";
            result += args[index];
            result += " I32";
        }

        result += ")";
        return result;
    }

    auto function_signature(std::string_view name, std::vector< std::string > const& args) -> std::string
    {
        std::string result = "::";
        result += name;
        result += " FUNCTION";
        result += parameter_list(args);
        result += ": I32";
        return result;
    }

    auto generate_argument_names(std::mt19937_64& rng) -> std::vector< std::string >
    {
        std::uniform_int_distribution< int > count_dist(3, 4);
        auto const count = count_dist(rng);

        std::vector< std::string > args;
        args.reserve(static_cast< std::size_t >(count));
        for (int index = 0; index != count; ++index)
        {
            args.push_back(random_name(rng, "arg"));
        }
        return args;
    }

    auto append_function_body(std::string& out, std::size_t& line_count, std::vector< std::string > const& args) -> void
    {
        auto const& first = args[0];
        auto const& second = args[1];
        auto const& third = args[2];
        auto const& last = args.back();

        append_line(out, line_count, "{");
        append_line(out, line_count, "  VAR local I32 := " + first + " + (" + second + " * 3);");
        append_line(out, line_count, "  local := local + (" + third + " - 7);");
        append_line(out, line_count, "  local := (local * 2) + (" + first + " #++ 1);");
        append_line(out, line_count, "  IF (local > " + last + ")");
        append_line(out, line_count, "  {");
        append_line(out, line_count, "    local := local - 1;");
        append_line(out, line_count, "    ASSERT(local >= 0);");
        append_line(out, line_count, "  } ELSE {");
        append_line(out, line_count, "    local := local + 2;");
        append_line(out, line_count, "    ASSERT(local != 17);");
        append_line(out, line_count, "  }");
        append_line(out, line_count, "  WHILE (local < 128)");
        append_line(out, line_count, "  {");
        append_line(out, line_count, "    local := local + 1;");
        append_line(out, line_count, "    local := local + " + first + " + " + second + ";");
        append_line(out, line_count, "  }");
        append_line(out, line_count, "  RETURN local;");
        append_line(out, line_count, "}");
    }

    auto append_function_declaration(std::string& out, std::size_t& line_count, std::mt19937_64& rng) -> void
    {
        auto const name = random_name(rng, "fn");
        auto args = generate_argument_names(rng);

        append_line(out, line_count, function_signature(name, args));
        append_function_body(out, line_count, args);
    }

    auto append_class_declaration(std::string& out, std::size_t& line_count, std::mt19937_64& rng) -> void
    {
        auto const class_name = random_name(rng, "cls");
        auto const method_name = random_name(rng, "method");
        auto args = generate_argument_names(rng);

        append_line(out, line_count, "::" + class_name + " CLASS");
        append_line(out, line_count, "{");
        append_line(out, line_count, "  .value VAR I32;");
        append_line(out, line_count, "  .count VAR I64;");
        auto method_decl = std::string("  .");
        method_decl += method_name;
        method_decl += " FUNCTION";
        method_decl += parameter_list(args);
        method_decl += ": I32";
        append_line(out, line_count, method_decl);
        append_function_body(out, line_count, args);
        append_line(out, line_count, "}");
    }

    auto generate_source_file(std::size_t target_lines, std::uint64_t seed) -> std::string
    {
        std::mt19937_64 rng(seed);
        std::string out;
        out.reserve(target_lines * 72);

        std::size_t line_count = 0;
        append_line(out, line_count, "::" + random_name(rng, "outer") + " NAMESPACE");
        append_line(out, line_count, "{");
        append_line(out, line_count, "::" + random_name(rng, "inner") + " NAMESPACE");
        append_line(out, line_count, "{");

        std::size_t declaration_index = 0;
        while (line_count + 32 < target_lines)
        {
            if (declaration_index % 9 == 0)
            {
                append_class_declaration(out, line_count, rng);
            }
            else
            {
                append_function_declaration(out, line_count, rng);
            }
            ++declaration_index;
        }

        while (line_count + 2 < target_lines)
        {
            append_line(out, line_count, "::" + random_name(rng, "pad") + " VAR I32 := 1;");
        }

        append_line(out, line_count, "}");
        append_line(out, line_count, "}");
        return out;
    }

    auto write_file(std::filesystem::path const& path, std::string const& contents) -> void
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw quxlang::compilation_error("failed to open parser benchmark corpus file for writing: " + path.string());
        }
        out.write(contents.data(), static_cast< std::streamsize >(contents.size()));
        if (!out)
        {
            throw quxlang::compilation_error("failed to write parser benchmark corpus file: " + path.string());
        }
    }

    auto read_file(std::filesystem::path const& path) -> std::string
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            throw quxlang::compilation_error("failed to open parser benchmark corpus file for reading: " + path.string());
        }

        return std::string(std::istreambuf_iterator< char >(in), std::istreambuf_iterator< char >());
    }

    auto corpus_file_path(std::filesystem::path const& dir, corpus_spec spec, std::size_t index) -> std::filesystem::path
    {
        auto file_name = std::string(spec.name);
        file_name += "_";
        file_name += std::to_string(index);
        file_name += ".qx";
        return dir / file_name;
    }

    auto ensure_corpus(corpus_spec spec) -> std::filesystem::path
    {
        auto const root = corpus_root();
        auto const dir = root / spec.name;
        auto const marker = dir / "CORPUS_VERSION";

        if (std::filesystem::exists(marker) && read_file(marker) == corpus_marker_text)
        {
            return dir;
        }

        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);

        for (std::size_t index = 0; index != spec.file_count; ++index)
        {
            auto const contents = generate_source_file(spec.target_lines_per_file, 0x9e3779b97f4a7c15ULL + index);
            write_file(corpus_file_path(dir, spec, index), contents);
        }

        write_file(marker, std::string(corpus_marker_text));
        return dir;
    }

    auto load_corpus(corpus_spec spec) -> corpus
    {
        auto const dir = ensure_corpus(spec);

        corpus result;
        result.files.reserve(spec.file_count);

        for (std::size_t index = 0; index != spec.file_count; ++index)
        {
            auto path = corpus_file_path(dir, spec, index);
            auto contents = read_file(path);
            result.byte_count += contents.size();
            result.files.push_back(generated_file{.name = path.filename().string(), .contents = std::move(contents)});
        }

        return result;
    }

    auto parse_file(generated_file const& file, std::size_t index) -> std::size_t
    {
        std::uint64_t const file_id = static_cast< std::uint64_t >(index);
        auto ctx = quxlang::parsers::parsing_context{
            .file_id = file_id,
            .source_locations_enabled = true,
            .iter_begin = file.contents.begin(),
            .iter_pos = file.contents.begin(),
            .iter_end = file.contents.end(),
        };

        auto parsed = quxlang::parsers::parse_file(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw quxlang::compiler_bug("parser benchmark did not consume the whole generated file: " + file.name);
        }

        return parsed.declarations.size();
    }

    auto parse_corpus(corpus const& input) -> std::size_t
    {
        std::size_t declarations = 0;

        for (std::size_t index = 0; index != input.files.size(); ++index)
        {
            declarations += parse_file(input.files[index], index);
        }

        return declarations;
    }

    auto parse_corpus_multicore(corpus const& input, std::size_t thread_count) -> std::size_t
    {
        if (thread_count == 0)
        {
            throw std::invalid_argument("parser benchmark thread count must be nonzero");
        }

        std::vector< std::thread > workers;
        std::vector< std::size_t > declaration_counts(thread_count, 0);
        std::vector< std::exception_ptr > worker_exceptions(thread_count);
        workers.reserve(thread_count);

        for (std::size_t worker_index = 0; worker_index != thread_count; ++worker_index)
        {
            auto worker = [&, worker_index]()
            {
                try
                {
                    std::size_t const begin = input.files.size() * worker_index / thread_count;
                    std::size_t const end = input.files.size() * (worker_index + 1) / thread_count;

                    std::size_t declarations = 0;
                    for (std::size_t file_index = begin; file_index != end; ++file_index)
                    {
                        declarations += parse_file(input.files[file_index], file_index);
                    }
                    declaration_counts[worker_index] = declarations;
                }
                catch (...)
                {
                    worker_exceptions[worker_index] = std::current_exception();
                }
            };

            workers.emplace_back(worker);
        }

        for (std::thread& worker : workers)
        {
            worker.join();
        }

        std::size_t declarations = 0;
        for (std::size_t worker_index = 0; worker_index != thread_count; ++worker_index)
        {
            if (worker_exceptions[worker_index] != nullptr)
            {
                std::rethrow_exception(worker_exceptions[worker_index]);
            }
            declarations += declaration_counts[worker_index];
        }

        return declarations;
    }

    auto run_parser_benchmark(benchmark::State& state, corpus_spec spec) -> void
    {
        auto const input = load_corpus(spec);

        for (auto _ : state)
        {
            auto declarations = parse_corpus(input);
            benchmark::DoNotOptimize(declarations);
        }

        state.SetBytesProcessed(static_cast< std::int64_t >(state.iterations()) * static_cast< std::int64_t >(input.byte_count));
        state.SetItemsProcessed(static_cast< std::int64_t >(state.iterations()) * static_cast< std::int64_t >(input.files.size()));
    }

    auto run_multicore_parser_benchmark(benchmark::State& state, corpus_spec spec) -> void
    {
        auto const input = load_corpus(spec);
        std::size_t const thread_count = static_cast< std::size_t >(state.range(0));

        for (auto _ : state)
        {
            auto declarations = parse_corpus_multicore(input, thread_count);
            benchmark::DoNotOptimize(declarations);
        }

        state.SetBytesProcessed(static_cast< std::int64_t >(state.iterations()) * static_cast< std::int64_t >(input.byte_count));
        state.SetItemsProcessed(static_cast< std::int64_t >(state.iterations()) * static_cast< std::int64_t >(input.files.size()));
    }

    auto multicore_parser_args(benchmark::internal::Benchmark* benchmark) -> void
    {
        benchmark->ArgName("Threads");
        for (int thread_count : {1, 2, 4, 8, 16, 20})
        {
            benchmark->Arg(thread_count);
        }
    }

    void BM_ParseGeneratedLargeFiles(benchmark::State& state)
    {
        run_parser_benchmark(state, large_files_spec);
    }

    void BM_ParseGeneratedManySmallFiles(benchmark::State& state)
    {
        run_parser_benchmark(state, small_files_spec);
    }

    void BM_ParseGeneratedLargeFilesMulticore(benchmark::State& state)
    {
        run_multicore_parser_benchmark(state, large_files_spec);
    }

    void BM_ParseGeneratedManySmallFilesMulticore(benchmark::State& state)
    {
        run_multicore_parser_benchmark(state, small_files_spec);
    }
} // namespace

BENCHMARK(BM_ParseGeneratedLargeFiles)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ParseGeneratedManySmallFiles)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ParseGeneratedLargeFilesMulticore)->Apply(multicore_parser_args)->UseRealTime()->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ParseGeneratedManySmallFilesMulticore)->Apply(multicore_parser_args)->UseRealTime()->Unit(benchmark::kMillisecond);
