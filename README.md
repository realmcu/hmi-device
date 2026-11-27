# Device Drivers

This repository contains external device drivers grouped by SoC, board,
interface, and device function.

## Phase 1 Layout

```text
device/
├── output/display/       # shared LCDC core/controllers, SoC ports, panels/boards
├── input/                # input Kconfig; touch core/controllers/ports
├── zephyr/               # DTS bindings; display/key Zephyr adapters
├── legacy/               # legacy SoC drivers
└── docs/                 # architecture docs and CI
```

## Shared display controllers

8762G, 8773E and 8773G share the LCDC SDI/RGB device implementation with separate
clock/Pad/IRQ ports. Boards remain unchanged. See
[output/display/README.md](output/display/README.md) for selection, supported
clocks, SDK contracts and validation limits.

## Input/display independence

The device module registers its own input Kconfig. Touch and key/button builds
can be enabled with `CONFIG_REALTEK_DISPLAY=n`; only display devices link the
display hardware module. Existing target and per-driver config names are
retained. See [input/README.md](input/README.md) for touch layering, standalone
configuration and validation.

## Branch Model

- `master` contains documentation, branch conventions, and the branch index.
- Driver implementation branches are named from the SoC, board, interface,
  device type, and device-specific attributes.
- Driver code must not be committed directly to `master`.

See [docs/branch-index.md](docs/branch-index.md) for the active branch list and
[docs/branch-naming.md](docs/branch-naming.md) for the naming convention.
