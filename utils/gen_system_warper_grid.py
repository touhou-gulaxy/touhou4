#!/usr/bin/env python3
"""生成使用变量参数的系统跃迁 move_system 脚本。

Stellaris 4.6 的 move_system 已支持变量和脚本值，不再需要展开
角度 0..360 与距离 0..MAXD 的全部组合。
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "common/inline_scripts/effect/touhou_system_warper_move_system_detail.txt"

L = [
    "# 由 utils/gen_system_warper_grid.py 生成（勿手改）。",
    "# move_system 直接读取距离变量和角度参数。",
    "root.fromfrom = {",
    "    move_system = {",
    "        target = root.from",
    "        min_distance = root.touhou_system_warper_distance",
    "        max_distance = root.touhou_system_warper_distance",
    "        min_orientation_angle = $value$",
    "        max_orientation_angle = $value$",
    "        hyperlane = $link$",
    "    }",
    "}",
]
OUT.write_text("\n".join(L) + "\n", encoding="utf-8")
print(f"[gen] {OUT.relative_to(ROOT)}: {len(L)} 行, {OUT.stat().st_size} 字节")
