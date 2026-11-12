# Branch Naming

Use lowercase ASCII names separated by hyphens:

```text
<soc>-<board>-<interface>-<device-type>-<device-attribute>[-<resolution-or-signal>]
```

Examples:

```text
rtl87x3g-m8102301-qspi-lcd-auto-detect-360360
rtl87x3g-m8102301-i2c-gsensor-bma253
rtl87x3g-m8102301-i2c-gsensor-sc7a20-int
```

Rules:

- Use one SoC spelling consistently within a repository.
- Include the controller or sensor model when the device type alone is not
  unique.
- Include the bus type for external devices.
- Use `360360` rather than `360x360` in branch names for compatibility with
  existing Realtek configuration names.
- Keep the branch name stable after other manifests or projects reference it.
