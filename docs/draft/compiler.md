# Quxlang Compiler `qxlc`

qxlc <source directory> <output directory> [options]


## Options:

--cache, -c Location of cache directory.
--logs, -l Location of log directory.
--threads, -t Number of threads to use for compilation (default: number of hardware threads).
--help, -h Show this help message and exit.
--dump=(always|error|never|success) Dump the query graph to a file in the logs directory. 
    "always" dumps on every compilation, 
    "error" dumps only if an error occurs,
    "success" dumps only if compilation is successful
    "never" never dumps, and  (default: never).

The compiler has the following documented exit codes:

0: Compilation successful
1: Compilation failed due to a compilation error
2: Compilation failed due to a _compilation exception_, not suspected to be a compiler bug
3: Compilation failed due to a _compilation exception_ caused by a compiler bug
137: Killed, likely due to exceeding memory limits or manual termination

Return codes of 0 or 1 produce _canonical results_.

A compilation error is a cachable failure, and indicates that the source code is invalid in some way. A report is produced
which contains details about the error, and an exit code of 1 indicates the compilation error report was successfully
written out.

A compilation exception indicates that an abnormal condition caused compilation to fail, such as an I/O error reading
file data, or out of memory condition. This result is not canonical.

Given infinite memory, there is no guarantee that a compilation attempt will eventually terminate. However, the compiler 
is guaranteed to eventually terminate or OOM given finite memory as the compiler is guaranteed not to run infinite loops
without allocating more memory. 