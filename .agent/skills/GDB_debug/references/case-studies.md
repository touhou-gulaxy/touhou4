# Stellaris Touhou GDB Case Studies

## 4.6 Startup SIGSEGV: Dynamic Leader Initialization

Observed on Stellaris 4.6.0, SCM commit
`7a28fe4f5c9ea23b8eb8e182a8ce4b7d0a1b5a9c`.

Repeated crash reports showed `SIGSEGV` with the same relative frames:

```text
stellaris(+0x289dd67)
stellaris(+0x1634230)
stellaris(+0x1635770)
stellaris(+0x1634230)
...
```

The repeated mid-stack frame indicated recursion or repeated dereference rather
than a random graphics failure. The crash occurred during startup/save
initialization, before gameplay.

The implicated code generated inline-script paths from the selected leader ID:

```text
inline_script = { script = effect/touhou_leader_function_respawn_$ID$ }
```

That generalized menu expanded IDs for which no corresponding effect file
existed. The older 4.5 behavior tolerated some missing inline-script expansions;
4.6 did not reliably treat them as inert.

The effective migration was:

- comment out the broad dynamic `inline_script` expansion and select the
  initialization branch with ordinary script conditions;
- change the remaining five respawn wrapper files from `$ID$` to their literal
  leader IDs;
- add explicit `spth_effect_init_<ID>_effect_non_start` no-op fallbacks for leader
  IDs without a dedicated non-starting implementation;
- keep intentional empty inline scripts only when the file exists;
- verify all generated IDs against real filenames with exact case.

After these changes, 4.6 reached the main menu and wrote complete script
documentation without generating a new crash directory. This is strong evidence
for the startup fix, but it does not prove later event-time leader behavior.

## Ruled Out: Custom `EVENT_DIPLO` Needing `ideology_window`

The 4.6 ideology UI added many windows named `ideology_window` elsewhere in the
interface, but the vanilla `EVENT_DIPLO` window itself does not contain one. The
crash-local logs contained no `EVENT_DIPLO` or `ideology_window` lookup failure.

Do not attribute a crash to the custom event window merely because both changes
occurred in 4.6. Require a missing named element error, a GUI stack, or a
reproducible UI-opening crash.

## Later `--continuelastsave` SIGABRT Is a Different Case

A later crash at `2026-11-24 07:39:57` was `SIGABRT`, not the earlier startup
`SIGSEGV`. Its metadata contained:

```text
--continuelastsave
```

and its stack passed through
`std::rethrow_exception` toward `abort()`. This identifies an uncaught C++
exception during save/campaign loading. The abort stack is not interchangeable
with the earlier leader inline-script SIGSEGV. Reproduce this case with the same
save and verify the save-load phase separately.

## Local Variables with a Stripped Binary

The current Linux executable is a stripped PIE:

```text
no symbols
no DWARF line information
```

GDB can recover:

- thread backtraces and stable image-relative offsets;
- registers, instruction context, mappings, shared libraries;
- memory bytes and inferred container/object fields;
- crash address and signal.

GDB cannot normally recover:

- named source locals such as `leader_id`, `option`, or `script_name`;
- source lines;
- C++ object types and STL contents through symbols.

Therefore questions such as “what exact local variable triggered the crash?”
must be answered by combining register/memory inspection, decompiled object
layouts, and observable script logs. Avoid claiming DWARF-level local-variable
recovery when the binary is stripped.

## 4.6 Save-Load SIGSEGV: Missing Brace Corrupted an Effect Tree

The repeated `SIGSEGV` at `stellaris+0x289dd67` was initially associated with
57 unknown dynamically generated leader trigger scripts. Removing those errors
did not change the stack or prevent the crash.

Native-core inspection showed:

```text
rdi = 0
mov 0x110(%rdi),%rdi
=> mov (%rdi),%rax
```

The original argument had been a `CScriptedEffect`. Its field at `+0x110`, which
should have referenced a `CEffect`, was zero. The caller was iterating a four
element effect vector; the broken object was the fourth element. Its embedded
source string identified:

```text
common/scripted_effects/<overwrite>_effects.txt:635
@ scripted effect finish_first_contact_effect
at events/!!!!_touhou_vanilla_overwrite_events.txt line: 86
```

Commit bisection used the same 4.6.0 binary, playset, recent save, and
`--continuelastsave` flow:

- `2766c38a fix: sync vanilla overwrite for 4.6.0` crashed at the same offset
  after about 14 seconds.
- Parent `cd2e102a` remained active for more than 50 seconds and did not create a
  crash; GDB was then interrupted intentionally while the game was in normal GLX
  buffer-swap code.
- The relevant child diff added this malformed one-line enclave switch case:

```txt
champions_forge_enclave_country = { country_event = { id = nomads.155 scopes = { from = root.owner } }
```

It has three opening braces and only two closing braces. The following
`default` case was therefore parsed under the wrong effect, corrupting the
`finish_first_contact_effect` tree. The correct line is:

```txt
champions_forge_enclave_country = { country_event = { id = nomads.155 scopes = { from = root.owner } } }
```

Lessons:

- A duplicate-definition warning is not proof of the CTD cause.
- Parse-time errors and execution-time crashes can be unrelated.
- A member-dispatch fault may have a valid wrapper object with one null linked
  object; do not report only the final `rdi = 0`.
- Commit-level parent/child testing is the fastest way to distinguish an old
  bug from a regression and identify the exact introducing diff.
- Core generation can consume all tmpfs space. Preserve the registers and
  backtrace first, keep one complete core, and remove only incomplete temporary
  cores created by the current run.

## Reusable Evidence

Crash snapshots used for these cases are under:

```text
~/.local/share/Paradox Interactive/Stellaris/crashes/
```

Use the `meta.yml`, `exception.txt`, `error.log`, `system.log`, and `.dmp` files
together. Keep unrelated optional-mod missing-definition noise separate from
errors on the crash caller chain.
