# AGENTS.md

Maintenance notes for coding agents working on this disting NT plugin.

## Required skill context

Use the `disting-nt-cpp-plugin-writer` skill before changing this repo:

- Skill file: `/Users/nealsanche/.codex/skills/disting-nt-cpp-plugin-writer/SKILL.md`
- API reference: `/Users/nealsanche/.codex/skills/disting-nt-cpp-plugin-writer/references/api-reference.md`
- Patterns: `/Users/nealsanche/.codex/skills/disting-nt-cpp-plugin-writer/references/patterns.md`
- Testing: `/Users/nealsanche/.codex/skills/disting-nt-cpp-plugin-writer/references/testing.md`

Follow the skill rules: multiply `numFramesBy4` by `4`, treat bus parameters as 1-based, set `alg->parameters` and `alg->parameterPages` in `construct`, preserve parameter order/GUID unless explicitly asked to break compatibility, and verify hardware plus nt_emu builds.

## Project context

- Plugin name: `Mathulate`
- Plugin source: `mathulate.cpp`
- Testable math core: `mathulate_core.h`, `mathulate_core.cpp`
- Unit tests: `tests/mathulate_core_test.cpp`
- Build file: `Makefile`
- API dependency: git submodule at `distingNT_API`
- Original design: `SPEC.md`, copied from Substrate artifact `46b3ac7e-f350-44a6-8c6e-fc5af8f71be8` (`Disting NT Math Function Plugin Spec`)
- GUID: `NT_MULTICHAR('N', 's', 'M', 't')`
- Tag: `kNT_tagUtility`
- UI version string: injected by `Makefile` as `MATHULATE_VERSION` from `git describe --tags --always --dirty`

## Compatibility invariants

Do not reorder released parameters unless the user accepts preset breakage. Current order:

1. `CV A`
2. `CV B`
3. `CV Out`
4. `CV Out mode`
5. `Equation`
6. `Scale`
7. `Offset`
8. `Clock In`
9. `Speed`
10. `Phase`

The enum values for `Equation` and `Speed` are also preset-facing; append new equations/speeds instead of reordering or renaming existing meanings.

## Version control workflow

Make git commits at each coherent stage of work. Do not leave a long chain of unrelated edits uncommitted.

Suggested commit boundaries:

- dependency/build setup changes
- plugin behavior changes
- test additions or test-harness refactors
- documentation/agent-instruction updates

Before committing, run the smallest relevant verification command (`make unit` for math-core/test edits; `make verify` before handing off larger plugin changes). Use concise imperative commit messages, for example `Add math core safety tests`.

## Build and verification

From repo root:

```sh
git submodule update --init --recursive
make unit
make hardware
make test
make check
make verify
```

Expected outputs:

- Hardware relocatable object: `plugins/mathulate.o`
- nt_emu desktop library: `plugins/mathulate.dylib` on macOS, `.so` on Linux, `.dll` on Windows
- Native safety test binary: `build/mathulate_core_test`

`make unit` checks every equation for finite raw values, bounded CV output, divide/modulo zero safety, singularity-prone inputs, phase-driven output safety, and Lorenz long-run stability. Spiky/discontinuous outputs are allowed; NaN/Inf and out-of-range final CV are not.

Custom UI policy: keep the draw callback returning `true` so the standard parameter line stays hidden. Tiny text is baseline-aligned; a top-row tiny label should use baseline `y=5`. The graph is phase-shifted by the `Phase` parameter itself, not just by moving the dot. Do not hardcode release versions in `mathulate.cpp`; use the `MATHULATE_VERSION` define supplied by the Makefile.

`make check` reports math-library undefined symbols (`sinf`, `cosf`, `expf`, etc.) and host/API symbols used by the custom UI (`NT_drawText`, `NT_drawShapeI`, `NT_setParameterFromUi`, etc.) for the relocatable/plugin host build; that is expected as long as there are no missing project-local symbols.
