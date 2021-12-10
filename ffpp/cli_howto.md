# CLI Howto

Just a cheat sheet of some of the most common CLI commands used in development:

## Meson

Following commands assume that the name of the build directory is `build`.
When `ninja` command is used, it assumes that it's in the build directory.

- Use release build (-O3): `meson --buildtype release build`

- Wipe the current build directory and reconfigure using new options in meson.build: `meson setup build --wipe`

- Set compiler (default: gcc): `CC=clang CXX=clang++ meson build`

- Use `cppcheck`: `ninja cppcheck`

- Run all unit tests: `ninja test`

- Run coverage and get report: Build the project with `meson build -Db_coverage=true`, then `ninja test`, finally `ninja coverage`

- Use sanitizers (e.g. ASAN): `meson <other options> build -Db_sanitize=address`

- Use `clang-tidy` (a static checker, very helpful): `ninja scan-build`

- Link `./compile_commands.json` for clangd language server: `ln -s ./build/compile_commands.json ./compile_commands.json`

## Linux Perf

## Valgrind

- Use Callgrind for profiling: `valgrind --tool=callgrind ./bin`

The KCachegrind from KDE project can be used to visualize the profiling result.

## Gbenchmark C++

- Use filter for benchmarks: `./benchmark --benchmark_filter=regrex`
