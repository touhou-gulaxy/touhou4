# WIP Stuffs

This directory contains **W**orking **I**n **P**rogress contents, which are not current development baseline but will restore future.

## Combat Effects

The combat effects are based on art resources from the game Touhou Fantastic Danmaku Festival Part II/III, and already get permission to use.

The directories `barrage_resources` and `muzzle_flash` contain the enemy danmuku resource import and the muzzle/windup
entity changes extracted from:

- Base: `9090f82daf96166f1d5732f570fde7b71f3a7dac`
- Tip: `499fb5f40673406d222044acef87f6612fb04631`

The active mod has been restored to the base versions for the modified
definition files. Unrelated beam-origin and Yuyuko material changes from the
tip remain active.

Retained active changes:

- `gfx/models/ships/ent/spth_combat_objects.gfx`
- `gfx/models/ships/ent/th07_yuyuko_fan_normal.dds`
- `gfx/models/ships/ent/th07_yuyuko_fan_specular.dds`
- `gfx/models/ships/th_combat_effects.asset`
- `gfx/particles/magic_fields/spth_marisa_beam_origin.asset`
- `gfx/particles/magic_fields/spth_magic_field_marisa.dds`
- `gfx/particles/spth_mofazhen.gfx`
- `gfx/particles/spth_mofazhen.asset` (whitespace-only normalization)

### Details

#### `barrage_resources/`

Contains 283 files from the enemy barrage asset library:

- 150 DDS textures
- 126 particle assets
- model, entity, GFX, metadata, documentation, and generator files

These files were not referenced by gameplay scripts in `499fb5f4`.

#### `muzzle_flash/resources/`

Contains 44 muzzle/windup-related resources:

- Magic-field and muzzle-flash particle assets
- Sakura stage 6 windup and muzzle-flash assets
- SPARK replacement textures
- GFX registries
- S/M/L muzzle-flash migration generator

### `muzzle_flash/files/`

Contains complete `499fb5f4` versions of every modified definition file:

- `common/component_templates/spth_mutation_weapon_components.txt`
- `common/component_templates/th_weapon.txt`
- `common/component_templates/touhou_non_genso_parts.txt`
- `gfx/models/combat_items/spth_danmuku.asset`
- `gfx/projectiles/thxl_weapons.txt`

#### `muzzle_flash/diffs/`

Contains one per-file diff for each definition and `all.patch`, which combines
all five definitions.

### Restore

Need manual restore in later development.
