# Deterministic Development

The compiler should produce the same output given the same input, regardless of the environment in which it is run.


## The Workflow

Quxlang is designed to allow you to start coding, not require you to go futzing about with installed libraries, toolchains, and package managers. With that in mind, compiling a Quxlang program has exactly three steps:

1. Obtain the `qxc` compiler version of your choice.
2. Download the program sources into a directory.
3. Run the `qxc` compiler against those sources.

The `qxc` compiler does not have options that affect compilation mode or optimization, it does not read from system libraries or installed packages.

If you want to update a package to a new version, you need to copy the new version into your source bundle directory.

## The `quxbuild.yaml` file

Every option you need to control compilation mode is never a command line option to `qxc`, instead such options go into the `quxbuild.yaml` file.

There are some downsides to this approach, it is not possible to blindly point `qxc` to a hello world program and get a working binary you can run. At a minimum, you need to know the platform and architecture you want to run the program on, and create a target in the `quxbuild.yaml` file with the appropriate values for platform and cpu.

`qxc` has no concept of the "native" cpu or platform. As a result, setup of the build file is a bit more complex initially. But as a consequence, the program can be always compiled by anyone else with the same source code and compiler version (assuming adequate RAM/storage) and they will obtain binaries that are identical to the ones you produce. No more "it compiles on my machine". 

## `qxc` compiler flags

The `qxc` binary may have some compiler flags which control things like how errors are reported, how many threads it will use, or how long it may attempt to compile something before giving up. It's worth noting that `qxc` has separate concepts of a "compilation error" and a "compilation exception". Error results are canonical failures, "compilation exceptions" however can be non-deterministic. Regardless of the options you set, if you get a "compilation success" or "compilation error", it will always be identical given the same compiler version and source bundle. I.e. compilation success and compilation errors are canonical results which may be permanently cached, but _exceptions_ include non-deterministic failures like _timeout_ or _out of memory_ situations which may be retried.

`qxc` intentionally rejects providing an option like `-fconstexpr-steps=N` in favor of timeouts or memory limits as this might encourage developers to treat such values as canonical failures. If any deterministic limits are ever added, they will be added as build file options or source code annotations and made into canonical errors instead.

## The Halting Problem

Quxlang explicitly does not guarantee that every program has a canonical compiled result, because to do so would require either solving the Halting Problem or setting arbitrary resource limits. Such arbitrary limits cause endless bickering over what the "reasonable" limit should be, and no consensus can exist, given people have different computers with different amounts of RAM and processing power. 

`qxc` will halt when provable cases of infinite recursion are detected, but not every case can be detected.

