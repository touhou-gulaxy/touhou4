---
name: touhou-fleet-performance
description: Diagnose and benchmark Touhou mod fleet-selection UI stutter, especially costs caused by evaluation_resource, ship resource tables, component cost aggregation, and large selected fleets. Use for frame-time work where camera movement or model rendering is not the suspected primary cause.
---

# Touhou Fleet Performance

Use this skill for fleet UI pauses that scale with selected ship count, component
count, weapons, upkeep entries, or design evaluation.

## Separate the Variables

Record:

- Baseline FPS/frame time with nothing selected
- Fleet selected, galaxy-map camera, fleet UI only
- Fleet composition and exact design count
- Whether ships are moving, fighting, taking damage, or in the same system
- Camera zoom and whether ship entities are visible

Reproduce with the same save and camera. UI-only stutter should not be attributed
to model rendering without an A/B test that changes visibility.

## Inspect the Resource Path

`evaluation_resource` makes design or fleet UI evaluate the selected resource.
Do not assume this reads only `cost`; inspect the complete ship resource table and
component-derived costs/upkeeps. A hidden resource can still incur expensive
modifier aggregation or produce missing-icon warnings.

Current optimization helpers:

```text
common/inline_scripts/ship_sizes/touhou_optimize_evaluation_resource.txt
common/inline_scripts/ship_sizes/touhou_optimize_evaluation_resource_upkeeps.txt
```

Placement invariants:

- `evaluation_resource` is a ship-size top-level field.
- The upkeep cancellation helper belongs at the end of `resources = { ... }`.
- Do not insert it inside a cost/upkeep context or after another inline script
  where it becomes part of the wrong block.
- The helper currently adds `touhou_ship_combat_power = 1` and
  `-1` under `exists = owner`. Verify owner scope for special ship sizes such as
  growth stages.

## Useful A/B Tests

Change one variable at a time:

1. Remove component cost contribution with `components_add_to_cost = no`.
2. Remove alloy cost only.
3. Remove energy upkeep only.
4. Replace `evaluation_resource` with a hidden resource that has no component
   aggregation.
5. Keep ship visuals and camera identical.

Record FPS and frame time, not only perceived smoothness. Preserve profiling logs
before restarting because the game may truncate them.

## Profiling Limits

The script profiler misses many allocations and does not create a complete
per-shot `on_hit` root. Do not use it alone for:

- `CEventScope` construction and copies
- General heap allocation
- Full weapon `on_hit` costs
- Unprofiled C++ modifier/resource recomputation

Use the injected report or Linux `perf` when function-level attribution is needed.
The injected report path is in `.agent/local_config.json`.

## Existing Hot Paths

Start with these known high-frequency paths when investigating battle or
large-fleet lag:

- `events/spth_fleet_events.txt`
  - `spth_fleet.2042`: dynamic regeneration and Aura/Yuyuko coverage refresh
  - `spth_fleet.2045`: fleet-power-gap fire-rate refresh
  - `spth_fleet.1002` / `.1104`: ship disabling and extra-life handling
  - `spth_fleet.1202`: kill-driven butterfly-stack updates
- `common/scripted_effects/spth_misc_effects.txt`
  - `touhou_stop_global_modifier_system`
  - `touhou_resume_global_modifier_system`
- `common/component_templates/`
  - weapon `on_hit`, `triggered_ship_modifier`, and dynamic component costs

Batch modifier updates must wrap the whole operation between the stop/resume
helpers. Do not nest the global batch state or leave it open on an early return.

## Completion Criteria

Report the measured before/after numbers, exact fleet composition, camera/UI
conditions, changed script path, and any new resource or modifier icon warnings.
