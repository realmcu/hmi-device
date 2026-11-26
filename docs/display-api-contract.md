# Display API Contract

The display stack is OS-neutral.  Zephyr and FreeRTOS only provide the
semaphore and vector-table wrappers; they do not define the display semantics.

```text
HoneyGUI (application/UI)
  DC_DOUBLE / DC_RAMLESS
        |
        v
rtk_display_submit() / rtk_display_wait_complete()
        |
        v
rtk_sdi / rtk_rgb
        |
        v
LCDC / DBIB / eDPI
```

## Application API

```c
int rtk_display_submit(rtk_display_t *disp,
                       const uint8_t *buf,
                       uint32_t len_pixels,
                       uint32_t timeout_ms);

int rtk_display_wait_complete(rtk_display_t *disp,
                              uint32_t timeout_ms);
```

`rtk_display_submit()` is the normal frame submission path.

### SDI / DBIB

```text
wait previous transfer
arm TX_AUTO_DONE
start transfer
return                     current transfer is asynchronous
```

`rtk_display_wait_complete()` can be called explicitly to wait for the current
transfer.  It cleans up LCDC/DBIC after the interrupt.

### RGB

```text
arm TX_AUTO_DONE
switch infinite-DMA buffer address
wait TX_AUTO_DONE
verify CURR_SAR is inside the new buffer
return                     safe to draw the next frame
```

RGB must not return before the address switch is confirmed; otherwise the next
frame can write the buffer that is still being scanned.

## Completion interrupt

All three paths use the same interrupt:

```text
Display_IRQn = 67
LCDC_STATUS_TX_AUTO_DONE_INT
```

The interrupt must be unmasked with:

```c
LCDC_MaskINTConfig(LCDC_INT_MASK_TX_AUTO_DONE, DISABLE);
```

`LCDC_Init()` leaves the mask bits set; `ENABLE` keeps it masked and the
wait will never complete.

## Region writes

| `display_write` input | SDI | RGB |
|---|---|---|
| full frame | `rtk_display_commit()` / `rtk_display_submit()` | same |
| region | `rtk_display_write_region()` | `-ENOTSUP` |

RGB requires a complete framebuffer.  "Region" is a Zephyr transfer term, not a
HoneyGUI PFB buffer mode.

## HoneyGUI buffer modes

| `dc.type` | Meaning | Buffers |
|---|---|---|
| `DC_DOUBLE` | full framebuffer | two full-frame PSRAM buffers |
| `DC_RAMLESS` | PFB / partial framebuffer | two section buffers, GDMA to full frame |

PFB/FULL describe the application's rendering buffer strategy.  They are not
OS-level or controller-level concepts.
