# Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
# Compiles and executes benchmark contracts with the compiler built by this workspace.
cmake_minimum_required(VERSION 3.25)
foreach(required QXC SOURCE_BUNDLE WORK_DIR PLATFORM CPU)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
file(MAKE_DIRECTORY "${WORK_DIR}/bundle/modules")
foreach(module runtime std syscall posix benchmarks benchmark_files)
    file(COPY "${SOURCE_BUNDLE}/modules/${module}" DESTINATION "${WORK_DIR}/bundle/modules")
endforeach()
file(MAKE_DIRECTORY "${WORK_DIR}/bundle/modules/benchmark_checks/sources")
file(COPY "${SOURCE_BUNDLE}/modules/tests/sources/main_test_benchmark_runtime.qxs" DESTINATION "${WORK_DIR}/bundle/modules/benchmark_checks/sources")
file(READ "${SOURCE_BUNDLE}/modules/tests/sources/main_test_22_filesystem.qxs" filesystem_tests)
string(REPLACE "LANGUAGE QUXLANG EN 0.0;" "LANGUAGE QUXLANG EN 0.0;\nIMPORT std;\nIMPORT syscall;" filesystem_tests "${filesystem_tests}")
file(WRITE "${WORK_DIR}/bundle/modules/benchmark_checks/sources/filesystem.qxs" "${filesystem_tests}")
set(target "targets:\n  native:\n    platform: ${PLATFORM}\n    cpu: ${CPU}\n    backend: llvm\n    steppings: [{attributes: []}]\n    modules:\n      RUNTIME: {source: runtime}\n      std: {source: std}\n      syscall: {source: syscall}\n      posix: {source: posix}\n      main: {source: benchmarks}\n      checks: {source: benchmark_checks}\n      benchmark_files: {source: benchmark_files}\n")
set(outputs "outputs:\n  tests: {target: native, type: unit_test_suite, build_type: Release, test_modules: [checks]}\n  suite: {target: native, type: benchmark_suite, build_type: Release}\n  numeric: {target: native, type: benchmark_executable, build_type: Release, benchmark: '::namespace_case::numeric'}\n")
if(PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
    string(APPEND outputs "  copy: {target: native, type: benchmark_executable, build_type: Release, main_module: benchmark_files, benchmark: '::copy'}\n")
endif()
file(WRITE "${WORK_DIR}/bundle/qxcbuild.yml" "${target}${outputs}")
execute_process(COMMAND "${QXC}" "${WORK_DIR}/bundle" "${WORK_DIR}/compiled" native
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 600)
file(WRITE "${WORK_DIR}/compile.log" "${output}${errors}")
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Benchmark compilation failed: ${errors}")
endif()
set(binary "${WORK_DIR}/compiled/output")
execute_process(COMMAND "${binary}/tests" RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Benchmark runtime tests failed: ${output}${errors}")
endif()
file(WRITE "${WORK_DIR}/runtime-tests.log" "${output}${errors}")
execute_process(COMMAND "${binary}/suite" --benchmark-iterations=2 --benchmark-repetitions=2 --benchmark-warmup=0 --benchmark-format=json
    RESULT_VARIABLE status OUTPUT_VARIABLE results ERROR_VARIABLE errors TIMEOUT 30)
file(WRITE "${WORK_DIR}/results.json" "${results}")
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Benchmark suite failed: ${errors}\n${results}")
endif()
string(JSON channels LENGTH "${results}")
if(NOT channels EQUAL 10)
    message(FATAL_ERROR "Expected ten expanded measurement channels: ${results}")
endif()
math(EXPR last "${channels} - 1")
foreach(index RANGE 0 ${last})
    string(JSON name GET "${results}" ${index} benchmark)
    string(JSON measurement GET "${results}" ${index} measurement)
    string(JSON repetitions LENGTH "${results}" ${index} samples)
    if(NOT repetitions EQUAL 2)
        message(FATAL_ERROR "Incorrect repetition count for ${name}")
    endif()
    set(expected 3)
    if(name MATCHES "insert_delete/small$")
        set(expected 8)
    elseif(name MATCHES "insert_delete/large$")
        set(expected 32)
    elseif(name MATCHES "repeated_channel$")
        set(expected 5)
    elseif(name MATCHES "numeric/default$")
        set(expected 7)
    elseif(name MATCHES "lifetimes$")
        set(expected 6)
    endif()
    if(measurement STREQUAL "unused")
        set(expected 0)
        string(JSON summary_type TYPE "${results}" ${index} median_ns_per_operation)
        if(NOT summary_type STREQUAL "NULL")
            message(FATAL_ERROR "Unexecuted channel must be unmeasured")
        endif()
    endif()
    foreach(sample RANGE 0 1)
        string(JSON count GET "${results}" ${index} samples ${sample} count)
        if(NOT count EQUAL expected)
            message(FATAL_ERROR "${name}/${measurement}: expected ${expected} operations, received ${count}")
        endif()
    endforeach()
endforeach()
execute_process(COMMAND "${binary}/suite" --list-benchmarks --benchmark-filter=insert_delete
    RESULT_VARIABLE status OUTPUT_VARIABLE listing ERROR_VARIABLE errors TIMEOUT 30)
if(NOT status EQUAL 0 OR NOT listing MATCHES "insert_delete/small" OR NOT listing MATCHES "insert_delete/large" OR listing MATCHES "numeric")
    message(FATAL_ERROR "Listing or filtering failed: ${listing}${errors}")
endif()
execute_process(COMMAND "${binary}/numeric" --size=7 --enabled true RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT status EQUAL 0 OR NOT output STREQUAL "")
    message(FATAL_ERROR "Standalone numeric argument loading failed: ${output}${errors}")
endif()
execute_process(COMMAND "${binary}/numeric" --enabled=true --size 7 RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT status EQUAL 0 OR NOT output STREQUAL "")
    message(FATAL_ERROR "Standalone option order or alternate spelling failed: ${output}${errors}")
endif()
foreach(arguments "--size=7" "--size=7;--size=7;--enabled=true" "--size=7;--enabled=true;--unknown=x" "--size=bad;--enabled=true" "--size=18446744073709551616;--enabled=true" "--size=7;--enabled=bad")
    execute_process(COMMAND "${binary}/numeric" ${arguments} RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 2)
        message(FATAL_ERROR "Expected argument failure for ${arguments}: ${status}, ${output}${errors}")
    endif()
endforeach()
if(PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
    string(REPEAT "benchmark input\nwith multiple lines\n" 512 file_content)
    file(WRITE "${WORK_DIR}/entrée-文件-📄.bin" "${file_content}")
    file(WRITE "${WORK_DIR}/output.bin" "${file_content}stale suffix")
    execute_process(COMMAND "${binary}/copy" --output "${WORK_DIR}/output.bin" "--data=${WORK_DIR}/entrée-文件-📄.bin"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 0 OR NOT output STREQUAL "")
        message(FATAL_ERROR "Standalone file benchmark failed: ${output}${errors}")
    endif()
    file(SHA256 "${WORK_DIR}/entrée-文件-📄.bin" input_hash)
    file(SHA256 "${WORK_DIR}/output.bin" output_hash)
    if(NOT input_hash STREQUAL output_hash)
        message(FATAL_ERROR "Standalone file output differs from its input")
    endif()
    file(WRITE "${WORK_DIR}/empty.bin" "")
    execute_process(COMMAND "${binary}/copy" "--data=${WORK_DIR}/empty.bin" "--output=${WORK_DIR}/output.bin"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    file(SIZE "${WORK_DIR}/output.bin" output_size)
    if(NOT status EQUAL 0 OR NOT output_size EQUAL 0)
        message(FATAL_ERROR "Empty input did not truncate the output: ${output}${errors}")
    endif()
    execute_process(COMMAND "${binary}/copy" "--data=${WORK_DIR}/absent-directory/missing.bin" "--output=${WORK_DIR}/output.bin"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 2 OR NOT output STREQUAL "")
        message(FATAL_ERROR "File-open failure did not prevent the workload: ${output}${errors}")
    endif()
endif()
message(STATUS "Benchmark execution, measurement counts, CLI loading, and file output passed")

# A deterministic runtime clock isolates measurement accounting from host timing.
set(clock_file "${WORK_DIR}/bundle/modules/runtime/sources/benchmark_clock.qxs")
file(READ "${clock_file}" clock_source)
string(FIND "${clock_source}" "::BENCHMARK_CLOCK DOC" clock_begin)
string(FIND "${clock_source}" "::BENCHMARK_OPERATION_COUNT DOC" clock_end)
string(SUBSTRING "${clock_source}" 0 ${clock_begin} clock_prefix)
string(SUBSTRING "${clock_source}" ${clock_end} -1 clock_suffix)
file(WRITE "${clock_file}" "${clock_prefix}::benchmark_test_ticks VAR U64;\n::BENCHMARK_CLOCK FUNCTION(): U64 { benchmark_test_ticks += 10; RETURN benchmark_test_ticks; }\n${clock_suffix}")
file(READ "${WORK_DIR}/bundle/modules/benchmarks/sources/benchmarks.qxs" lifetime_source)
string(REPLACE "    counter++;" "    counter++; RUNTIME_MODULE::BENCHMARK_CLOCK();" lifetime_source "${lifetime_source}")
string(REPLACE ".counter-> := .counter-> - 1;" ".counter-> := .counter-> - 1; RUNTIME_MODULE::BENCHMARK_CLOCK();" lifetime_source "${lifetime_source}")
file(WRITE "${WORK_DIR}/bundle/modules/benchmarks/sources/benchmarks.qxs" "${lifetime_source}")
file(WRITE "${WORK_DIR}/bundle/qxcbuild.yml" "${target}outputs:\n  suite: {target: native, type: benchmark_suite, build_type: Release}\n")
execute_process(COMMAND "${QXC}" "${WORK_DIR}/bundle" "${WORK_DIR}/controlled" native
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 600)
file(WRITE "${WORK_DIR}/controlled-compile.log" "${output}${errors}")
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Controlled clock compilation failed: ${errors}")
endif()
execute_process(COMMAND "${WORK_DIR}/controlled/output/suite" --benchmark-iterations=2 --benchmark-repetitions=1 --benchmark-warmup=0 --benchmark-format=json
    RESULT_VARIABLE status OUTPUT_VARIABLE results ERROR_VARIABLE errors TIMEOUT 30)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Controlled clock suite failed: ${errors}")
endif()
foreach(index RANGE 0 ${last})
    string(JSON name GET "${results}" ${index} benchmark)
    string(JSON measurement GET "${results}" ${index} measurement)
    string(JSON elapsed GET "${results}" ${index} samples 0 elapsed_ns)
    set(expected 10)
    if(name MATCHES "insert_delete/|repeated_channel$|lifetimes$")
        set(expected 20)
    endif()
    if(name MATCHES "lifetimes$")
        set(expected 60)
    endif()
    if(measurement STREQUAL "unused")
        set(expected 0)
    endif()
    if(NOT elapsed EQUAL expected)
        message(FATAL_ERROR "${name}/${measurement}: expected ${expected} ns, received ${elapsed}")
    endif()
    if(name MATCHES "insert_delete/small$" AND NOT measurement STREQUAL "unused")
        string(JSON ratio GET "${results}" ${index} median_ns_per_operation)
        if(NOT ratio STREQUAL "2.5")
            message(FATAL_ERROR "Expected 20 ns / 8 operations = 2.5 ns/op, received ${ratio}")
        endif()
    endif()
endforeach()
file(WRITE "${clock_file}" "${clock_source}")

# Each negative fixture uses its own source declaration and the ordinary output pipeline.
file(MAKE_DIRECTORY "${WORK_DIR}/bundle/modules/probe/sources")
string(REPLACE "main: {source: benchmarks}" "main: {source: probe}" probe_target "${target}")
string(REPLACE "    backend: llvm" "    run_static_tests: false\n    backend: llvm" probe_target "${probe_target}")
function(reject_benchmark label declaration kind expected)
    file(WRITE "${WORK_DIR}/bundle/modules/probe/sources/probe.qxs" "LANGUAGE QUXLANG EN 0.0;\n${declaration}\n")
    set(selection "")
    if(kind STREQUAL "benchmark_executable")
        if(ARGC GREATER 4)
            set(selection ", benchmark: '${ARGV4}'")
        else()
            set(selection ", benchmark: '::probe'")
        endif()
    endif()
    file(WRITE "${WORK_DIR}/bundle/qxcbuild.yml" "${probe_target}outputs:\n  probe: {target: native, type: ${kind}, build_type: Release${selection}}\n")
    execute_process(COMMAND "${QXC}" "${WORK_DIR}/bundle" "${WORK_DIR}/rejected-${label}" native
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
    file(WRITE "${WORK_DIR}/${label}.log" "${output}${errors}")
    if(status EQUAL 0 OR NOT errors MATCHES "${expected}")
        message(FATAL_ERROR "${label}: expected ${expected}, received status ${status}: ${errors}")
    endif()
endfunction()
reject_benchmark(body_standalone "::probe BENCHMARK() BODY { BENCHMARK_LOOP {} }" benchmark_executable "requires ONESHOT")
reject_benchmark(parser_unavailable "::probe BENCHMARK(@value I32) CLI_PARSER(unavailable) ONESHOT {}" benchmark_executable "unavailable")
reject_benchmark(parser_incompatible "::unparsed STRUCT {} ::probe BENCHMARK(@value unparsed) ONESHOT {}" benchmark_executable "DEFAULT_PARSER")
reject_benchmark(missing_symbol "::different BENCHMARK() ONESHOT {}" benchmark_executable "[Bb]enchmark|[Cc]ould not|[Ee]xpected")
reject_benchmark(missing_cases "::probe BENCHMARK(@value U64) ONESHOT {}" benchmark_suite "requires CASE")
reject_benchmark(missing_loop "::probe BENCHMARK() BODY {}" benchmark_suite "exactly one")
reject_benchmark(two_loops "::probe BENCHMARK() BODY { BENCHMARK_LOOP {} BENCHMARK_LOOP {} }" benchmark_suite "exactly one")
reject_benchmark(nested_loop "::probe BENCHMARK() BODY { BENCHMARK_LOOP { BENCHMARK_LOOP {} } }" benchmark_suite "exactly once")
reject_benchmark(oneshot_loop "::probe BENCHMARK() ONESHOT { BENCHMARK_LOOP {} }" benchmark_suite "ONESHOT requires none")
reject_benchmark(nested_measurement "::probe BENCHMARK() ONESHOT { MEASURE { MEASURE {} } }" benchmark_suite "cannot be nested")
reject_benchmark(unknown_channel "::probe BENCHMARK [time]() ONESHOT { MEASURE(other) {} }" benchmark_suite "Unknown benchmark measurement")
reject_benchmark(ambiguous_channel "::probe BENCHMARK [first, second]() ONESHOT { MEASURE {} }" benchmark_suite "requires a name")
reject_benchmark(measurement_outside_loop "::probe BENCHMARK() BODY { MEASURE {} BENCHMARK_LOOP {} }" benchmark_suite "inside BENCHMARK_LOOP")
reject_benchmark(bypass_loop "::probe BENCHMARK() BODY { RETURN; BENCHMARK_LOOP {} }" benchmark_suite "control flow")
reject_benchmark(break_loop "::probe BENCHMARK() BODY { BENCHMARK_LOOP { BREAK; } }" benchmark_suite "control flow")
reject_benchmark(continue_loop "::probe BENCHMARK() BODY { BENCHMARK_LOOP { CONTINUE; } }" benchmark_suite "control flow")
reject_benchmark(break_after_inner "::probe BENCHMARK() BODY { BENCHMARK_LOOP { LOOP DO { BREAK; } MEASURE {} BREAK; } }" benchmark_suite "control flow")
reject_benchmark(continue_after_inner "::probe BENCHMARK() BODY { BENCHMARK_LOOP { MEASURE { LOOP DO { BREAK; } CONTINUE; } } }" benchmark_suite "control flow")
reject_benchmark(break_after_static_loop "::probe BENCHMARK() BODY { BENCHMARK_LOOP { STATIC_WHILE(FALSE) {} MEASURE {} BREAK; } }" benchmark_suite "control flow")
reject_benchmark(continue_after_static_loop "::probe BENCHMARK() BODY { BENCHMARK_LOOP { MEASURE { STATIC_WHILE(FALSE) {} CONTINUE; } } }" benchmark_suite "control flow")
reject_benchmark(exit_measurement "::probe BENCHMARK() BODY { BENCHMARK_LOOP { LOOP DO { MEASURE { BREAK; } } } }" benchmark_suite "control flow")
reject_benchmark(invocation_selection "::probe BENCHMARK() ONESHOT {}" benchmark_executable "without invocation syntax" "::probe#()")
reject_benchmark(nested_invocation_selection "::probe BENCHMARK() ONESHOT {}" benchmark_executable "without invocation syntax" "::probe#()::other")
message(STATUS "Controlled timing and benchmark compilation diagnostics passed")

# Decimal scaling applies before the destination floating-point range is checked.
file(WRITE "${WORK_DIR}/bundle/modules/probe/sources/probe.qxs" [=[LANGUAGE QUXLANG EN 0.0;
::probe BENCHMARK(@single F32, @wide F64) ONESHOT
{
  TEST_ASSERT(single == 1.0);
  TEST_ASSERT(wide == 1.0);
}
]=])
file(WRITE "${WORK_DIR}/bundle/qxcbuild.yml" "${probe_target}outputs:\n  probe: {target: native, type: benchmark_executable, build_type: Release, benchmark: '::probe'}\n")
execute_process(COMMAND "${QXC}" "${WORK_DIR}/bundle" "${WORK_DIR}/floating" native
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Floating-point fixture failed to compile: ${errors}")
endif()
string(REPEAT "0" 400 zeroes)
foreach(value "1${zeroes}e-400" "1.${zeroes}" "0.${zeroes}1e401")
    execute_process(COMMAND "${WORK_DIR}/floating/output/probe" "--single=${value}" --wide "${value}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 0 OR NOT output STREQUAL "")
        message(FATAL_ERROR "Finite decimal argument failed to parse: ${status}, ${output}${errors}")
    endif()
endforeach()
message(STATUS "Long floating-point command-line arguments passed")

# A throwing measurement invalidates the entire sample and unwinds measured locals.
file(WRITE "${WORK_DIR}/bundle/modules/probe/sources/probe.qxs" [=[LANGUAGE QUXLANG EN 0.0;
::cleanup STRUCT
{
  .DESTRUCTOR FUNCTION() { RUNTIME_MODULE::write(@fd 2, @msg "measurement cleanup\n"); }
}
::probe BENCHMARK() ONESHOT
{
  MEASURE { VAR local cleanup; THROW (1 AS I32); }
}
]=])
file(WRITE "${WORK_DIR}/bundle/qxcbuild.yml" "${probe_target}outputs:\n  probe: {target: native, type: benchmark_suite, build_type: Release}\n")
execute_process(COMMAND "${QXC}" "${WORK_DIR}/bundle" "${WORK_DIR}/exception" native
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Exception fixture failed to compile: ${errors}")
endif()
execute_process(COMMAND "${WORK_DIR}/exception/output/probe" --benchmark-repetitions=1 --benchmark-warmup=0 --benchmark-format=json
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
string(JSON samples LENGTH "${output}")
if(NOT status EQUAL 1 OR NOT samples EQUAL 0 OR NOT errors MATCHES "measurement cleanup")
    message(FATAL_ERROR "Exception failed to invalidate the sample or unwind its locals: ${status}, ${output}${errors}")
endif()
message(STATUS "Exception invalidation and measured-object cleanup passed")

# Parsing failure unwinds parsed arguments and the parser before the workload starts.
file(WRITE "${WORK_DIR}/bundle/modules/probe/sources/probe.qxs" [=[LANGUAGE QUXLANG EN 0.0;
IMPORT std;
::argument STRUCT
{
  .DESTRUCTOR FUNCTION() { RUNTIME_MODULE::write(@fd 2, @msg "argument cleanup\n"); }
}
::parser STRUCT
{
  .CONSTRUCTOR FUNCTION() { RUNTIME_MODULE::write(@fd 2, @msg "parser constructed\n"); }
  .DESTRUCTOR FUNCTION() { RUNTIME_MODULE::write(@fd 2, @msg "parser cleanup\n"); }
  .PARSE TEMPLATE(@T TYPE AUTO) FUNCTION(@INPUT CONST& std::string, @OUTPUT WRITE& T) MUT
  {
    RUNTIME_MODULE::write(@fd 2, @msg "argument parsed\n");
    IF (INPUT == "fail") { THROW (1 AS I32); }
  }
}
::probe BENCHMARK(@first argument, @second argument) CLI_PARSER(parser) ONESHOT
{
  PANIC "Workload ran after parsing failed";
}
]=])
file(WRITE "${WORK_DIR}/bundle/qxcbuild.yml" "${probe_target}outputs:\n  probe: {target: native, type: benchmark_executable, build_type: Release, benchmark: '::probe'}\n")
execute_process(COMMAND "${QXC}" "${WORK_DIR}/bundle" "${WORK_DIR}/parser-exception" native
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Parser exception fixture failed to compile: ${errors}")
endif()
execute_process(COMMAND "${WORK_DIR}/parser-exception/output/probe" --second=fail --first=ok
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
set(expected "parser constructed\nargument parsed\nargument parsed\nargument cleanup\nargument cleanup\nparser cleanup\n")
if(NOT status EQUAL 2 OR NOT output STREQUAL "" OR NOT errors STREQUAL expected)
    message(FATAL_ERROR "Parser failure did not preserve parsing order and cleanup: ${status}, ${output}${errors}")
endif()
execute_process(COMMAND "${WORK_DIR}/parser-exception/output/probe" --first=ok
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT status EQUAL 2 OR errors MATCHES "parser constructed|argument parsed")
    message(FATAL_ERROR "Missing arguments were not rejected before parser construction: ${status}, ${output}${errors}")
endif()
message(STATUS "Parser exception cleanup and argument prevalidation passed")
