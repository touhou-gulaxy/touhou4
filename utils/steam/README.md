# Steam Workshop direct publishing

`workshop-publish.sh` updates a Stellaris Workshop item with Valve's
SteamCMD. It does not start Stellaris or the Paradox Launcher.

SteamCMD still requires interactive Steam authentication and an account with
permission to edit the Workshop item. The script never accepts, records, or
puts the password in the command line.

## Prerequisites

Install SteamCMD once:

```bash
utils/steam/workshop-publish.sh --install-steamcmd
```

Alternatively, set `STEAMCMD` to an existing `steamcmd.sh` or `steamcmd`
executable.

## Preview

Generate and inspect the upload manifest without logging in:

```bash
utils/steam/workshop-publish.sh --dry-run --print-vdf
```

The default source is the repository root. By default the script updates
Workshop item `1834433554` from `descriptor.mod`, uses
`../touhou_resources/desc_zh_latest.txt`, keeps the existing cover, applies the
tags from `descriptor.mod`, and sets public visibility.

The current module directory is about 5.4 GB. SteamPipe performs the initial
scan and only uploads changed content on later runs. Everything under the
content directory participates in the scan, including `backup`, `WIP`, and
tooling files.

## Publish

Use the Steam account that owns the Workshop item:

```bash
STEAM_USERNAME=your_steam_login \
  utils/steam/workshop-publish.sh \
  --notes "本次更新说明"
```

SteamCMD prompts for the password and Steam Guard as needed. Once it has a
valid cached session, subsequent uploads normally prompt less often.

To replace the cover and load notes from a file:

```bash
STEAM_USERNAME=your_steam_login \
  utils/steam/workshop-publish.sh \
  --preview ../touhou_resources/workshop-preview.png \
  --notes-file ../touhou_resources/update-notes.txt
```

To create a separate new item, provide both a cover and `--create`:

```bash
utils/steam/workshop-publish.sh --create \
  --preview ../touhou_resources/workshop-preview.png \
  --notes "首次发布"
```

Use `--content DIR` for a staged copy if the full development tree should not
be uploaded.
