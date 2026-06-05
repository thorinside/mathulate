# Mathulate

Mathulate is a disting NT custom C++ plugin generated from the Substrate “Disting NT Math Function Plugin Spec”. It processes two CV inputs as normalized values, runs one of 20 safe math equations, then outputs bounded CV.

## Build and test

This folder expects a disting NT API checkout at `./distingNT_API`, or set `DISTINGNT_API`:

```sh
make unit
make hardware DISTINGNT_API=/path/to/distingNT_API
make test DISTINGNT_API=/path/to/distingNT_API
make verify
```

`make unit` runs dependency-free native C++ safety tests for all 20 math equations.

Outputs:

- Hardware: `plugins/mathulate.o`
- nt_emu desktop: `plugins/mathulate.dylib` / `.so` / `.dll`

## Parameters

- `CV A`, `CV B`: CV inputs, normalized internally from ±10V to ±1.
- `CV Out` + mode: destination CV bus and replace/add mode.
- `Equation`: 20-function math library.
- `Scale`: output gain, 0–200%.
- `Offset`: output DC offset, -10V to +10V.
