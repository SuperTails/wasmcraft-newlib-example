# Wasmcraft + Newlib

*This example is part of the [Wasmcraft Suite](https://github.com/SuperTails/wasmcraft2).
See the link for more details on the project.*

Most software one would want to compile using Wasmcraft is not freestanding,
i.e. it needs resources and functions provided by an operating system.
For example, `printf` and `malloc` are provided by the C standard library, and so are not available by default.

This project demonstrates how to include the [newlib](https://sourceware.org/newlib/)
C standard library implementation in a project so that Wasmcraft code can use the C standard library.

## Layout

### `main.c`

This contains code that uses functions like `printf`, `open`, and `malloc`
to demonstrate that the standard library is usable and works properly.

### `stubs.c`

This contains the glue code that allows newlib to work in Minecraft.
It is divided into a few sections which can be modified to work with various different programs.

## Usage

To build the example, run `build.sh`.
This will generate `newlib_example.wasm` and `newlib_example.wat`.

`newlib_example.wasm` can then be used in [Wasmcraft](https://github.com/SuperTails/wasmcraft2) directly
or in [the simulator](https://github.com/SuperTails/wasmcraft-simulator) to see the results.