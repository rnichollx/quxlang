#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/../.." && pwd)"
workspace_dir="${QXC_WORKSPACE_DIR:-$repo_root/dev-workspace}"
system_name="${QXC_SYSTEM_NAME:-system-clang}"
qxc_path="${QXC_BIN:-$workspace_dir/buildspaces/$system_name/quxlang/Release/qxc}"
input_dir="${QXC_INPUT_DIR:-$script_dir/testdata/testmodule}"
output_dir="${QXC_OUTPUT_DIR:-/tmp/qxc-fusion-native-failures}"
ubuntu_image="${QXC_UBUNTU_IMAGE:-ubuntu:24.04}"

if [[ ! -x "$qxc_path" ]]; then
    printf 'Missing Release qxc: %s\n' "$qxc_path" >&2
    exit 1
fi
if ! command -v docker >/dev/null 2>&1; then
    printf 'docker was not found in PATH.\n' >&2
    exit 1
fi

if [[ "${QXC_CLEAN_OUTPUT:-0}" == "1" ]]; then
    case "$output_dir" in
        ''|'/'|'/tmp'|'/tmp/'|'/private/tmp'|'/private/tmp/')
            printf 'Refusing to clean unsafe output directory: %s\n' "$output_dir" >&2
            exit 1
            ;;
    esac
    rm -rf -- "$output_dir"
fi
mkdir -p "$output_dir"

printf 'Compiling fusion native expected-failure executables...\n'
"$qxc_path" "$input_dir" "$output_dir" tests-linux-x64

output_dir="$(cd -- "$output_dir" && pwd)"
passed=0

while IFS='|' read -r executable expected_output; do
    [[ -n "$executable" ]] || continue

    test_path="output/tests-linux-x64/$executable"
    host_test_path="$output_dir/$test_path"
    if [[ ! -x "$host_test_path" ]]; then
        printf 'Missing executable test artifact: %s\n' "$host_test_path" >&2
        exit 1
    fi

    printf '\n== fusion native expected failure: %s ==\n' "$executable"
    set +e
    actual_output="$(docker run --rm --platform=linux/amd64 -v "$output_dir:/qxc:ro" "$ubuntu_image" "/qxc/$test_path" 2>&1)"
    actual_status=$?
    set -e
    printf '%s\n' "$actual_output"

    if [[ "$actual_status" -eq 0 ]]; then
        printf 'Expected %s to exit nonzero.\n' "$executable" >&2
        exit 1
    fi
    if [[ "$actual_output" != *"$expected_output"* ]]; then
        printf 'Expected output from %s to contain: %s\n' "$executable" "$expected_output" >&2
        exit 1
    fi

    passed=$((passed + 1))
done <<'CASES'
fusion-native-explicit-panic|Panic: Explicit native PANIC reached
fusion-native-match-default-fail|Panic: MATCH DEFAULT FAIL reached
fusion-native-match-valueless|Panic: MATCH encountered a valueless fusion without DEFAULT
fusion-native-unwrap-wrong-alternative|Panic: UNWRAP expected VARIANT alternative U32
fusion-native-unwrap-valueless|Panic: UNWRAP encountered a valueless VARIANT
CASES

printf '\nAll %d fusion native expected-failure tests passed.\n' "$passed"
