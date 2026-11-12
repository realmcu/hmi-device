# 360x360 QSPI LCD Auto-Detect

## Scope

This document describes the RTL87x3G M8102301 QSPI LCD auto-detect
implementation for 360x360 panels.

The driver reads the panel ID before running any panel-specific initialization
sequence, then dispatches to the matching panel initialization code.

## Configuration

Enable the driver with:

```conf
CONFIG_REALTEK_LCD_AUTO_360_360_QSPI=y
```

The existing `CONFIG_REALTEK_LCD_ST77916_360_360_QSPI` configuration is a peer
configuration and must remain disabled when the auto-detect driver is enabled.

## Panel ID Read

The driver uses DBIC user-mode reads with command `0x04` (`RDDID`):

1. Request four bytes.
2. Discard the first byte as the dummy byte.
3. Treat the remaining ID1/ID2/ID3 bytes as a 24-bit big-endian ID.
4. Mask bit 7 of each ID byte.
5. Return the packed `uint32_t` value.

The panels use different DBIC read configurations:

| Panel | `so_dnum` | Normalized ID |
| --- | --- | --- |
| ST77916 | `1` | `0x003F7F4F` |
| GC9B71 | `0` | `0x004D387F` |

The driver first tries `so_dnum=1`, then retries with `so_dnum=0` when the first
read returns zero. If `0x04` still returns zero, it falls back to commands
`0xDA`, `0xDB`, and `0xDC`.

## Initialization Dispatch

```text
0x003F7F4F -> ST77916_INIT_CODE()
0x004D387F -> GC9B71_INIT_CODE()
```

No panel-specific initialization is selected for an unknown ID. The raw
24-bit ID is logged to make new panel support easier to add.

## Initialization Data

Panel initialization sequences are kept in separate text files and included by
the common driver:

- `lcd_st77916_360_360_init_code.txt`
- `lcd_gc9b71_360_360_init_code.txt`

Each sequence calls the common `lcd_auto_write_cmd_data(cmd, data, len)` helper
directly and calls `platform_delay_ms()` directly where the vendor sequence
requires a delay.

The common command buffer supports up to 64 payload bytes. This limit is
required by GC9B71 command `0x6E`, which has 32 data bytes.

## GC9B71 Scan Direction

The default sequence follows the active vendor initialization. For reverse
scan, the following settings must be changed together:

```text
B6 = 00 40
EC = 87
```

Do not change only one of these commands.

## Validation

The implementation was validated on hardware through the COM3 console. Both
panels are detected before initialization and the expected panel sequence is
selected.

ST77916:

```text
[LCD] id: 3F 7F 4F
LCD hardware initialized successfully
```

GC9B71:

```text
[LCD] id (read mode 0): 4D 38 7F
LCD hardware initialized successfully
```

## Source

The initial implementation is tracked in `HoneyRepo/driver/display` change
405182. It should be migrated here without maintaining two independent copies.

## Adding Another Panel

1. Add the normalized 24-bit panel ID.
2. Determine the required DBIC read mode.
3. Add a separate initialization sequence file.
4. Add the ID-to-initialization mapping.
5. Verify the detected ID and panel initialization through the console.
