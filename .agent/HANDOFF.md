# 项目交接：东方繁星录

最后更新：2026-11-15

记录基线：`dev/4.6`，提交 `f5e08448`，MOD 版本 `2.5.3`。
当前工作树已有大批未提交功能修改，状态必须每次以 Git 为准。

开始修改前必须执行：

```bash
git status --short --branch
git log -5 --oneline --decorate
```

当前工作树和历史状态以 Git 输出为准，本文只记录需要避免重复发现的稳定知识和
交接时状态。

## 1. 项目概况

- 项目：东方繁星录 ~ Symphony of Phantasm
- 游戏：Stellaris `v4.5.*`
- 当前验证客户端：Cygnus `v4.5.4`
- MOD 版本：`2.5.3`
- Workshop ID：`1834433554`
- 主分支：`dev/4.6`
- MOD 是脚本与资源项目，不需要编译；游戏直接读取 `common/`、`events/`、
  `interface/`、`gfx/`、`localisation/` 等目录

设备相关的外部路径、日志位置和授权外部资源记录在
[local_config.json](local_config.json)。当前设备的详细日志和未验证项见
[local_handoff.md](local_handoff.md)。

## 2. 当前版本状态

`f5e08448` 已将：

- `descriptor.mod` 更新为 `2.5.3`
- `spth_version` 更新为 `2.5.3 §E梦幻净土§!版本`
- `spth_version_date` 更新为 `2026.11.14`
- `spth_latest_update_info` 更新为 2.5.3 内容
- 历史日志加入 `spth_history_update_info_2_5_1` 和
  `spth_history_update_info_2_5_2`

版本号、日期、最新日志、历史日志和
`common/game_concepts/spth_word_concepts.txt` 中的历史概念条目必须同步修改。

本次版本范围：

- 2.5.2：`a2360778..6dd75387`
- 2.5.3：`9526e0bd..f5e08448`

`a2360778` 是最后一次把 `spth_version` 从 2.5.0 改为 2.5.1 的提交。
计算后续发布范围时不要简单使用 `HEAD~N`，应先用 `git log -S spth_version`
定位版本边界。

### 当前工作树待发布改动

以下内容存在于 `f5e08448` 之后的工作树，尚未提交，也没有分配新版本号或写入
`spth_latest_update_info`：

- 新增四阶段幻想星环方舟巨构，可依次建造基址、环形骨架、居住环壳体和核心，
  最终转为可自主航行的移动环形世界舰。
- 幻想星环方舟使用独立事件链、巨构本地化、outliner 图标、唯一建造计数和
  第二个轴基挂载槽；完工舰船具有大幅提高的舰体、护盾、装甲、回复、减伤与
  建筑/模块容量。
- 建立标准方舟武器挂载体系：40 个普通武器模块、7 个轴基模块，覆盖 S/M/L、
  XL、PD、鱼雷和轴基槽位。所有模块图标均使用模块底图与武器图标合成。
- 建立无灵力方舟武器挂载体系：23 个无灵力武器组件全部有对应挂载，其中
  21 个普通模块、2 个轴基模块。
- 无灵力武器改用 `spth_component_attributes_non_genso` 和统一倍率变量；
  21 个原版带命中效果的武器补充弱化版 `on_hit`，并修正普通/超重魔炮、
  冈格尼尔2、梦想封印的图标与槽位。
- 新增 7 个标准舰级太阳反应堆的无灵力版本，使用稀有水晶替代福卡。
- 新增无灵力舰载神社、月面维修器、隙间引擎和 Koishi 反制光环的配套修正，
  并将隙间引擎恢复为原版耗能、冷却和 relay bypass 数值。
- 方舟模块通过 `can_use_touhou_arkship_module` 统一检查可用舰型。
- 调整月都主力舰防御组件、幻想星环方舟、战斗神社、夜雀食堂和突触凝练舰的
  容量与生存数值。
- 拥有任意东方国家认证的国家均可使用神隐星门，不再仅限幻想乡权威。

最近一次用户加载使用 Stellaris `v4.5.4`，启动时间为 2026-11-15 18:47，
未生成新 crash 目录，`error.log` 中也未发现上述新增挂载文件、GFX 或
`touhou_non_genso_parts.txt` 的路径错误。该加载覆盖到无灵力挂载加入之前；
最新挂载、巨构阶段和数值平衡仍需用户实测。

### 下次更新日志草案

版本号和日期待定，确认后再同步 `spth_version`、`spth_version_date`、
`spth_latest_update_info`、历史日志和 Workshop 简介：

```text
1. 新增四阶段巨构“幻想星环方舟”，完工后可转化为可自主航行、自带殖民地的
   幻想星环世界舰。
2. 新增标准方舟武器挂载体系，覆盖普通炮塔、点防、鱼雷、XL 和轴基槽位。
3. 新增无灵力方舟武器挂载，并重做无灵力武器的属性倍率、命中效果和图标。
4. 新增无灵力太阳反应堆，以及其他无灵力舰船组件配套内容。
5. 提高幻想星环世界舰、战斗神社和方舟舰的容量与生存能力。
6. 调整月都主力舰防御组件及相关舰船平衡数值。
7. 东方国家认证现在均可使用神隐星门。
8. 修复方舟舰建造、组件槽位、科技解锁和相关本地化问题。
```

### 建议 Git commit message

```text
feat: expand arkship construction and non-genso loadouts

- Add the four-stage Gensokyo ringworld arkship megastructure and events.
- Add standard and non-genso arkship weapon mounts with composite icons.
- Add weakened non-genso on-hit effects, reactors and scaling attributes.
- Rebalance arkship, starbase and lunar capital durability and capacities.
- Unify Touhou gateway access and complete localisation for new content.
```

## 3. 本轮已经落地的关键改动

### 特殊项目民用舰船兼容

- 新增 `common/inline_scripts/special_projects/touhou_special_project_requirements_*.txt`
- 使用 `triggered_requirement` 兼容科研船、工程船、殖民船、运输船及方舟舰
- 已迁移多项科研特殊项目、先驱者项目和 `spth_disable_habitat_shield_2`
- `TOUHOU_MAKAI_SKY_CONTROLL_REPAIR` 的原 `requirements` 已注释，并通过科研船
  和四艘工程船的 triggered requirement 表达

修改旧式 `requirements` 时，不要同时保留旧块。旧 `requirements` 和
`triggered_requirement` 会同时参与判定，通常表现为更严格而不是替代。

### 舰船性能优化

- 东方主力舰船和月都主力舰船通过
  `common/inline_scripts/ship_sizes/touhou_optimize_evaluation_resource.txt`
  将 `evaluation_resource` 切换为隐藏资源 `touhou_ship_combat_power`
- 在资源表末尾使用
  `touhou_optimize_evaluation_resource_upkeeps.txt`
  添加 `+1` 和带 `exists = owner` 的 `-1` upkeep 抵消
- 目标是降低选中大规模舰队时资源表重算造成的 UI 卡顿

不要把这个 helper 放到 `resources` 块中的错误位置。`evaluation_resource` 是
ship size 顶层字段；upkeep 抵消必须位于 `resources = { ... }` 块末尾。

### Hellsing 法令

- `hmx_hellsing` 当前 `length = 120`，冷却 `360` 天
- 达到全部可用舰坞上限时设置冷却旗标，使 `allow` 失效并终止法令
- `on_disabled` 清除运行变量、关闭所有活动 Hellsing 舰坞并重新开始完整冷却

`allow` 会在法令活动期间重新判定。把冷却旗标放进 `allow` 会让活动法令自动
停用，这是当前实现有意使用的终止机制，不是普通“只禁用按钮”。

未来若希望“冷却只阻止再次启用、不打断当前运行”，必须用
`has_edict = hmx_hellsing` 与独立 stop 旗标区分两种状态。当前需求仍是
“到期或达到上限就终止并冷却”。

持续时间的最终设计存在未决偏差：项目讨论中曾提出 90 天，当前代码和已在
2.5.3 日志中发布的值都是 120 天。修改前先得到明确设计决定。

### 领袖复活

- 手动复活入口通过
  `common/inline_scripts/effect/touhou_leader_function_respawn_$ID$`
  选择统治者专用逻辑
- `spth_effect_respawn_ruler_leader` 只在领袖于开局时被选为统治者时使用
- 空 inline script 是有语义的；文件存在但内容为空时不会报 missing，也不会
  注入额外 effect

修改动态 inline script 文件名时必须使用小写 ID，并确认对应文件存在。不要依赖
CWTools 推断 inline script 内部 scope，CWTools 可能返回 `unknown`。

### 灵潜重写与 UI

- `events/touhou_shroud_events.txt` 中可见事件统一接入
  `spth_special_diplomacy_window` 和 `spth_special_diplomacy_option`
- `toho.8300` 中多个 `shroud.*` 入口已转发到 `touhou_shroud.*`
- `toho.8301` 第一个 desc 已排除以下两种危机结束状态：
  `spth_lunar_capital_psi_crisis_ended`、`ag_zeta_crisis_ended`

当前日志仍报告 `toho.8301` 重复 triggered localization。两个危机同时结束时，
第二个和第三个 desc 仍会同时命中并重复输出 `.14/.16/.17/.18/.20/.21/.22`。
后续应让这两个 desc 互斥，或用 `if / else_if` 在同一个 desc 中选择文本。

## 4. Stellaris 脚本语义

### `triggered_requirement`

- `trigger` 的作用域是 Fleet，不是 country，也不是固定为当前项目舰船
- `count` 代表满足条件的相邻 Fleet 数量，不是舰船数量
- Fleet scope 中的 `any_owned_ship` 会遍历当前舰队中的舰船
- 科研船通常独立编队，因此 `count = 2` 通常等价于两艘科研船；但两艘科研船
  编入同一舰队时不满足两个 Fleet 的计数
- 领主要求与舰船要求都在同一个 Fleet 上判定

不要根据字段名把 `any_owned_ship` 误判为全国任意舰船。相关语义已由 Stellaris
触发器文档确认。

### Inline script

- `script` 路径相对 `common/inline_scripts/`，不带 `.txt`
- 动态路径中的 ID、leader class 和开关值必须与实际文件名大小写一致
- 文件存在但为空是合法用法，可用于 `leader_no`、`leader_class_any` 等分支
- 游戏未找到脚本时可能把展开结果视为空；不要把这种现象当作类型安全或语义
  校验，仍应保证文件和分支明确存在

### 事件、scope 和 on_action

- 事件 scope 错误通常只写日志，不会中止游戏
- 脚本条件不应依赖短路求值来保护错误 scope
- `on_megastructure_built` 和 `on_megastructure_upgraded` 的参数 scope 不同，
  接入前检查原版 `on_actions` 注释
- 覆盖原版事件 ID 会依赖加载顺序并可能产生重复 ID；优先使用 MOD 命名空间和
  转发

## 5. 本地化

- 各语言文件必须保留 UTF-8 BOM 和语言头，例如 `l_english:`、`l_french:`
- 更新英文基准后运行：

```bash
utils/loc/sync_english_localisation.sh --dry-run
utils/loc/sync_english_localisation.sh
```

- 同步目标是除 `simp_chinese` 外的所有已支持语言
- 唯一排除项是
  `localisation/russian/replace/spth_vanilla_repair_l_russian.yml`
- 同步工具只替换首行语言头，正文使用英文基准，并保持幂等
- 不要在同步后意外覆盖中文独立翻译或该俄语文件

## 6. 验证流程

最低要求：

```bash
git diff --check
```

功能修改后：

1. 用 CWTools Stellaris Language Server 检查语法和未知键。
2. 启动当前兼容版本，确认新增文件实际加载。
3. 检查 `error.log` 中本项目路径、scope、除零、未知 effect 和本地化错误。
4. 对特殊项目、法令和 UI 做游戏内触发测试，不以“启动无解析错误”代替运行验证。
5. 性能修改使用同一存档、同一镜头和同一舰队组合做 A/B，不要混入模型渲染变量。

当前已知日志噪声和待办记录在 [local_handoff.md](local_handoff.md)。

## 7. 常用工具

- `utils/loc/sync_english_localisation.sh`：英文基准同步，支持 `--dry-run`
- `utils/gen_component_vars.sh`：组件变量同步
- `utils/ships/gen_ags_psionic_variants.sh`：AGS 灵能舰船颜色差分
- `utils/touhou_gen_ship_modifier.sh`：舰船修正生成
- `utils/touhou_gen_ship_modifier_loc.sh`：舰船修正本地化生成
- `utils/izakaya/process_recipe.sh`：夜雀食堂配方处理
- `utils/tech/tech_dependency_analyzer.cpp`：科技依赖分析

生成器输出必须人工检查，避免覆盖手工修正。

## 8. 项目技能

- [stellaris-script-verification](skills/stellaris-script-verification/SKILL.md)
  - 脚本、scope、inline script、特殊项目、法令和日志验证
- [touhou-fleet-performance](skills/touhou-fleet-performance/SKILL.md)
  - 舰队 UI 卡顿、`evaluation_resource`、资源表和 A/B 分析
- [touhou-release](skills/touhou-release/SKILL.md)
  - 版本边界、更新日志、本地化同步和发布检查

## 9. 交接原则

- 先读目标文件附近注释和原版同类实现
- 不覆盖来源不明的未提交修改
- 不把 CWTools 能解析等同于游戏内语义正确
- 不因性能优化静默改变 scope、触发概率、资源成本或事件时序
- 每次修复记录实际验证条件、日志位置和仍未覆盖的运行场景
- 4.6 preview 资料见
  [STELLARIS_4.6_PREVIEW_COMPARISON.md](STELLARIS_4.6_PREVIEW_COMPARISON.md)
