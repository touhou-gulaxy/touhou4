---
name: stellaris-script-verification
description: Verify Stellaris script changes in the Touhou mod when editing events, on_actions, inline scripts, special projects, edicts, scopes, or localisation-linked triggers. Use after script edits where syntax alone cannot prove gameplay semantics.
---

# Stellaris Script Verification

Use this workflow for changes that affect event scope, trigger timing, inline
script expansion, special-project requirements, edicts, or overwrite load order.

## Establish Context

1. Read `.agent/HANDOFF.md` and `.agent/local_handoff.md`.
2. Run `git status --short` and inspect nearby upstream examples in the vanilla
   game files before changing a shared pattern.
3. Read `.agent/local_config.json` instead of hard-coding machine paths.

## Validate in Layers

1. Run CWTools Stellaris Language Server diagnostics for syntax, unknown fields,
   and invalid references.
2. Treat CWTools as incomplete for `inline_script` scope. It commonly reports
   `unknown`; verify actual scope in vanilla documentation or the decompiled
   source.
3. Launch the target game version with a relevant save.
4. Search `error.log` for the edited file paths, invalid scope, missing effects,
   divide-by-zero, duplicate IDs, and missing localisation.
5. Trigger the feature in game. A clean load does not validate event timing,
   UI layout, cooldowns, or resource totals.

For decompiled behavior, use the path under
`extern_paths[key=path.game.decompile]` in `.agent/local_config.json`.

## Non-Obvious Semantics

- `triggered_requirement.trigger` executes in Fleet scope.
- `triggered_requirement.count` counts qualifying adjacent Fleets, not ships.
- In Fleet scope, `any_owned_ship` iterates ships in that Fleet.
- Leader checks in a triggered requirement must be evaluated on the same Fleet.
- Replacing legacy `requirements` with `inline_script` does not remove the old
  block. Both blocks are enforced while both remain active.
- An empty inline script file is meaningful. It expands to nothing without a
  missing-script error.
- Dynamic inline-script filenames are case-sensitive in practice. Use lowercase
  IDs and ensure every selectable file exists.
- An edict's `allow` is checked while the edict is active. A state placed in
  `allow` can terminate the current edict; use `has_edict` and a separate stop
  flag when cooldown must only block future activation.
- Script conditions do not guarantee short-circuit scope protection. Put
  ship-only operations under an explicit ship scope instead of relying on one
  `if` limit.

## Special Projects

The current reusable requirement helpers are in:

```text
common/inline_scripts/special_projects/
```

Use `touhou_type` values whose modifier is
`ship_is_<touhou_type>`. Keep the leader-class and skill sub-scripts aligned with
the enabled `*_type` switch. Add a marker to every ship size or component that
grants the equivalent civilian capability.

Do not mechanically apply the civilian helper to military fleet-power,
`size_requirement`, station, army, or other unrelated requirement types.

## Completion Criteria

- CWTools has no new target-file errors.
- The game loads after the file's modification time.
- The relevant feature was executed or explicitly marked runtime-unverified.
- Every remaining log warning is recorded with its owning file and confidence
  about whether it is harmless.
