# CLI Howto

Just a cheat sheet of some of the most common CLI commands used in development:

## Meson

- Wipe build directory and reconfigure using new options

```bash
meson setup build --wipe
```

- Set compiler: `CC=mycc meson <options>`

- Use `cppcheck`: `ninja cppcheck`

- Use sanitizers (e.g. ASAN): `meson <other options> -Db_sanitize=address`

- Use `clang-tidy`: `ninja scan-build`

- Link `./compile_commands.json` for clangd: `ln -s ./build/compile_commands.json ./compile_commands.json`

## Linux Perf

