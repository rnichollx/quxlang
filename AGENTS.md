# Agents

1. Follow general code quality guidelines at https://gitlab.com/rpnx/rpnx-coding-standards when writing code.
2. Quxlang QXC is a fully deterministic, hermetic, and reproducible cross compiler, don't introduce any code that changes those properties.
3. Put temporary test files in a directory named `tmp` in the root of the repository, unless you've been asked to put them elsewhere. This directory is already in `.gitignore`.
4. Please ensure that your code adheres to the Zero Overhead Principle. We generally prefer undefined behavior over runtime checks that introduce significant overhead.
5. Quxlang is usually built by running `csetup download` to get dependencies, then `cbuild -c Release` from the `misc/build` directory. If the tool is not installed, you can get it at https://gitlab.com/rpnx/cbuild-go, usual commands to download it is something like `go install gitlab.com/rpnx/cbuild-go/cmd/cbuild@preview && go install gitlab.com/rpnx/cbuild-go/cmd/csetup@preview`. If that doesn't work, try "latest" instead of "preview". CMake, Ninja, Git, and a C++ compiler are also required.
6. LLVM will take a long time to build, try not to use `cbuild clean` as that will delete the LLVM build directory. Instead when needed clear only Quxlang's build directories using `cbuild clean -t quxlang`.
7. `cbuild test` will run tests, but it will do so in many different build configurations, you might want to use `cbuild test -c Release` to just run Release configuration tests. It will auto-rebuild any changed targets.
8. Prefer not to add new GTEST cases in .cpp files if the same thing can be tested from a Quxlang STATIC_TEST, UNIT_TEST, or DUAL_TEST. Prefer DUAL_TEST unless you have a reason to use another kind of test.
9. It's recommended to do end-to-end validation by running qxc against the testbundle and then running the resulting test executable for the current OS/platform combination.
