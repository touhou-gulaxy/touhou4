#!/usr/bin/env python3
"""Generate Stellaris assets for the Part III EnemyBarrages sprite set.

The source frames are padded to square canvases before being assembled into
vertical sprite strips. This keeps non-square source sprites proportional when
the same rectangular quad or billboard is used for every bullet.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path


DEFAULT_SOURCE = Path(
    "/home/koishi/SteamLibrary/steamapps/common/Touhou FDF Part II/"
    "ExtractedAssets-PartIII/BulletTextures/EnemyBarrages"
)

# These are single-frame directional sprites. Animated directional sprites stay
# particles so their source frames remain discrete instead of visibly blurring
# across an animated UV strip.
MODEL_SOURCE_IDS = set(range(25, 49))

DIRECTIONAL_TERMS = (
    "苦无",
    "米弹",
    "针弹",
    "椭圆",
    "炎弹",
    "尖波",
    "圆波",
    "锥弹",
    "小波",
    "乌鸦弹",
    "大妖光波",
    "大尖光波",
    "血妖小波",
    "血锥波",
)


def run(command: list[str]) -> None:
    subprocess.run(command, check=True)


def frame_canvas_size(width: int, height: int) -> int:
    longest = max(width, height)
    if longest <= 64:
        return 64
    if longest <= 128:
        return 128
    return 256


def make_texture(source_dir: Path, item: dict, output: Path) -> None:
    frames = item["frames_detail"]
    size = frame_canvas_size(item["width"], item["height"])
    output.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="spth_barrage_") as tmp:
        padded: list[str] = []
        for frame in frames:
            frame_path = source_dir / frame["path"]
            frame_output = Path(tmp) / f"frame_{frame['index']:03d}.png"
            run(
                [
                    "magick",
                    str(frame_path),
                    "-background",
                    "none",
                    "-gravity",
                    "center",
                    "-extent",
                    f"{size}x{size}",
                    str(frame_output),
                ]
            )
            padded.append(str(frame_output))

        command = ["magick"]
        if len(padded) > 1:
            command.extend(padded)
            command.append("-append")
        else:
            command.append(padded[0])
        command.extend(
            [
                "-strip",
                "-define",
                "dds:compression=dxt5",
                "-define",
                "dds:mipmaps=1",
                str(output),
            ]
        )
        run(command)


def model_entry(item: dict, slug: str) -> str:
    scale = frame_canvas_size(item["width"], item["height"]) / 10.0
    return f"""
entity = {{
    name = "{slug}_entity"
    pdxmesh = "{slug}_mesh"
    cull_radius = {scale * 2.0:.2f}
    scale = {scale:.2f}
    default_state = "idle"
    state = {{
        name = "idle"
        looping = yes
    }}
}}
"""


def particle_entry(item: dict, slug: str) -> str:
    canvas = frame_canvas_size(item["width"], item["height"])
    return f"""
entity = {{
    name = "{slug}_entity"
    pdxmesh = "locator_mesh"
    cull_radius = {canvas / 5.0:.2f}
    scale = 1.0
    default_state = "idle"
    state = {{
        name = "idle"
        looping = yes
        start_event = {{
            particle = "{slug}_particle"
            keep_particle = yes
            trigger_once = yes
        }}
    }}
}}
"""


def particle_asset(item: dict, slug: str, texture_path: str) -> str:
    animated = len(item["frames_detail"]) > 1
    directional = any(term in item["name"] for term in DIRECTIONAL_TERMS)
    rotation_speed = 0
    if animated and not directional and any(
        term in item["name"] for term in ("小星", "闪玉", "闪中", "闪鳞")
    ):
        rotation_speed = 60

    return f"""particle = {{
    name = "{slug}_file"
    subsystem = {{
        name = "sprite"
        max_amount = 1
        slave_particles = 0
        sort = "depth"
        emitter_type = "point"
        invert = no
        trail = no
        spritesheet_animation = {"yes" if animated else "no"}
        spritesheet_animation_loop = 1.000000
        local_space = yes
        billboard = {"no" if directional else "yes"}
        hide = no
        texture = {{
            file = "{texture_path}"
            x = 1
            y = {len(item["frames_detail"])}
            shader = "ParticleAdditive"
        }}
        color = {{
            x = 255
            y = 255
            z = 255
            alpha = 255
        }}
        position = {{
        }}
        duration = -1
        emission_pulse_duration = -1
        life = 10
        emission = 1
        size = 1
        particle_pitch = 0
        rotation_speed = {rotation_speed}
    }}
    force = {{
        type = "planar"
        name = "force0"
        position = {{
            0 0 0
        }}
        direction = {{
            0 1 0
        }}
        local_force = yes
        yaw = 0
        division = 16
        amount = 5
    }}
}}
"""


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    manifest = json.loads((args.source / "manifest.json").read_text(encoding="utf-8"))
    model_dir = repo / "gfx/models/combat_items/enemy_barrages"
    texture_dir = model_dir
    particle_dir = repo / "gfx/particles/combat/enemy_barrages"
    model_dir.mkdir(parents=True, exist_ok=True)
    texture_dir.mkdir(parents=True, exist_ok=True)
    particle_dir.mkdir(parents=True, exist_ok=True)

    shutil.copy2(
        repo / "gfx/models/combat_items/th_barrage_star.mesh",
        model_dir / "enemy_barrage_plane.mesh",
    )

    mesh_defs: list[str] = []
    particle_defs: list[str] = []
    entities: list[str] = []
    readme_rows: list[str] = []
    index: list[dict] = []

    for item in manifest["types"]:
        type_id = int(item["type_id"])
        slug = f"spth_enemy_barrage_{type_id:03d}"
        texture_name = f"{slug}.dds"
        texture_path = texture_dir / texture_name
        make_texture(args.source, item, texture_path)
        model_texture_ref = texture_name
        particle_texture_ref = (
            f"gfx/models/combat_items/enemy_barrages/{texture_name}"
        )
        is_model = type_id in MODEL_SOURCE_IDS
        animated = len(item["frames_detail"]) > 1

        if is_model:
            mesh_defs.append(
                f"""    pdxmesh = {{
        name = "{slug}_mesh"
        file = "gfx/models/combat_items/enemy_barrages/enemy_barrage_plane.mesh"
        meshsettings = {{
            index = 0
            texture_diffuse = "{model_texture_ref}"
            shader = "PdxMeshAlphaAdditive"
        }}
    }}
"""
            )
            entities.append(model_entry(item, slug))
            kind = "model"
        else:
            particle_defs.append(
                f"""    pdxparticle = {{
        name = "{slug}_particle"
        type = "{slug}_file"
        scale = {frame_canvas_size(item["width"], item["height"]) / 10.0:.2f}
    }}
"""
            )
            entities.append(particle_entry(item, slug))
            (particle_dir / f"{slug}.asset").write_text(
                particle_asset(item, slug, particle_texture_ref), encoding="utf-8"
            )
            kind = "particle"

        readme_rows.append(
            f"| {type_id:03d} | {item['name']} | {item['width']}x{item['height']} | "
            f"{len(item['frames_detail'])} | {kind} | "
            f"{'effect' if type_id >= 143 else 'projectile'} | `{slug}_entity` |"
        )
        index.append(
            {
                "type_id": type_id,
                "name": item["name"],
                "entity": f"{slug}_entity",
                "kind": kind,
                "role": "effect" if type_id >= 143 else "projectile",
                "animated": animated,
                "frames": len(item["frames_detail"]),
                "texture": f"gfx/models/combat_items/enemy_barrages/{texture_name}",
            }
        )

    (model_dir / "spth_enemy_barrages.gfx").write_text(
        "objectTypes = {\n" + "".join(mesh_defs) + "}\n", encoding="utf-8"
    )
    (repo / "gfx/particles/spth_enemy_barrages.gfx").write_text(
        "objectTypes = {\n" + "".join(particle_defs) + "}\n", encoding="utf-8"
    )
    (model_dir / "spth_enemy_barrage_entities.asset").write_text(
        "".join(entities), encoding="utf-8"
    )
    (model_dir / "index.json").write_text(
        json.dumps(index, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (model_dir / "README.md").write_text(
        "# Enemy barrage entities\n\n"
        "Generated from Part III EnemyBarrages. IDs 025-048 are static, "
        "directional sprites rendered with a shared square plane mesh. "
        "Other types are locator entities with independently defined particle "
        "effects; animated types use vertical sprite sheets.\n\n"
        "| ID | Source | Pixels | Frames | Kind | Role | Entity |\n"
        "|---:|---|---:|---:|---|---|---|\n"
        + "\n".join(readme_rows)
        + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
