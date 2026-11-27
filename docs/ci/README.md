# Phase 1 panel registry and CI matrix

`registry/panels.json` is the single source of truth for the 8773G SDI and RGB
panels.

## Generated files

Do not edit these by hand; run:

```bash
python modules/device/output/display/registry/panel_registry.py --generate
```

| Generated file | Consumer |
|---|---|
| `registry/Kconfig.panels` | display `Kconfig` via `orsource "../device/output/display/registry/Kconfig.panels"` |
| `registry/panel_registry.cmake` | `output/display/core`, `output/display/panel`, `zephyr/display` |
| `registry/panel_sources.py` | compatibility output; no current consumer |

`panels.json` entries carry `interface: "SDI"` or `"RGB"`. The generator emits
one source list for both, plus separate SDI/RGB lists and selection guards.

## Validation

```bash
python modules/device/output/display/registry/panel_registry.py --check
```

This catches:

- Kconfig/CMake/SConscript registration drift;
- stale generated files;
- missing panel source files;
- legacy entries missing from `legacy/CMakeLists.txt`;
- manifest duplicates;
- invalid or duplicate interface entries.

Use `--strict` to also fail on legacy Kconfig panel symbols that are not in the
manifest.

## Build matrix

```bash
python modules/device/docs/ci/panel_matrix.py --all --jobs 1
```

For each `build: true` panel it creates a temporary Kconfig fragment, selects
that panel, disables the other manifest panels, and runs:

```bash
west build -b rtl87x3g_sample -p -d build_ci/<id> -- \
  -DENABLE_GENERATE_VERSION=OFF -DEXTRA_CONF_FILE=build_ci/<id>/panel_fragment
```

Logs are kept in `realtek-app/samples/honeygui/build_ci/<id>.log`.
Use `--panel <id>` for a subset and `--dry-run` to inspect commands.

RGB entries also use `build: true`; `board_display_new()` selects the RGB
constructor from the target panel Kconfig symbol, so the matrix exercises the
RGB constructor and controller link path. The sample `prj.conf` still selects
SDI by default when no RGB panel symbol is set.

## Jenkins

See `device/ci/panel-matrix.jenkinsfile`. It assumes the current directory is
the west workspace root (the directory containing `modules/`, `realtek-app/`,
`zephyr/`), and that `west` and the Zephyr toolchain are available.

## Current status (Phase 1)

- `panel_registry.py --check`: 7/7 registered (5 SDI, 2 RGB).
- SDI panels: SH8601Z 410x502 / ST7801N / ST77916 / GC9B71 / auto.
- RGB panels: EK9716 800x480 / JD9165BA 1024x600.
- Matrix: EK9716 and JD9165BA RGB builds pass and link their constructors.
- Legacy RGB/SPI/8080 symbols remain in display `Kconfig` for non-migrated
  drivers; they are reported by `--check` but are not part of this manifest.
