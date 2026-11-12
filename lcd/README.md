# LCD Auto-Detect Driver

This directory is reserved for the M8102301 QSPI LCD auto-detect driver.

See [auto-detect-360x360.md](auto-detect-360x360.md) for the current design,
supported panel IDs, initialization dispatch, and validation notes.

Expected components:

- Common QSPI/DBIC transport and panel ID detection.
- ST77916 360x360 initialization sequence.
- GC9B71 BOE 1.389-inch 360x360 initialization sequence.
- Build integration and panel ID dispatch.

Do not add board pinmux or product configuration to this directory unless it is
part of the reusable LCD driver contract.
