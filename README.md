# Touhou Stellar Chronicles ~ Symphony of Phantasm

A large gameplay mod for Stellaris 4.5 centered on Touhou Project species,
leaders, events, ships, weapons, spiritual-power industries, and crisis chains.

This repository is the mod source itself. It does not require a compiler or a
separate build step; the game reads the `.txt`, `.yml`, `.gfx`, `.asset`, and
`.gui` files directly.

## Requirements

- Stellaris `v4.5.*`
- Any separately listed mod dependencies used by the gameplay files
- CWTools and Stellaris Language support are recommended for editor validation

Current mod metadata is in [`descriptor.mod`](descriptor.mod).

## Repository layout

- `common/` - game definitions, components, ships, technologies, rules,
  modifiers, scripted effects, and other data-driven content
- `events/` - event chains, fleet systems, crises, and dynamic combat logic
- `localisation/` - localisation files kept in the main mod repository
- `gfx/`, `interface/`, `sound/`, and `music/` - presentation assets
- `utils/` - code generators and maintenance scripts
- `backup/` - local backups and reference copies; do not treat as active content

## Companion resources

Source artwork, audio, Workshop descriptions, and partially translated English
localisation are stored in the sibling repository:

```text
../touhou_resources/
```

Workshop description files use this convention:

- `desc.txt` - English Workshop item information
- `desc_cn.txt` - Chinese Workshop item information
- First line: Workshop item name
- Following blank line(s): the Workshop item description

The checked-in resource tree currently contains `desc_zh.txt` rather than
`desc_cn.txt`; treat `desc_cn.txt` as the documented canonical name.

`../touhou_resources/utf8/` contains partially translated English localisation
files. Preserve localization keys, placeholders such as `$name$`, formatting
such as `§Y...§!`, localization commands, and line breaks when translating.

See [.agent/HANDOFF.md](.agent/HANDOFF.md) for engineering notes, performance
investigation paths, and known pitfalls.

## Development

Typical validation:

```bash
git diff --check
```

Also run the CWTools diagnostics configured in `.vscode/`, then launch the mod
and inspect:

```text
~/.local/share/Paradox Interactive/Stellaris/logs/error.log
```

There is no repository-wide automated test suite. Gameplay changes should be
tested from a suitable save, with particular attention to event scope,
localization placeholders, modifier recursion, and combat performance.

## Utility scripts

Examples:

```bash
./utils/gen_component_vars.sh --dry-run
./utils/ships/gen_ags_psionic_variants.sh --dry-run
```

Generated outputs should be reviewed before committing.

## Performance analysis

Useful local references outside this repository:

- Game installation:
  `/home/koishi/SteamLibrary/steamapps/common/Stellaris/`
- Decompiled 4.5 source:
  `/home/koishi/ghidra_decompiled/stellaris4.5/`
- Injection-based performance report:
  `/home/koishi/ghidra_decompiled/stellaris4.5/perf/pdxperf-report.txt`

Stellaris script profiling is useful for event, trigger, and script-value
hotspots, but it is not a complete C++ profiler. In particular, weapon
`on_hit` execution, scope construction/deep copies, and allocator activity
must be verified with `pdxperf`, `perf`, or controlled A/B tests.

