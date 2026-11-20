---
name: touhou-release
description: Prepare a Touhou mod Workshop release by deriving changelog boundaries from Git history, updating version metadata and history entries, syncing English localisation, and checking release artifacts. Use when assigning a new mod version or producing Workshop update notes.
---

# Touhou Release

Use this skill only when preparing a versioned release or changing the in-game
update history.

## Establish the Version Range

Do not infer the previous release boundary from commit count. Locate the last
commit that changed `spth_version`:

```bash
git log --all --full-history -G 'spth_version:' -- localisation/english/th_l_english.yml
git log --oneline --reverse <previous-version-commit>..<release-commit>
git diff --shortstat <previous-version-commit>..<release-commit>
```

Summarize player-visible changes from commits and final diffs, not from
intermediate WIP commits that were later reverted.

At the 2.5.3 handoff:

- 2.5.2 range: `a2360778..6dd75387`
- 2.5.3 range: `9526e0bd..f5e08448`

## Update All Version Surfaces

Keep these synchronized:

- `descriptor.mod` `version`
- `spth_version`
- `spth_version_date`
- `spth_latest_update_info`
- `spth_history_update_info`
- New `spth_history_update_info_<version>` and `_desc` localisation
- Matching `game_concepts` entry under
  `common/game_concepts/spth_word_concepts.txt`

Ensure the previous version's history entry was added before adding the current
one. Check English and Simplified Chinese separately when not running the bulk
sync.

## Localisation Sync

Run:

```bash
utils/loc/sync_english_localisation.sh --dry-run
utils/loc/sync_english_localisation.sh
```

The script syncs English body text into all non-Chinese locales and rewrites only
the first-line language header. It never touches `simp_chinese`.

The only excluded target is:

```text
localisation/russian/replace/spth_vanilla_repair_l_russian.yml
```

Confirm that file's hash or `git diff` is unchanged after a sync.

## Release Checks

Run:

```bash
git diff --check
utils/loc/sync_english_localisation.sh
```

The second run must report zero created and zero updated files.

Then:

1. Check CWTools diagnostics for changed script files.
2. Launch the release version and confirm it loads.
3. Search `error.log` for changed-file paths and new errors.
4. Mark features as runtime-tested only when their event, edict, UI, or project
   was actually triggered.
5. Inspect Workshop description files in `../touhou_resources/`; remember that
   `desc_zh.txt` is the currently observed Chinese filename.

Do not claim a design target differs from code. Read the final values from the
current commit and mention unresolved discrepancies explicitly.
