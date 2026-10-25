#!/usr/bin/env python3
"""gen_system_warper_grid.py —— 把 move_system 的距离轴做「全路径展开」。

外层 common/inline_scripts/effect/touhou_system_warper_move_system_angle.txt 是
361 个角度分支（$value$ = 0..360），它调用的 detail 脚本由本脚本生成：
每个角度分支内部再枚举距离 0..MAXD（默认 50）——
即 361 * 51 = 18411 个 move_system，距离/角度都可精确控制。

用法：python3 utils/gen_system_warper_grid.py [MAXD]
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "common/inline_scripts/effect/touhou_system_warper_move_system_detail.txt"
MAXD = int(sys.argv[1]) if len(sys.argv) > 1 else 50

L = ["# 由 utils/gen_system_warper_grid.py 生成（勿手改）：距离 0..%d 全枚举。" % MAXD,
     "# 外层 angle 脚本传入 $value$（角度）与 $link$，两层组合 = 全路径展开。"]
for d in range(MAXD + 1):
    L += [f"{'if' if d == 0 else 'else_if'} = {{",
          f"    limit = {{ check_variable = {{ which = touhou_system_warper_distance value = {d} }} }}",
          "    root.fromfrom = {",
          "        move_system = {",
          "            target = root.from",
          f"            min_distance = {d}  max_distance = {d}",
          "            min_orientation_angle = $value$  max_orientation_angle = $value$",
          "            hyperlane = $link$",
          "        }",
          "    }",
          "}"]
OUT.write_text("\n".join(L) + "\n", encoding="utf-8")
print(f"[gen] {OUT.relative_to(ROOT)}: 距离分支 {MAXD+1} 个, {len(L)} 行, {OUT.stat().st_size} 字节")
