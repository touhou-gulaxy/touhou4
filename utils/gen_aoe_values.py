#!/usr/bin/env python3
"""gen_aoe_values.py

------------------------------
`common/inline_scripts/ship_components/touhou_aoe_attribute.txt` 与
  `touhou_chain_damage_attribute.txt`
      min = @[ touhou_aoe_<slot>_<faction>_<type>_t<tier>_e.._s.._re.._rs.._min ]
      min = @[ touhou_chain_<name>_<tag>_<type>_min ]

--------------------
    python3 utils/gen_aoe_values.py            # 重新生成 就地写回 config
    python3 utils/gen_aoe_values.py --check    # 只比对，不一致 exit 1
    python3 utils/gen_aoe_values.py --dry-run  # 打印将写入的内容
"""
from __future__ import annotations

import os
import re
import sys
from decimal import Decimal, getcontext
from pathlib import Path

getcontext().prec = 28

_env_root = os.environ.get("TOUHOU_MOD_ROOT")
ROOT = Path(_env_root).resolve() if _env_root else Path(__file__).resolve().parents[1]
SV_DIR = ROOT / "common/scripted_variables"
CFG = SV_DIR / "touhou_aoe_config.txt"
AOE_SCRIPT = "touhou_aoe_attribute"
CHAIN_SCRIPT = "touhou_chain_damage_attribute"
GEN_HEAD = "# ===== BEGIN GENERATED"

# ---------------------------------------------------------------- 读取字面量变量
SCALARS: dict[str, Decimal] = {}
_NUM = re.compile(r"^@([A-Za-z_]\w*)\s*=\s*(-?\d+(?:\.\d+)?)\s*$")


def load_scalars() -> None:
    for p in sorted(SV_DIR.glob("*.txt")):
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for line in text.splitlines():
            m = _NUM.match(line.strip())
            if m:
                SCALARS.setdefault(m.group(1), Decimal(m.group(2)))


def dec(name: str) -> Decimal:
    """取 scripted_variables 里的字面数字，取不到就报错（避免算出静默的 0）。"""
    if name not in SCALARS:
        raise KeyError(f"scripted_variables 里找不到字面量 @{name}")
    return SCALARS[name]


def scale_value(param: str) -> Decimal:
    """`dmg_scale = 1_25` -> @touhou_aoe_value_1_25（没有就直接按 1.25 解释）。"""
    key = f"touhou_aoe_value_{param}"
    if key in SCALARS:
        return SCALARS[key]
    return Decimal(param.replace("_", "."))


# ------------------------------------------------------- 扫描 inline_script 调用点
def all_texts() -> dict[Path, str]:
    out: dict[Path, str] = {}
    for p in sorted(ROOT.glob("common/**/*.txt")):
        try:
            out[p] = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            pass
    return out


def parse_params(body: str) -> dict[str, str]:
    return dict(re.findall(r"([A-Za-z_]\w*)\s*=\s*(\"[^\"]*\"|[^\s{}]+)", body))


def script_stem(body: str) -> str | None:
    m = re.search(r"script\s*=\s*([^\s{}]+)", body)
    return Path(m.group(1)).stem if m else None


def collect(texts: dict[Path, str]):
    sites: dict[str, list[tuple[Path, dict[str, str]]]] = {}
    callers: dict[str, list[dict[str, str]]] = {}
    for p, t in texts.items():
        for m in re.finditer(r"inline_script\s*=\s*\{([^{}]*)\}", t, re.S):
            body = m.group(1)
            stem = script_stem(body)
            if not stem:
                continue
            params = parse_params(body)
            sites.setdefault(stem, []).append((p, params))
            callers.setdefault(stem, []).append(params)
    return sites, callers


def resolve(sites, callers, key: str):
    """把某个 inline_script 的调用点解析成具体实参（$param$ 转发展开一层）。"""
    out = []
    for p, params in sites.get(key, []):
        if not any("$" in v for v in params.values()):
            out.append((p, params))
            continue
        name = p.stem
        for outer in callers.get(name) or [{}]:
            merged = {}
            for k, v in params.items():
                m = re.fullmatch(r"\$(\w+)\$", v.strip())
                merged[k] = outer.get(m.group(1), v.strip()) if m else v.strip()
            out.append((p, merged))
    return out


# --------------------------------------------------------------------- 计算与生成
AOE_KEYS = ("slot", "faction", "type", "tier", "dmg_extra", "dmg_scale", "rng_extra", "rng_scale")


def fmt(d: Decimal) -> str:
    """引擎的 CFixedPoint 是 1/1000，这里同样按 3 位小数给值。"""
    return str(d.quantize(Decimal("0.001")))


def aoe_block():
    errs, combos = [], set()
    sites, callers = _CACHE
    for _, p in resolve(sites, callers, AOE_SCRIPT):
        if any(k not in p for k in AOE_KEYS):
            errs.append(f"{AOE_SCRIPT}: 参数不全 {sorted(p.items())}")
            continue
        bad = [k for k in AOE_KEYS if "." in p[k]]
        if bad:
            errs.append(f"{AOE_SCRIPT}: 参数 {bad} 含 '.'，会拼出非法变量名 -> {sorted(p.items())}")
            continue
        combos.add(tuple(p[k] for k in AOE_KEYS))

    lines = ["", GEN_HEAD + ": per-combination AOE values (do not hand-edit) =====",
             "# 由 utils/gen_aoe_values.py 生成，纯字面数字；改因子后重跑本脚本。",
             "# 供 inline_script ship_components/touhou_aoe_attribute 用 @[ 名字 ] 取值。",
             "@touhou_aoe_value_1_25 = 1.25"]
    for slot, fac, typ, tier, de, ds, rg_e, rg_s in sorted(combos):
        key = (f"{slot}_{fac}_{typ}_t{tier}_e{de}"
               f"_s{ds.replace('.', '_')}_re{rg_e}_rs{rg_s}")
        dmg = (Decimal(de)
               + dec("touhou_aoe_dmg_base")
               * dec(f"touhou_aoe_dmg_{slot}_slot_factor")
               * scale_value(ds)
               * dec(f"touhou_aoe_dmg_{typ}_type_factor")
               * dec(f"touhou_aoe_dmg_{fac}_factor")
               * dec(f"touhou_aoe_dmg_t{tier}_factor")
               * dec("touhou_aoe_dmg_factor"))
        rng = (Decimal(rg_e)
               + dec("touhou_aoe_rng_base")
               * dec(f"touhou_aoe_rng_{slot}_slot_factor")
               * Decimal(rg_s)
               * dec(f"touhou_aoe_rng_{typ}_type_factor")
               * dec(f"touhou_aoe_rng_{fac}_factor")
               * dec(f"touhou_aoe_rng_t{tier}_factor")
               * dec("touhou_aoe_rng_factor"))
        lines += [f"@touhou_aoe_{key}_min = {fmt(dec('touhou_aoe_dmg_min_factor') * dmg)}",
                  f"@touhou_aoe_{key}_max = {fmt(dec('touhou_aoe_dmg_max_factor') * dmg)}",
                  f"@touhou_aoe_{key}_rng = {fmt(rng)}"]
    lines.append("# ===== END GENERATED: per-combination AOE values =====")
    return "\n".join(lines) + "\n", errs, len(combos)


def chain_block():
    errs, combos = [], set()
    sites, callers = _CACHE
    for _, p in resolve(sites, callers, CHAIN_SCRIPT):
        if any(k not in p or "$" in p[k] for k in ("name", "tag", "type")):
            errs.append(f"{CHAIN_SCRIPT}: name/tag/type 未解析 {sorted(p.items())}")
            continue
        combos.add((p["name"], p["tag"], p["type"]))

    lines = ["", GEN_HEAD + ": per-combination chain values (do not hand-edit) =====",
             "# 取 spth_<name>_<tag>_<type>_damage_min/_max（spth_component_variables.txt）"]
    for name, tag, typ in sorted(combos):
        base = f"spth_{name}_{tag}_{typ}"
        expr = ((dec(f"{base}_damage_min") + dec(f"{base}_damage_max")) / 4
                * dec(f"touhou_generic_scale_damage_{typ}_factor"))
        lines += [f"@touhou_chain_{name}_{tag}_{typ}_min = "
                  f"{fmt(dec('touhou_chain_dmg_min_factor') * expr)}",
                  f"@touhou_chain_{name}_{tag}_{typ}_max = "
                  f"{fmt(dec('touhou_chain_dmg_max_factor') * expr)}"]
    lines.append("# ===== END GENERATED: per-combination chain values =====")
    return "\n".join(lines) + "\n", errs, len(combos)


_CACHE = (None, None)


def main() -> int:
    global _CACHE
    load_scalars()
    _CACHE = collect(all_texts())
    try:
        aoe, err_a, n_a = aoe_block()
        chain, err_c, n_c = chain_block()
    except KeyError as e:
        print(f"[!] {e}", file=sys.stderr)
        print("[!] 缺字面量多为因子名写错，或误删了 touhou_aoe_config.txt 里的基础变量", file=sys.stderr)
        return 2
    for e in err_a + err_c:
        print("[!]", e, file=sys.stderr)

    old = CFG.read_text(encoding="utf-8")
    i = old.find(GEN_HEAD)
    head = (old[:i].rstrip("\n") + "\n") if i >= 0 else (old.rstrip("\n") + "\n")
    new = head + aoe + chain

    if "--dry-run" in sys.argv:
        print(new)
        return 0
    if "--check" in sys.argv:
        if new == old:
            print(f"OK: config 与生成结果一致（AOE {n_a} 组合 + chain {n_c} 组合）")
            return 0
        print("config 与生成结果不一致：跑 python3 utils/gen_aoe_values.py", file=sys.stderr)
        return 1
    CFG.write_text(new, encoding="utf-8")
    print(f"已写入 {CFG.relative_to(ROOT)}：AOE {n_a} 组合 / chain {n_c} 组合")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
