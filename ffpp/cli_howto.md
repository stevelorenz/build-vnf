# CLI Howto

Just a cheat sheet of some of the most common CLI commands used in development:

## Meson

- Use release build: `meson setup build --buildtype release`

- Wipe build directory and reconfigure using new options: `meson setup build --wipe`

- Set compiler: `CC=mycc meson setup build`

- Use `cppcheck`: `ninja cppcheck`

- Run unit tests: `ninja test`

- Use sanitizers (e.g. ASAN): `meson <other options> -Db_sanitize=address`

- Use `clang-tidy`: `ninja scan-build`

- Link `./compile_commands.json` for clangd: `ln -s ./build/compile_commands.json ./compile_commands.json`

## Linux Perf

