---
name: triggered-modifier
description: Design, audit, and migrate Stellaris triggered modifier blocks, including potential/possible gates, greyed inactive tooltips, trigger key overrides, fix_key formatting, and dummy grey modifier replacement. Use when deciding whether a conditional modifier should hide or explain its inactive state.
---

# Stellaris Triggered Modifier

Use this skill for `triggered_modifier`, `triggered_country_modifier`,
`triggered_planet_modifier`, and the other `triggered_*_modifier` block types.
The goal is to preserve gameplay semantics while producing a tooltip that tells
the player why a modifier is active or inactive.

## Establish the Target Version

The detailed tooltip fields in this skill are 4.6 behavior. Stellaris 4.5 does
not recognize all of them.

- Keep 4.5 release files on the old branch.
- Use `potential`-only behavior for 4.5.
- Apply `possible`, `show_*`, and override keys only on the 4.6 branch.
- Do not change `supported_version` merely because a file parses.

Authoritative references:

- `common/README_TRIGGERED_MODIFIERS.txt` in the target game.
- Nearby vanilla 4.6 examples in `common/traits`,
  `common/governments/civics`, `common/governments/councilors`,
  `common/resolutions`, and `common/ideologies/tenets`.

## Understand the Gates

For an ordinary triggered modifier, `potential` controls both application and
whether the block is normally visible.

For the complex triggered modifier used by 4.6 tenets:

- `potential` is the hard context/visibility gate.
- `possible` is the current applicability gate.
- Modifiers apply only when both are true.
- A false `potential` hides the block even when a `show_*` field is enabled.

Do not add `possible` to every block. Use it only for a database/context that
parses the complex triggered modifier model. Otherwise use `potential`.

All triggers run in the scope supplied by the parent block. Confirm whether it
is country, planet, colony, sector, fleet, leader, or pop-group scope before
copying a condition from another modifier type.

## Tooltip Policy

Choose one policy deliberately:

| Player need | Fields |
|---|---|
| Hidden entirely while inactive | No display field |
| Active effect without an explanation | No field; active modifiers display normally |
| Explain why the active effect applies | `show_trigger_if_true = yes` |
| Show an inactive effect in grey | `show_if_trigger_false = yes` |
| Show the failed condition | `show_trigger_if_false = yes` |
| Show the complete grey entry | Both `show_if_trigger_false = yes` and `show_trigger_if_false = yes` |
| Replace the active condition text | `trigger_true_key_override = <loc_key>` |
| Replace the inactive condition text | `trigger_false_key_override = <loc_key>` |

Key invariants:

- `show_trigger_if_false` shows the failure condition, not the modifier values.
- `show_if_trigger_false` shows the modifier values, not the failure reason.
- `trigger_true_key_override` automatically enables the true condition display.
- `trigger_false_key_override` automatically enables the false condition display.
- Overrides take a localization key, not literal text.
- In a complex triggered modifier, the show fields describe `possible`;
  `potential` remains a hard hide gate.

Use both show flags when converting a dummy grey line: the player expects to see
the value and why it is inactive.

## Dummy Grey Migration

A dummy grey pattern usually consists of:

1. A real triggered modifier for the active branch.
2. Extra modifiers named like `<actual_modifier>_dummy_grey`.
3. A separate scripted modifier whose localization wraps the real modifier name
   in grey formatting.

Migrate only when all of these are true:

- The target block type is known to support detailed triggered-modifier tooltips.
- The replacement block has the real modifier key and value.
- `potential` is a simple, stable activation condition.
- The desired result is "show inactive with reason", not a fundamentally
  different tooltip or different gameplay scope.
- The mod is on a branch targeting 4.6 or later.

Migration shape:

```text
triggered_country_modifier = {
    potential = {
        <real activation condition>
    }

    show_if_trigger_false = yes
    show_trigger_if_false = yes

    <actual modifier key> = <value>
}
```

Then remove the dummy modifier definition from `common/scripted_modifiers` and
its `_dummy_grey` localization only after confirming there are no remaining
users.

Do not mechanically migrate when:

- The dummy sits in a different trigger scope from the real effect.
- One real modifier is replaced by several different dummy keys or values.
- The branch selects between incompatible state machines, not a simple boolean.
- The block type has no confirmed support for trigger details.
- The current mod release must still run on 4.5.
- The dummy is injecting text or behavior that `show_*` cannot reproduce.

## `fix_key` and Multipliers

Use `fix_key` when automatic modifier wording is too generic:

```text
triggered_country_modifier = {
    country_society_research_produces_mult = 0.01
    mult = value:species_in_empire
    fix_key = species_in_empire
    should_mult_desc = no
}
```

Localization placeholders:

- `$NAME$`: localized modifier name.
- `$VALUE$`: formatted modifier value.
- `$MULT$`: multiplier value.

`fix_key` changes presentation only. It applies to each modifier in the block.
When `mult` or `multiplier` is present, the engine can use the
`<fix_key>_mult_desc` or `<fix_key>_mult_desc_plural` localization variant.

`should_mult_desc` defaults to `yes`. It controls the automatic multiplier
description; it does not change effect calculation. Set it to `no` when the
custom `fix_key` text already explains the per-unit calculation and an automatic
multiplier description would be redundant.

## Audit Workflow

When asked which modifiers can be migrated:

1. Locate every active `*_dummy_grey` use in triggered blocks, not merely the
   scripted modifier definitions.
2. Group uses by parent definition, trigger type, scope, and condition.
3. Compare the dummy values with the corresponding real modifier values.
4. Classify each group as direct migration, needs scope redesign, unsupported
   block type, or retain unchanged.
5. Identify dummy definitions and localization keys that become dead after
   migration.
6. Report migration candidates and non-candidates separately. Do not edit files
   unless the request also asks for implementation.

Preferred migration candidates usually have:

- Mutually exclusive conditions such as leader class, season, origin, or policy.
- One real modifier and one or more dummy copies in the same block.
- Existing custom tooltips that already explain the inactive state.

Higher-value ordinary conditional blocks that do not yet use dummies are also
candidates when:

- The effect is important but currently disappears without explanation.
- The condition is understandable to the player.
- Showing the inactive effect will not create a misleading gameplay promise.

Avoid converting every hidden condition. Internal cleanup modifiers, AI-only
checks, performance switches, and implementation details should remain hidden.

## Validation

Run CWTools for the target game version, then test in game:

- Active condition: modifier applies and its explanation appears as intended.
- False condition: effect does not apply and the entry is greyed when requested.
- Scope changes: no invalid scope error occurs in `error.log`.
- Localization: override keys, `fix_key`, and multiplier variants resolve.
- Removal: no remaining `_dummy_grey` uses or orphaned localization after
  cleanup.
- Branch isolation: 4.5 still loads if that version remains supported.

Report runtime-unverified tooltip behavior explicitly. Static parsing cannot
prove final layout, coloring, or icon presentation.
