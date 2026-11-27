---
name: gdb-debug
description: Diagnose Stellaris Touhou mod startup, main-menu, save-load, and event-time CTDs on Linux with GDB, crash reports, minidumps, and Paradox logs. Use when a CTD needs phase identification, stack/register inspection, script-to-stack correlation, or verification that a crash fix is effective.
---

# GDB Debug Stellaris Mod CTDs

Diagnose the crash from evidence, not from the last harmless `error.log` warning.
A successful main-menu load, a clean script parse, and an in-game feature working
are different verification levels.

## First Capture a Reproducible Baseline

1. Read `.agent/local_handoff.md` and `.agent/local_config.json` for current
   paths. Do not assume an old game version or playset.
2. Record:
   - Git branch, commit, and dirty files.
   - Exact game version and binary hash/Build ID.
   - Loaded DLC and mods.
   - Crash directory timestamp and `meta.yml` `LaunchArguments`.
3. Copy the live `game.log` and `error.log` to `/tmp` before launching. Stellaris
   overwrites them.
4. Preserve the newest crash directory separately. Its logs are the snapshot
   nearest to the crash.

The existing launcher is `utils/automation/run-game.sh`. It handles the required
Workshop dependency, optional mods, the repository mod, and `--continuelastsave`.
Use `-d` when the current `dlc_load.json` already represents the failing playset.

## Identify the Crash Phase

Start with the crash artifacts:

```bash
ls -lt "$HOME/.local/share/Paradox Interactive/Stellaris/crashes"
CRASH="$HOME/.local/share/Paradox Interactive/Stellaris/crashes/<timestamp>"
sed -n '1,220p' "$CRASH/exception.txt"
sed -n '1,180p' "$CRASH/meta.yml"
tail -200 "$CRASH/error.log"
tail -100 "$CRASH/system.log"
```

Classify before debugging:

- `SIGSEGV`: invalid memory access. Repeated identical offsets strongly suggest
  recursion, a corrupted container, or dereferencing an unresolved script
  object.
- `SIGABRT`: an assertion or uncaught C++ exception reached `abort()`. The stack
  may show `std::rethrow_exception`, which is the error transport mechanism, not
  necessarily the original bad script.
- Last successful log timestamp:
  - database/script parsing happens before the main menu;
  - game start/on_action failures happen after starting or loading a campaign;
  - `--continuelastsave` means save loading must be part of the reproduction.
- A missing crash directory after one launch proves only that one configuration
  did not reach the same fatal path.

## Reproduce Under GDB

The game is a stripped PIE. Crash offsets such as `stellaris(+0x289dd67)` are
still useful; absolute addresses change with ASLR.

Use a GDB command file so every reproduction captures the same state:

```gdb
set pagination off
set confirm off
set print pretty on
set print thread-events off
set follow-exec-mode same
handle SIGPIPE nostop noprint pass
run
thread apply all bt 40
bt full
info registers
info proc mappings
info sharedlibrary
x/24i $pc-32
generate-core-file /tmp/stellaris-repro.core
```

Run the repository launcher and let it `exec` the game:

```bash
gdb -q -x /tmp/stellaris.gdb --args \
  bash utils/automation/run-game.sh -d
```

Use `-c` when reproducing a save-load crash. If Steam launch arguments are part
of the failure, reproduce through the launcher and attach to the child process
instead:

```gdb
set follow-fork-mode child
set detach-on-fork off
```

Do not send `SIGSTOP`, `SIGINT`, or `SIGTERM` and treat the resulting stack as a
game crash.

## Interpret a Stripped Stack

The public Linux binary normally has no DWARF and no symbol table:

```bash
file /path/to/stellaris
nm -C /path/to/stellaris
readelf -n /path/to/stellaris
```

Consequences:

- `bt`, `x/i`, registers, mappings, and raw memory work.
- `info args`, `info locals`, source lines, and C++ type names generally do not.
- “Restore local state” means reconstructing registers, object fields, container
  contents, and script state, not recovering named C++ locals from DWARF.
- Use the 4.5 decompiled source and established field offsets only as structural
  references. Reconfirm offsets against the current binary version.
- For an absolute PIE address, subtract the executable mapping base from
  `info proc mappings` to obtain a stable relative offset.

When the crash report itself contains `stellaris(+0x...)` offsets, compare those
offsets across repeated runs before doing deeper memory inspection.

## Correlate the Stack with Mod Scripts

Do not stop at the first nearby `error.log` line. Follow the full caller chain.
Search both the latest crash log and the live log:

```bash
rg -n -C 4 \
  'inline_script|metascript|Compiling source|Wrong scope|deferred read|unknown|Failed|invalid' \
  "$CRASH/error.log" | tail -400
```

Prioritize failures that execute during the same phase as the crash:

- unresolved dynamic `inline_script` names;
- missing scripted effects called from generated/inlined option blocks;
- recursive or repeated scripted-effect calls;
- invalid scopes inside values evaluated during database setup;
- duplicate on_action/event execution after start or save load.

For generated inline scripts, expand every template ID and verify the referenced
file exists with exact case:

```bash
rg -n 'inline_script\s*=\s*[^\n]*\$' common events
find common/inline_scripts -type f | sort
```

An empty file is valid when intentional. A missing file is not equivalent to a
safe no-op on 4.6.

## Inspect Exception and Dump Artifacts

The `.dmp` files are Breakpad minidumps:

```bash
file "$CRASH"/*.dmp
```

Try, but do not assume support. GDB 16.3 on this host reports
`Undefined target command: "minidump"`:

```gdb
target minidump /path/to/crash.dmp
thread apply all bt 40
info registers
```

If GDB cannot consume the minidump, use `exception.txt` plus the crash-local
logs. Do not waste time treating an unsupported dump as a broken file. For a
native ELF core:

```bash
gdb /path/to/stellaris /path/to/core
```

For `SIGABRT`, inspect whether the innermost frames are:

- assertion/abort;
- uncaught exception forwarding;
- script-object constructor/destructor recursion;
- save deserialization after `--continuelastsave`.

Only call the failure caused by a specific script when a script error, the crash
phase, a stack change after the targeted fix, and a successful reproduction agree.

## Validate the Fix

Record a new timestamp baseline, relaunch, and require all of the following
matching the failing path:

1. The same playset, save, and launch phase are exercised.
2. No new crash directory is generated.
3. The previously implicated script error disappears from `error.log`.
4. The executable reaches the same or later phase than before.
5. For event-time fixes, trigger the event/effect; a main-menu load is not enough.

Use [references/case-studies.md](references/case-studies.md) for the concrete
Touhou 4.6 crash investigations, ruled-out hypotheses, and remaining limits of
the stripped-binary analysis.
