# Device Drivers

This repository contains external device drivers grouped by SoC, board,
interface, and device function.

## Branch Model

- `master` contains documentation, branch conventions, and the branch index.
- Driver implementation branches are named from the SoC, board, interface,
  device type, and device-specific attributes.
- Driver code must not be committed directly to `master`.

See [docs/branch-index.md](docs/branch-index.md) for the active branch list and
[docs/branch-naming.md](docs/branch-naming.md) for the naming convention.
