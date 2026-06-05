# Mathulate

> It’s alive! Well, technically it’s a relocatable ARM object file, but let’s not spoil the moment.

Mathulate is a disting NT custom C++ plugin for the Eurorack experimentalist who has looked at two control voltages and thought, “Yes, but what if they had a brief, ill-advised encounter with trigonometry?”

Feed it two CV signals. Mathulate normalizes them, subjects them to one of 20 equations from the slightly humming laboratory cabinet, applies scale and offset, and then politely clamps the result to the Disting NT-safe range of `-10V` to `+10V`. No NaNs, no infinities, no divide-by-zero goblins. Just modulation with a lab coat, safety goggles, and an improbability budget.

## What it does

Mathulate turns clocked or free-running phase into one mathematically persuaded CV output using:

- arithmetic: add, subtract, multiply, safe divide, modulo
- periodic functions: sine, cosine, safe tangent
- shaping: absolute value, square, sigmoid, tanh, exponential, logarithm, soft clip
- charmingly suspicious behavior: Chebyshev, Lorenz, modulo folding, sinc, Gaussian

Some functions are smooth. Some are spiky. Some may appear to have been invented during a thunderstorm by someone shouting “SCIENCE!” at an oscilloscope. This is intentional.

## Build and test

First, make sure the disting NT API submodule is present:

```sh
git submodule update --init --recursive
```

Then summon the machinery:

```sh
make unit
make hardware DISTINGNT_API=/path/to/distingNT_API
make test DISTINGNT_API=/path/to/distingNT_API
make verify
```

`make unit` runs dependency-free native C++ safety tests for all 20 equations. These tests are less concerned with whether a function is tasteful and more concerned with whether it remains finite, bounded, and unlikely to melt the tea trolley.

Outputs:

- Hardware: `plugins/mathulate.o`
- nt_emu desktop: `plugins/mathulate.dylib` / `.so` / `.dll`

Tagged pushes matching `v*` trigger `.github/workflows/release.yaml`, which builds/tests the plugin and publishes `mathulate-plugin.zip` containing:

```text
programs/plug-ins/mathulate/mathulate.o
```

The version shown on the custom UI is injected at build time from `git describe --tags --always --dirty` via the Makefile. Tagged release builds therefore display the release tag.

## Parameters

- `CV A`: optional phase modulation input, normalized internally from ±10V to ±0.5 phase turns. Set to `None` for unmodulated internal/free-running phase.
- `CV B`: optional second equation input, normalized internally from ±10V to ±1. Set to `None` for 0V.
- `CV Out` + mode: destination CV bus and replace/add mode.
- `Equation`: selects the experiment currently being performed.
- `Scale`: output gain, 0–200%.
- `Offset`: output DC offset, -10V to +10V.
- `Clock In`: optional clock CV input. Leave at `0` for internal/free-running speed.
- `Speed`: free-running rate, or clock division/multiplication when clocked (`/16` through `x8`, plus `/24`, `/48`, `/96`).
- `Phase`: phase offset, 0–100%, shifting both the rendered graph and the modulation phase.

## Custom UI

Mathulate takes over the front-panel controls:

- left encoder: equation
- right encoder: speed / clock division
- left pot: scale
- center pot: offset
- right pot: phase offset
- pot buttons: reset their matching pot parameter
- left encoder button: jump to the next equation category
- right encoder button: reset phase

The display suppresses the standard parameter line and shows `MATHULATE`, the version number, clock/internal status, a phase-shifted graph of the selected equation, and a dot traveling along the curve. Think of it as an oscilloscope that has read too many popular science magazines.

## Safety notes from the lab

Mathulate’s core is tested to ensure:

- all 20 equations produce finite values
- final output stays within `[-10V, +10V]`
- safe divide and modulo survive zero and near-zero denominators
- log, sinc, tangent, and Lorenz do not wander off into numerical hyperspace

Clicks, spikes, repetitions, and goofy behavior are allowed. Crashes and infinities are not. This is, after all, a respectable establishment, even if someone did leave the Lorenz attractor running overnight.

## License

MIT. See `LICENSE`.
