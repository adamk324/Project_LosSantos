# Architecture notes

## Principles

1. Validate infrastructure before game-specific integration.
2. Use synthetic fixtures instead of pretending the project is a real GTA V port.
3. Keep the compatibility layer explicit and testable.
4. Prefer durable abstractions over one-off hacks.

## Current foundation

The initial codebase introduces:

- a PowerPC instruction decoder capable of identifying common branch and load patterns,
- a basic ARM64 instruction emitter for synthetic validation,
- a memory arena allocator designed to mirror runtime allocation needs,
- a small Android native library skeleton for CI and Android toolchain checks,
- a GitHub Actions workflow set that exercises the host build and Android cross-build path.

## Planned expansion

The next engineering milestones are:

- ELF/Xbox executable loading abstractions,
- a memory manager compatible with PowerPC page semantics,
- a host-side runtime scheduler and synchronization primitives,
- a Vulkan rendering backend and SDL3 window integration,
- audio, controller, and filesystem abstractions,
- more complete recompilation stages with synthetic instruction tests.

## Integration gate

When a valid Xbox 360 GTA V executable is eventually available, the project will move to a dedicated integration stage that validates:

- executable format compatibility,
- import/export table assumptions,
- memory layout assumptions,
- unsupported instructions, and
- runtime behavior differences from the Xbox 360 environment.

This gate is explicit and must remain separate from the generic infrastructure tests.
