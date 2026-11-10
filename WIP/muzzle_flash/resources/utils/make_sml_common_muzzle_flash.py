#!/usr/bin/env python3
"""Split S/M/L projectile muzzle flashes from shared capital-weapon effects."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMPONENT_DIR = ROOT / "common/component_templates"
PROJECTILE_FILE = ROOT / "gfx/projectiles/thxl_weapons.txt"
ENTITY_FILE = ROOT / "gfx/models/combat_items/spth_danmuku.asset"

COMMON_ENTITIES = {
    "spth_gungnir_muzzle_flash_entity": "spth_gungnir_common_muzzle_flash_entity",
    "spth_spark_muzzle_flash_entity": "spth_spark_common_muzzle_flash_entity",
    "spth_yuzhu_muzzle_flash_entity": "spth_yuzhu_common_muzzle_flash_entity",
    "spth_reimu_fantasia_muzzle_flash_entity": "spth_reimu_fantasia_common_muzzle_flash_entity",
    "spth_sakura_muzzle_flash_entity": "spth_sakura_common_muzzle_flash_entity",
    "spth_sakura_alt_muzzle_flash_entity": "spth_sakura_alt_common_muzzle_flash_entity",
}


def strip_comments(text: str) -> str:
    text = re.sub(r"#[^\n]*", "", text)
    return re.sub(r"//[^\n]*", "", text)


def blocks(text: str, pattern: str) -> list[tuple[int, int, str]]:
    result: list[tuple[int, int, str]] = []
    for match in re.finditer(pattern, text):
        index = match.end()
        depth = 1
        quoted = False
        escaped = False
        while index < len(text) and depth:
            char = text[index]
            if quoted:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    quoted = False
            else:
                if char == '"':
                    quoted = True
                elif char == "{":
                    depth += 1
                elif char == "}":
                    depth -= 1
            index += 1
        if depth == 0:
            result.append((match.start(), index, text[match.start():index]))
    return result


def replace_once(text: str, pattern: str, replacement: str, within: tuple[int, int, str]) -> str:
    start, end, block = within
    new_block, count = re.subn(pattern, replacement, block, count=1)
    if count != 1:
        raise RuntimeError(f"Expected one match for {pattern!r}")
    return text[:start] + new_block + text[end:]


def component_slot_data() -> tuple[set[str], dict[str, set[str]]]:
    sml: set[str] = set()
    slots_by_projectile: dict[str, set[str]] = {}
    for path in COMPONENT_DIR.rglob("*.txt"):
        text = path.read_text(encoding="utf-8-sig")
        for _, _, block in blocks(text, r"weapon_component_template\s*=\s*\{"):
            projectile = re.search(r'\bprojectile_gfx\s*=\s*"?([^"\s]+)"?', block)
            tags = re.search(r"\btags\s*=\s*\{([^}]*)\}", block)
            if not projectile or not tags:
                continue
            slots = set(re.findall(r"\b([smlxt])_slot\b", tags.group(1)))
            if not slots:
                if "point_defence" in tags.group(1):
                    slots.add("pd")
                elif "torpedo" in tags.group(1):
                    slots.add("torpedo")
                else:
                    slots.add("other")
            name = projectile.group(1)
            slots_by_projectile.setdefault(name, set()).update(slots)
            if slots & {"s", "m", "l"}:
                sml.add(name)
    return sml, slots_by_projectile


def ensure_common_entities(write: bool) -> None:
    text = ENTITY_FILE.read_bytes().decode("utf-8")
    entity_blocks = blocks(text, r"entity\s*=\s*\{")
    by_name = {
        match.group(1): item
        for item in entity_blocks
        if (match := re.search(r'\bname\s*=\s*"?([^"\s]+)"?', item[2]))
    }
    additions: list[str] = []
    for source_name, common_name in COMMON_ENTITIES.items():
        if common_name in by_name:
            continue
        if source_name not in by_name:
            raise RuntimeError(f"Missing source entity {source_name}")
        _, _, source_block = by_name[source_name]
        cleaned_lines = []
        for line in source_block.splitlines(keepends=True):
            active = strip_comments(line)
            if "spth_magic_field_" in active or "spth_sakura_stage6_muzzle_flash_" in active:
                continue
            cleaned_lines.append(line)
        common_block = "".join(cleaned_lines)
        common_block = re.sub(
            r'(\bname\s*=\s*)"?' + re.escape(source_name) + r'"?',
            rf'\1"{common_name}"',
            common_block,
            count=1,
        )
        additions.append(
            f"\n# S/M/L no-magic muzzle variant derived from {source_name}\n"
            + common_block.rstrip()
            + "\n"
        )
    if additions and write:
        ENTITY_FILE.write_bytes((text.rstrip() + "\n" + "".join(additions)).encode("utf-8"))
    for name in COMMON_ENTITIES.values():
        print(("present" if name in by_name else "create"), name)


def update_projectiles(sml: set[str], slots_by_projectile: dict[str, set[str]], write: bool) -> dict[str, str]:
    text = PROJECTILE_FILE.read_bytes().decode("utf-8")
    projectile_blocks = blocks(text, r"projectile_gfx_(?:beam|ballistic)\s*=\s*\{")
    existing_names = {
        match.group(1)
        for item in projectile_blocks
        if (match := re.search(r'\bname\s*=\s*"([^"]+)"', item[2]))
    }
    renamed: dict[str, str] = {}
    replacements: list[tuple[int, int, str]] = []
    appended: list[str] = []
    referenced_projectiles: set[str] = set()
    for path in COMPONENT_DIR.rglob("*.txt"):
        component_text = path.read_text(encoding="utf-8-sig")
        referenced_projectiles.update(
            match.group(1)
            for match in re.finditer(
                r'\bprojectile_gfx\s*=\s*"?([^"\s]+)"?', component_text
            )
        )

    for item in projectile_blocks:
        name_match = re.search(r'\bname\s*=\s*"([^"]+)"', item[2])
        muzzle_match = re.search(r'\bmuzzle_flash_entity\s*=\s*"([^"]+)"', item[2])
        if not name_match or not muzzle_match:
            continue
        name = name_match.group(1)
        muzzle = muzzle_match.group(1)
        if name not in sml or muzzle not in COMMON_ENTITIES:
            continue
        common = COMMON_ENTITIES[muzzle]
        shared = bool(slots_by_projectile.get(name, set()) - {"s", "m", "l"})
        if not shared:
            new_block = re.sub(
                r'(\bmuzzle_flash_entity\s*=\s*)"' + re.escape(muzzle) + r'"',
                rf'\1"{common}"',
                item[2],
                count=1,
            )
            replacements.append((item[0], item[1], new_block))
            print("replace", name, muzzle, "->", common)
            continue

        sml_name = f"{name}_sml"
        renamed[name] = sml_name
        if sml_name in existing_names:
            continue
        clone = item[2]
        clone = re.sub(
            r'(\bname\s*=\s*)"' + re.escape(name) + r'"',
            rf'\1"{sml_name}"',
            clone,
            count=1,
        )
        clone = re.sub(
            r'(\bmuzzle_flash_entity\s*=\s*)"' + re.escape(muzzle) + r'"',
            rf'\1"{common}"',
            clone,
            count=1,
        )
        appended.append(f"\n# S/M/L-only variant of {name}\n" + clone.rstrip() + "\n")
        print("clone", name, "->", sml_name, "muzzle", common)

    # If component repointing already happened in an interrupted run, create
    # the corresponding clone from the original block.
    for item in projectile_blocks:
        name_match = re.search(r'\bname\s*=\s*"([^"]+)"', item[2])
        muzzle_match = re.search(r'\bmuzzle_flash_entity\s*=\s*"([^"]+)"', item[2])
        if not name_match or not muzzle_match:
            continue
        name = name_match.group(1)
        muzzle = muzzle_match.group(1)
        sml_name = f"{name}_sml"
        if sml_name not in referenced_projectiles or sml_name in existing_names or muzzle not in COMMON_ENTITIES:
            continue
        clone = item[2]
        clone = re.sub(
            r'(\bname\s*=\s*)"' + re.escape(name) + r'"',
            rf'\1"{sml_name}"',
            clone,
            count=1,
        )
        clone = re.sub(
            r'(\bmuzzle_flash_entity\s*=\s*)"' + re.escape(muzzle) + r'"',
            rf'\1"{COMMON_ENTITIES[muzzle]}"',
            clone,
            count=1,
        )
        appended.append(f"\n# S/M/L-only variant of {name}\n" + clone.rstrip() + "\n")
        print("recover clone", name, "->", sml_name)

    if replacements:
        pieces = []
        cursor = 0
        for start, end, replacement in sorted(replacements):
            pieces.append(text[cursor:start])
            pieces.append(replacement)
            cursor = end
        pieces.append(text[cursor:])
        text = "".join(pieces)
    if write and appended:
        text = text.rstrip() + "\n" + "".join(appended)
    if write:
        PROJECTILE_FILE.write_bytes(text.encode("utf-8"))
    return renamed


def update_component_references(renamed: dict[str, str], write: bool) -> None:
    for path in COMPONENT_DIR.rglob("*.txt"):
        text = path.read_bytes().decode("utf-8")
        changed = False
        component_blocks = blocks(text, r"weapon_component_template\s*=\s*\{")
        for start, end, block in reversed(component_blocks):
            projectile = re.search(r'\bprojectile_gfx\s*=\s*"?([^"\s]+)"?', block)
            tags = re.search(r"\btags\s*=\s*\{([^}]*)\}", block)
            if not projectile or not tags:
                continue
            if not re.search(r"\b[sml]_slot\b", tags.group(1)):
                continue
            old_name = projectile.group(1)
            new_name = renamed.get(old_name)
            if not new_name:
                continue
            new_block = re.sub(
                r'(\bprojectile_gfx\s*=\s*)"?' + re.escape(old_name) + r'"?',
                rf'\1"{new_name}"',
                block,
                count=1,
            )
            text = text[:start] + new_block + text[end:]
            changed = True
            print("repoint", path.relative_to(ROOT), old_name, "->", new_name)
        if changed and write:
            path.write_bytes(text.encode("utf-8"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    sml, slots_by_projectile = component_slot_data()
    ensure_common_entities(args.write)
    renamed = update_projectiles(sml, slots_by_projectile, args.write)
    update_component_references(renamed, args.write)


if __name__ == "__main__":
    main()
