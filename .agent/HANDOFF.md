# 项目交接：东方繁星录

最后更新：2026-10-26

记录基线：`4.4` 分支，提交 `1635df02`

本文面向后续接手的开发者或编码代理。开始修改前仍应执行 `git status`，
不要假定本文记录时的工作区、日志或远端状态没有变化。

## 1. 项目概况

- 项目：东方繁星录 ~ Symphony of Phantasm
- 游戏：Stellaris `v4.5.*`
- MOD 版本：`2.5.1`
- Workshop ID：`1834433554`
- Git 远端：`git@github.com:touhou-gulaxy/touhou4.git`
- 当前主要开发分支：`4.4`
- MOD 源文件无需编译，游戏直接读取脚本和资源

主要玩法包括东方物种、特殊起源与领袖、幻想乡经济、灵力与弹幕科技、
特色舰船和舰船组件、巨型结构、事件链、中期与终局天灾，以及多个原版 MOD
的双向兼容内容。

## 2. 关键外部路径

| 用途 | 路径 |
|---|---|
| 游戏安装 | `/home/koishi/SteamLibrary/steamapps/common/Stellaris/` |
| 4.5 反编译源码 | `/home/koishi/ghidra_decompiled/stellaris4.5/` |
| 注入式性能报告 | `/home/koishi/ghidra_decompiled/stellaris4.5/perf/pdxperf-report.txt` |
| 游戏日志 | `~/.local/share/Paradox Interactive/Stellaris/logs/` |
| 配套资源 | `../touhou_resources/` |

游戏启动时会截断或重建 `script_profiling*.log`。需要保留的性能数据应先用
`cp` 复制到独立位置，再重新启动游戏。

## 3. 配套资源目录

`../touhou_resources/` 保存美术、音频、图标、翻译资源与发布文案。

创意工坊描述约定：

- `desc.txt`：英文创意工坊物品信息
- `desc_cn.txt`：中文创意工坊物品信息
- 第一行是物品名称
- 名称后经过空行，剩余内容为创意工坊物品描述

注意：记录时实际存在的是 `desc_zh.txt`，尚未统一为本文约定的
`desc_cn.txt`。引用或生成发布文案时先检查实际文件名。

`../touhou_resources/utf8/` 包含部分翻译的英文本地化。此前统计约：

- 34 个英文 YAML 文件
- 约 29,897 个本地化条目
- 5,274 个值仍含汉字
- 约 167,453 个汉字
- 主要缺口在 `th_l_english.yml` 和 `new_spth_l_english.yml`

翻译时保留 `$...$`、`§X...§!`、`[...]`、`£...£`、转义和换行。

## 4. 常用代码入口

- `events/spth_fleet_events.txt`
  - 舰船动态修正、额外生命、战斗事件与危机舰队机制的核心文件。
  - `spth_fleet.2042`：约每两日刷新动态再生、Arua 保护和幽幽子舰队覆盖率。
  - `spth_fleet.2045`：约每两日刷新战力差距射速系统。
  - `spth_fleet.1002` / `.1104`：舰船被禁用及额外生命处理，大规模战斗时高频。
  - `spth_fleet.1202`：击杀后更新返魂蝶叠层。
- `common/scripted_effects/th_dynamic_batte_effect.txt`
  - 动态条件修正和百分比修正工具。
- `common/scripted_effects/spth_crisis_intelligent_actions.txt`
  - 天灾舰队自动移动和交战寻路。
- `common/component_templates/`
  - 武器、组件、`on_hit`、`triggered_ship_modifier`。
- `common/on_actions/`
  - 战斗、舰船禁用、击杀和其他事件入口。
- `common/script_values/`
  - 动态修正数值和舰船覆盖率计算。
- `common/scripted_variables/`
  - 舰船、组件和武器生成值。

## 5. 性能相关约定

### 动态修正批处理

脚本效果在 `common/scripted_effects/spth_misc_effects.txt`：

```text
touhou_stop_global_modifier_system
touhou_resume_global_modifier_system
```

它们分别封装 `set_update_modifiers_batch = begin/end`。批量处理大量舰船时，
应包住整个修改过程，而不是每艘舰船分别结束一次。

不要轻率嵌套 batch。该状态是引擎全局状态，嵌套或异常退出可能造成修正更新
不完整。

### 已知性能特征

- `VOP_evt_ModifierUpdated.1` 的旧两日循环触发已从 on_action 中禁用。
- `spth_fleet.2042/2045` 的批量修正更新正常。
- `spth_fleet.1002 -> 1104` 是大规模舰船同时被禁用时的重要突发热点。
- `spth_fleet.2045` 的百分比射速工具是常规热点；零 offset 时可考虑快路径。
- 天灾寻路应使用 `any_fleet_in_system` 作为外层过滤，再用
  `any_owned_ship` 保留逐舰的停用/舰种语义。
- `triggered_ship_modifier` 在修正重算时求值；大量动态修正会放大其成本。
- `on_hit` 每命中触发，不能用普通两日事件的成本模型评估。

### 性能工具盲区

`script_profiler` 只记录显式 `CScopedStartProfile` 节点：

- 不能单独统计通用内存分配、`CEventScope` 构造和深拷贝。
- 不会建立独立的武器 `on_hit` root，因此本次日志看不到完整 `on_hit` 成本。
- 部分 `CGameRules::*` 会记录；例如 `CGameRules::IsCountryPsionic` 在高频
  武器射击路径中可能非常频繁。
- 开启 script profiler 本身会增加堆分配和计时开销，绝对耗时不可直接当作
  未开启 profiler 的帧时间。

需要完整函数/分配归因时使用：

```text
/home/koishi/ghidra_decompiled/stellaris4.5/perf/pdxperf-report.txt
```

或对带符号的 `stellaris` 进程使用 Linux `perf`。

## 6. 固定检查与测试

最低限度：

```bash
git diff --check
```

建议的完整验证流程：

1. 运行 `.vscode/` 配置的 CWTools 诊断。
2. 使用接近实际玩法的存档启动游戏。
3. 检查 `error.log` 中的脚本错误、无效 scope 和除零。
4. 触发至少一场包含目标舰船/武器/天灾的大型战斗。
5. 检查 `script_profiling_summary.log`。
6. 对硬编码战斗热点使用 `pdxperf` 或 `perf`。

当前没有统一自动测试套件。

## 7. 工具与生成器

- `utils/gen_component_vars.sh`
  - 编译 `utils/comp/component_vars.cpp` 和 `utils/component_vars_sync.cpp`
  - 支持 `--dry-run`、`--list`、`--no-backup`
- `utils/ships/gen_ags_psionic_variants.sh`
  - 生成 AGS 灵能舰船颜色差分资源
- `utils/touhou_gen_ship_modifier.sh`
- `utils/touhou_gen_ship_modifier_loc.sh`
- `utils/izakaya/process_recipe.sh`
- `utils/tech/tech_dependency_analyzer.cpp`

生成器输出在提交前必须检查，避免覆盖手工修正。

## 8. 编码与格式注意事项

- 游戏脚本通常允许 LF；部分原版覆盖文件使用 CRLF，修改时避免无意整文件
  换行转换。
- YAML 本地化文件通常为 UTF-8 with BOM。
- 不要删除 BOM、语言头 `l_english:` 或 `l_simp_chinese:`。
- `random_list` 权重修正使用 `factor`、`add`、`subtract` 等受支持字段。
  不要把随机权重乘算写成 `mult`。
- 复杂变量运算优先使用 `check_variable_arithmetic`，避免多层临时变量。
- 事件 scope 出错通常不会中止整个游戏，因此必须检查 `error.log`。

## 9. 当前建议后续工作

1. 清理 `th_l_english.yml`、`new_spth_l_english.yml` 等英文本地化缺口。
2. 使用 pdxperf A/B 验证高频 `on_hit` 和 `triggered_ship_modifier`。
3. 优化高射速 `on_hit` 中无条件的修正移除/添加和随机分支。
4. 评估把 `potential = { always = yes }` 的简单
   `triggered_ship_modifier` 迁移为静态 `ship_modifier`。
5. 继续检查大型战斗中的 `spth_fleet.1002`、`1104` 和额外生命路径。
6. 清理战斗期重复出现的无效事件目标、除零和本地化格式错误。

## 10. 交接原则

- 修改前先阅读目标文件附近的注释和命名约定。
- 优先做最小、可解释的增量修改。
- 不覆盖无法确认来源的未提交更改。
- 不因性能优化改变事件 scope、触发时序或玩家可见概率，除非明确说明。
- 每次调整后记录性能收益和正确性验证条件。
