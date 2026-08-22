# Program Startup and Runtime Hooks

Executable outputs connect user code to a runtime module. Most applications
only declare `::main`; runtime authors provide the lower-level entrypoints that
prepare the process and dispatch to it.

## Application entry function

A normal executable selects a zero-argument function returning `I32`:

```quxlang
::main FUNCTION(): I32
{
  RETURN 0;
}
```

The `main` declaration belongs to the logical module selected by the executable
output in `qxcbuild.yml`. It is not the native process symbol itself.

## Runtime entry declarations

The runtime module supplies three reserved declarations as needed:

| Declaration | Required contract | Purpose |
| --- | --- | --- |
| `::PROGRAM_START` | Target-specific `ASM_PROCEDURE` | Native process entry and early CPU detection |
| `::POST_DETECT` | `FUNCTION()` | Initialization after a CPU stepping is selected |
| `::UNIT_TEST_MAIN` | `FUNCTION(): I32` | Entry function for a `unit_test_suite` output |

A native start declaration is selected by platform and architecture:

```quxlang
::PROGRAM_START INCLUDE_IF(OS_LINUX) ASM_PROCEDURE X64
{
  CALL DETECT_CPU_ARCHINFO
  CALL PICK_STEPPING
  // Store the selected stepping and dispatch through POST_DETECT_FUNCTION_ARRAY.
}
```

Runtime entry declarations are runtime integration points, not alternative
spellings for an application's ordinary `main` function.

## Stepping dispatch objects

For an executable with CPU steppings, the compiler exposes:

- `STEPPING_COUNT`, the configured number of steppings;
- `ACTIVE_STEPPING`, the runtime-selected `SZ` index;
- `MAIN_FUNCTION_ARRAY`, an array of `PROCEDURE(: I32)` entries indexed by
  stepping;
- `POST_DETECT_FUNCTION_ARRAY`, the corresponding array of `PROCEDURE()`
  entries; and
- generated `DETECT_CPU_ARCHINFO` and `PICK_STEPPING` procedures.

The bootstrap path chooses the highest compatible stepping, records it in
`ACTIVE_STEPPING`, and invokes the matching post-detection function. The normal
runtime post-detection function then invokes
`MAIN_FUNCTION_ARRAY[ACTIVE_STEPPING]`.

Application code may read `ACTIVE_STEPPING` when it needs to report or inspect
the selected path:

```quxlang
VAR selected SZ := ACTIVE_STEPPING;
```

Instruction selection should usually remain behind runtime
`HAVE_<ATTRIBUTE>` queries and the stepping-specialized function graph rather
than branch manually on the numeric index. LLVM replaces a query with a
boolean constant when the selected compilation stepping fixes that attribute;
otherwise the query reads the detector-populated `_ENABLED` flag.

## Unit-test dispatch objects

A `unit_test_suite` output also exposes:

- `UNIT_TEST_COUNT`, the number of collected `UNIT_TEST` declarations;
- `UNIT_TEST_NAMES`, their names; and
- `UNIT_TEST_PROC`, their procedure table, grouped by stepping.

`::UNIT_TEST_MAIN` can iterate those objects and invoke the entry at
`ACTIVE_STEPPING * UNIT_TEST_COUNT + test_index`. These symbols belong to the
runtime test harness; ordinary tests are still declared with `UNIT_TEST`,
`STATIC_TEST`, or `DUAL_TEST`.

See [CPU capabilities and steppings](cpu-capabilities-and-steppings.md),
[Tests](../tests.md), and
[Assembly procedures](../assembly-procedures.md). The allocator,
diagnostic, initialization-guard, and thread teardown declarations supplied by
the same module are documented under
[Runtime module contracts](runtime-module-contracts.md).
