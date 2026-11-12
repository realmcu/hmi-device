# RTL87x3G M8102301 QSPI LCD Auto-Detect 360x360

This branch contains the M8102301 board-level QSPI LCD auto-detect
integration for 360x360 panels.

## Target

- SoC: RTL87x3G
- Board: M8102301
- Interface: QSPI
- Display resolution: 360x360
- Driver mode: read the panel ID before initialization and dispatch to the
  matching panel initialization sequence

## Supported Panel IDs

| Panel | Normalized ID |
| --- | --- |
| ST77916 | `0x003F7F4F` |
| GC9B71 | `0x004D387F` |

The driver tries both supported DBIC read configurations because the panels do
not use the same read mode.

## Layout

- `lcd/`: LCD auto-detect implementation and panel initialization data.

The implementation currently originates from `HoneyRepo/driver/display` and
will be migrated here without creating a second independently maintained copy.
