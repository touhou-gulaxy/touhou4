# Stellaris 4.6 原版覆盖与叙事变更审计

最后更新：2026-11-24

## 1. 基线与范围

本文是正式版审计，取代 `.agent/STELLARIS_4.6_PREVIEW_COMPARISON.md` 中
基于 4.6 preview 的待验证结论。

审计对象：

- 当前游戏：Stellaris `4.6.0 Corona Borealis`
- 模组：东方繁星录 `2.6.0`
- 模组发布声明：`supported_version="v4.6.*"`
- 原版 Git：`/home/koishi/SteamLibrary/steamapps/common/Stellaris`
  - 本地基线标签：`v4.5.4`
  - 基线提交：`76fc980 Initial Stellaris content snapshot`
  - 当前 4.6 内容位于未提交工作树
- 模组目录：`/home/koishi/IdeaProjects/touhou4`

原版工作树包含大量正常的 4.6 升级改动和新增文件，不能视为需要清理的脏工作树，
也不要执行 `git reset`、`git checkout -- .` 或提交原版文件。比较时应显式使用：

```bash
git -C /home/koishi/SteamLibrary/steamapps/common/Stellaris diff v4.5.4 -- <path>
```

本次只修改模组文档，没有修改原版文件或模组游戏内容。

## 2. 结论摘要

模组可以在 4.6 加载，但原版覆盖层尚未完全等价于 4.6：

1. 若干完整覆盖的 game rule、scripted trigger、scripted effect 和实体定义
   丢失了 4.6 新增条件或分支。
2. Nomads 与 Grand Archive 的一部分共享事件被 `always = no` 或空
   `immediate` 主动停用；它们不是静态加载错误，但对应玩法实际缺失。
3. `shroud.5310` 仍使用旧 UI，`paragon_2.1005` 缺少新的战利品逻辑。
4. 4.6 新增的绝对伦理、意识形态、`Lost Makers`、`Golden Generation` 和
   `Great Mosaic` 改变了若干原版政治与旧先驱叙事，模组文本应避免继续假定
   只有“普通/狂热伦理”，也不应把先驱历史写成完全静态的单一版本。
5. define 键、zone 语法、旧战争回调名和 `descriptor.mod` 已基本完成迁移；
   当前主要工作是行为合并、停用内容重新设计和文本语义同步。

## 3. 原版覆盖缺口

### 3.1 优先修复

| 项目 | 当前情况 | 影响 |
| --- | --- | --- |
| `can_jump_drive` | 缺少 `is_system_locked = no` | 锁定星系可能仍允许跃迁 |
| `can_species_be_assembled` | 缺少 `NOT = { is_being_integrated_by = root.owner }` | 正在被同化的物种可能仍能组装 |
| `can_enter_system_by_jump` | 规则与 `can_enter_system_by_jump_trigger` 都缺少 `has_star_flag = hyperthermia_system` | 可将舰船跃迁进 Galactic Hyperthermia 核心星系 |
| `system_blocks_sensors` | 缺少 `has_star_flag = surveillance_supercomputer_system` | 监视超级计算机星系可能错误提供完整传感器信息 |
| `orbital_bombardment_effects` | 缺少 4.6 的 `bombard_plundering` state | Nomads 的掠夺轰炸模式没有对应实体表现 |
| `shipyard` | 仍使用必选 `solar_system =`，原版已使用 `solar_system? =` | 特殊或无星系星港下可能判断失败 |

源码位置：

- `common/game_rules/zzzz_toho_game_rules.txt:185`
- `common/game_rules/zzzz_toho_game_rules.txt:317`
- `common/game_rules/zzzz_toho_game_rules.txt:376`
- `common/game_rules/���_touhou_gamerules.txt:194`
- `common/scripted_triggers/���东方覆盖_triggers.txt:354`
- `gfx/models/planets/�touhou_overwrite_planet_entities.asset:63`
- `common/starbase_modules/���touhou_overwrite_starbase_modules.txt:10`

`can_orbital_bombard` 和 `has_full_sensor_range` 已经保留原版 4.6 的主要放行
分支，不应按旧 preview 报告再次整体替换。

### 3.2 Scripted trigger

| 覆盖键 | 缺口 |
| --- | --- |
| `can_be_leader` | 丢失 `NOT = { has_trait = trait_syncretic_proles }` |
| `can_fill_specialist_job_trigger` | 丢失 `citizenship_cruise_passenger` 排除 |
| `politician_is_science_director` | 使用 `has_building`，原版为 `has_active_building` |
| `physicist_is_astral_researcher_trigger` | 使用 `has_building`，原版为 `has_active_building` |
| `is_organic_species` | 仍按机械 trait 反推，原版按 `BIOLOGICAL/LITHOID` archetype |
| `is_robotic_species` | 仍只检查 `trait_machine_unit/trait_mechanical`，原版使用 `species.is_robotic` |

源码位置：

- `common/scripted_triggers/���东方覆盖_triggers.txt:180`
- `common/scripted_triggers/���东方覆盖_triggers.txt:231`
- `common/scripted_triggers/���东方覆盖_triggers.txt:289`
- `common/scripted_triggers/���东方覆盖_triggers.txt:303`
- `common/scripted_triggers/mode_species_job_trigger.txt:1`

`has_psionic_leader_trait` 同时检查 `has_chosen_one_leader_trait` 和
`has_patron_chosen_one_leader_trait`，在语义上等价于 4.6 的
`has_any_chosen_one_leader_trait`。但模组附加的 Ancient Empire trait 查询
在没有安装对应兼容模组时会触发 `cannot find trait with key` 错误，应增加
存在性和兼容开关保护。

### 3.3 Scripted effect

- `finish_first_contact_effect` 缺少 Champions Forge enclave 完成接触后的
  `nomads.155`。
- `arkship_exodus_jump_system_effects` 改用了无 attacker 参数的
  `vanilla_overwrite_damage_ship`，丢失原版
  `attacker = event_target:arkship`，可能影响击杀归属和击杀回调。
- `dyson_gun_damage_ship` 仍按防御网格建筑和舰种判断减伤；4.6 原版改为
  `defense_platform_synchronized_shielding` 政策。当前实现会让政策与方舟武器
  减伤规则脱节。
- `dyson_gun_devastate_colony` 调用 `touhou_has_planet_shield`，在未安装
  Ancient Empire 时日志会出现 `ag_ancient_shield_modifier` 缺失。

`synth_queen_wipe_system`、`synth_queen_mini_wipe_system` 和
`activate_behemoth_superweapon` 的主要差异是模组为防止误毁东方/古代帝国内容
而添加的保护逻辑，本轮没有发现必须直接恢复的原版 4.6 新分支。后续合并时仍应
保留这些保护。

### 3.4 Shroud、协议和星港

- `common/inline_scripts/shroud/aura_wide_modifiers.txt` 缺少 4.6 末尾的
  `triggered_system_modifier = { ideological_push_mult = 0.05 }`。
- `psionic_aura_monthly_decrease.txt` 保留原版基础逻辑并追加东方抑制器/驱散器，
  结构兼容。
- `resource_subsidies_unity` 仍是旧版东方附庸自定义条款，未并入 4.6 的
  `preset_nomadic_purger` 专用条款。
- 附庸战争参与条款与持有地上限条款中存在模组主动平衡数值。不能仅按原版差异
  覆盖；应只补缺失字段，并另外确认 `loyalty_change/monthly_loyalty` 是否为有意
  重新平衡。

### 3.5 绝对伦理

`events/02_th_events.txt` 第 71–120 行仍有 8 处：

```txt
NOT = { has_ethic = ethic_fanatic_* }
```

这些选项用于阻止已经处于最高等级的伦理再次升级。4.6 的绝对伦理不会匹配精确
的 `ethic_fanatic_*`，因此绝对伦理国家仍可能看到降级/重复选择。应改为
`is_fanatic_* = yes/no`。

`common/opinion_modifiers/th_opinion_modifiers.txt` 中已经有独立的绝对伦理分支，
不应把它们合并掉。

### 3.6 共享事件

当前按顶层事件定义扫描，模组与原版至少有 19 个相同事件 ID。旧文档的 21 个口径
混入了仅被调用、但没有在模组中重新定义的事件，例如 `observer.100` 和
`utopia.3024`。

需要重点处理：

| 事件 | 状态 |
| --- | --- |
| `shroud.5310` | 旧自定义窗口；缺少 `base = enclave.405`、`desc_clear`、`option_clear`；龙神选项应迁到新 base |
| `nomads.51` | `always = no`，Shield Realignment checklist 被关闭 |
| `nomads.8004` | `always = no`，Waystation 在 raid war 中的失能流程被关闭 |
| `grand_archive.10095` | 仍被 `always = no`；4.6 当前版本只是短小的 Berserker Glands 突变 |
| `grand_archive.10090` | 仍被禁用；原版逻辑较重，需要性能与行为兼容方案 |
| `grand_archive.9000` | `immediate = {}`，领袖捕获、消息和 `grand_archive.9005` 后续被清空 |
| `paragon_2.1005` | 缺少 Plunderer、Plunderer II 和击杀海军容量缩放奖励；应保留模组新增的 Wandering in the Void 奖励 |

其余共享事件：

- `crisis.8010/8015/8030/8035/8042` 主要增加 `synth_queen_defeated`
  保护，没有发现新的 4.6 内容被删除。
- `crisis.6052` 和 `shroud.4972` 的 stop/resume 脚本确实封装了原版
  `set_update_modifiers_batch`。
- `shroud.4972` 当前以
  `touhou_cannot_destroyed_by_end_of_the_cycle = yes` 作为 `destroy_country`
  条件，语义可疑；若触发器表示“不能被摧毁”，这里应加 `NOT`。
- `first_contact_critters.25`、`grand_archive.3200`、`ancrel.4046`
  的主要差异是模组附加的 on_action 或兼容内容。
- `paragon_2.1020` 基本只有格式差异；`paragon_2.1021` 附加了 Hina 负面 trait
  拦截逻辑。

### 3.7 日志中的真实问题

最新 `error.log` 中，与覆盖层直接相关的可复现问题包括：

- `debug: can jump` custom tooltip 缺本地化；
- `ag_jump_drive_unable_system_tooltip` 缺本地化；
- `has_psionic_leader_trait` 查询未安装兼容模组中的 trait；
- `dyson_gun_devastate_colony` 间接查询未安装的
  `ag_ancient_shield_modifier`；
- 大量可选兼容模组的国家类型、星球类型和领袖岗位引用会产生 deferred key 错误。

重复定义日志本身是覆盖机制的预期结果，不能与上述真实缺键混为一谈。

## 4. 原版叙事与设定变化

以下内容基于原版 Git 的 `v4.5.4..working tree` 差异，主要查看英文脚本、
英文本地化、事件链和新增未跟踪文件。

### 4.1 4.6 的官方主题

`localisation/english/main_1_l_english.yml` 的 `STARTUP_TEXT` 明确说明：

- 4.6 为 `Corona Borealis`，核心扩展是 `Willpower`；
- 派系可以形成能跨边界传播的 `Ideologies`；
- 新增 5 个狂热公民性、绝对伦理和 Force of Will 雄心；
- 新增 3 个起源：Golden Generation、Lost Makers、Great Mosaic。

这不是单纯机制更新，政治理念本身被提升为可传播、可争夺、可发动战争的对象。

### 4.2 意识形态与绝对伦理

4.6 新增 8 个绝对伦理：

- Absolute Authoritarian：秩序和法律高于异议；
- Absolute Egalitarian：平等是最高价值；
- Absolute Militarist：文明以力量和战争维持荣誉与生存；
- Absolute Pacifist：战争应仅存在于历史记录中；
- Absolute Materialist：只以物质证据和客观性指导社会；
- Absolute Spiritualist：灵魂与宇宙超越是存在顶点；
- Absolute Xenophile：多样性和开放是文明潜力的来源；
- Absolute Xenophobe：自给自足、隔离和纯洁是生存真理。

意识形态系统新增：

- `Momentum`：社会对当前意识形态的支持变化；
- `Ideological Reach`：从星系向邻接星系传播；
- `Tenets`：由意识形态 Custodian 制定；
- 意识形态 Custodian、Rivalry、冲突、衰退和叛乱；
- Cultural Envoy 舰船和人口意识形态转化；
- Galactic Hall 可移动巨构及意识形态模块；
- 动态“Impose Ideology”战争目标和意识形态附庸条款。

原版不再只把政治视为帝国内部的固定伦理组合，而是把伦理外化为可在星际社会间
宣传、竞争和颠覆的意识形态。

### 4.3 五个新狂热公民性

- `Warmongers` / `Board of War`
  - 战争是国家运转的核心；
  - 普通社会阶层被军事阶层替代；
  - 开战时及每场舰队战之后，根据对手实力获得战争修正。
- `Open Stars Sovereignty` / `Strategic Networkers`
  - 以开放边境、移民和人口多样性为国家优势；
  - 可获得移民局 Starbase building；
  - 与其他帝国完成首次接触时获得 Unity。
- `Path of the Self` / `Divine Solutions`
  - 通过冥想、精神修炼和领袖培养追求启蒙；
  - 包含灵性修炼 situation、冥想决议、领袖特质和“神童”事件。
- `Social Concord` / `Peace Networks`
  - 将暴力和侵略视为公民责任的失败；
  - 获得调解型外交行动 `Sow Peace`；
  - 和平时期以及银河共同体全体和平时期获得额外收益。
- `Megacity Architects` / `Megacity Industries`
  - 将城市视为文明最合理的组织形式；
  - 强化城市化、住房和建筑槽位扩张；
  - 与 Arcology Project 路线联动。

这五个公民性都把伦理从抽象修正值转化为社会制度，与新的意识形态系统配套。

### 4.4 新起源

#### Great Mosaic

一个由多个古老文化、地区或机器子系统共同形成的文明。它在进入太空前通过
跨国工程和共同目标完成整合，并保留多种治理与非治理伦理。

- 普通有机版本强调 plurality；
- 蜂群版本强调一个 Mind 对多个有限自治节点的统合；
- 机器版本强调 Core Intelligence 对多功能子系统的统合；
- 机制上允许初始人口混合普通/狂热伦理，并额外获得一个 civic。

它适合用来解释“多元文明最终合流”，与现有以单一幻想乡权威为中心的设定不同。

#### Golden Generation

经历资源战争后，社会认定决定文明存亡的不是单纯力量，而是领导能力。
国家从婴儿期筛选学生，由精英学院培养行政、军事和科学领袖。

事件链包括：

- 领袖毕业与招募；
- 公众期待、丑闻、失势、同窗竞争和议会协同；
- 暗杀、克隆、延寿与死亡纪念；
- 对学院制度的肯定、反感及最终意识形态化。

它提供了“制度化培养超凡领袖”的完整叙事，可与模组中的特殊领袖体系对照，
但不应简单把所有东方领袖解释为同一学院产物。

#### Lost Makers

一个不知道自己最初用途的机器智能。其核心记忆约有 4.71% 是损坏且不可执行的
古代代码，机体还包含无法安全拆解的无效硬件。

起源链条会：

- 在本国星系生成地下考古遗址；
- 通过先驱文物恢复古代子程序；
- 开启通向未知制造者遗迹的虫洞；
- 逐个恢复“Project [Species Name]”的原始指令；
- 最终选择恢复或永久删除原初用途；
- 根据所抽中的先驱，生成不同的制造者历史。

这是一套“可变的旧文明起源叙事”，并非所有结果会在同一局中同时成立。

### 4.5 Lost Makers 的先驱分支

| 先驱/制造者 | 机器原本用途 | 关键背景 |
| --- | --- | --- |
| Vultaum | 近战战斗娱乐平台 | 机器是虚拟现实观众观看的 “Brawl Bots”，文明崩溃后发展出原始机器社会 |
| Yuht | 繁殖孵化场维护者 | 受 “Ministry of Breeding” 管理，负责 sterilize、驱除掠食者和照护幼体；Jabbardeeni 网络攻击摧毁原始指令 |
| First League | 高级战场机器人 | 对 Rikim 作战；V'ropak 工程师以秘密 kill switch 毁掉主指令 |
| Irassian | 自动隔离与防疫系统 | 面对 Javorian Pox，最终演变为无差别人口削减 |
| Cybrex/Kuur | 更可控的 Cybrex 替代品 | Kuur 先创造 Cybrex，后因威胁识别系统过于激进而研发本机种；Cybrex 发现自身将被淘汰后灭绝 Kuur |
| Zroni | 灵能/维度研究无人机 | 试图部分突破 Shroud；Zroni 进入 Shroud 的灾难破坏原始指令 |
| Baol/Grunur | 焚毁 Baol 森林的自动灭绝系统 | Grunur 为彻底消灭 Baol 而制造；机器社会在 Grunur 消亡后才脱离“只会燃烧”的用途 |
| Inetian | 陪伴与感官服务机器 | Inetian 为多种族公民制造定制伴侣；制造者衰亡后原始指令因长期不用而遗失 |
| adAkkaria | 灵能“Inner Storms”实验对象 | adAkkaria 用有感知的机器替代对生物的残酷实验，机器被认为只会模拟痛苦，但日志暗示并非如此 |

这使旧先驱不再是静态百科条目。尤其是在 `Lost Makers` 视角下，玩家会直接看到
自己与先驱、Baol、Grunur、Kuur 和 Cybrex 的关系。

### 4.6 旧先驱、Baol、Zroni 与 Cybrex 的文本扩展

- 普通先驱事件保留原版结局，但 `Lost Makers` 可以获得更完整的历史复盘：
  Vultaum 的模拟宇宙自杀、Yuht 对 Jabbardeeni 的灭绝对抗、First League 的
  饥荒崩解、Irassian 的 Javorian Pox 死亡世界，以及 Cybrex 的远征与退隐。
- Baol 事件新增 Lost Makers 专属对白。Baol 会察觉到机器形态与“burners”相似，
  玩家可问“你认得我们吗？”，并可选择“他们也会像祖先一样燃烧”。
- Baol 星界裂隙新增 Lost Makers 分支：机器会发现未来/平行宇宙中的 Grunur
  正是自己的制造者，并可能承认自己曾为灭绝 Baol 而造。
- Zroni 星界裂隙新增 Lost Makers 分支：可以把 Zroni 视为可能的制造者，
  询问机器与制造者之间的关联，甚至以“你创造了我们”回应 Zroni。
- 肃正协议事件中的 Cybrex 新增 Lost Makers 对话：
  “Well met, fellow children of the Kuur.”
  Cybrex 还承认 Lost Makers 的诞生导致其误判 Kuur 为威胁，并曾计划摧毁
  Lost Makers。

这意味着任何模组文本若写死“Baol 只被无名敌人灭绝”或“Cybrex 起源与 Kuur
无关”，在新版中会显得残缺。更稳妥的写法是使用兼容性限定子句，或避免否定
这些 route-specific 解释。

### 4.7 政治与制度文本的重写

- 派系加入/拥护不再只写死九种旧派系的伦理名称，而是使用动态
  `GetType`/`GetPopsGuidingEthic`。
- “Impose Ideology” 和 Ideology casus belli 使用动态名称，可随具体意识形态变化。
- Galactic Community 代表由 `Official` 担任 Delegate，其技能等级决定外交权重；
  旧文本中的 Envoy 已大量替换。
- Enmity tradition 的竞争对象从普通 Rival 扩展到 Rival Ideology。
- 领袖背景界面新增 birthplace、job、ethic、stratum、assignment 等动态字段。
- 再生概念被拆分为每日固定恢复、Regen Rating、Shipyard Repair 和
  Station Repair。

### 4.8 文本一致性修正

4.6 还批量修正了以下文本，不应误判为剧情重写：

- 大量 `GetPlanetHabitat` 被替换为 `GetPlanetMoon`，用于语境中更准确的
  “卫星/天体表面”描述；
- 领袖死亡、葬礼和 Paragon 事件加入性别中立动态代词；
- 多处 typo、单复数、图标键和 modifier tooltip 格式修正；
- `ai_servitude_desc` 修正 “Artifical” 为 “Artificial”；
- `economic_policy_civilian_desc` 修正 “become before” 为 “come before”。

## 5. 对东方繁星录的适配建议

推荐顺序：

1. 先修 game rule、物种岗位 trigger、传感器和轨道轰炸实体。
2. 合并 absolute ethics 的 8 处事件条件，并清理可选兼容模组的 trait/type 缺键。
3. 修 `shipyard`、`shroud.5310`、Dyson 政策判断、arkship attacker 和
   Shroud `ideological_push_mult`。
4. 重新启用并前置过滤 Nomads 与 Grand Archive 事件，避免继续用空
   `immediate` 关闭整个内容链。
5. 合并 `paragon_2.1005`、Champions Forge 接触奖励和 Nomadic Purger 协议条款。
6. 复查所有与“伦理最高等级”“外交使节”“派系”“意识形态战争”有关的文本，
   优先使用 4.6 的动态 localization scope。
7. 模组新增古代文明事件时，可参考 Kuur、Grunur、Jabbardeeni、Rikim、
   V'ropak、Inetian 和 adAkkaria，但不要把 Lost Makers 的可变起源全部视为
   同一时间线的固定事实。
8. 下一次原版更新后，重新生成共享事件、on_action 和直接覆盖键清单；不要沿用
   本文的行号或数量作为永久结论。

## 6. 已核查通过或无需迁移的项目

- `common/defines/*` 中的当前覆盖键均仍存在于 4.6，没有发现已删除 define 键。
- `common/zones` 的 `include/exclude` 已基本迁移；剩余
  `common/inline_scripts/zones/spth_common_zone_slots.txt` 使用 `include`
  是 zone slot 语法，不应改成 `included_buildings`。
- `descriptor.mod` 已声明 `v4.6.*`。
- 模组中没有 `on_entering_war` 旧回调残留。
- `can_orbital_bombard` 已保留 4.6 的
  `has_country_flag = can_orbital_bombard` 放行分支。
- `has_full_sensor_range` 已保留原版 `spy_orb_4/spy_orb_restored` 条件。
- `psionic_aura_monthly_decrease.txt` 的东方追加逻辑没有删除当前原版基础条件。
- `crisis.6052`、`shroud.4972` 的 stop/resume 脚本等价封装了 modifier batch。
- `gfx/FX/buttonstate_rendertarget_hologram.shader` 与 4.6 原版结构仍一致；
  移除高光和边缘叠加属于模组既有的 Linux/macOS 视觉兼容选择，应实机确认效果。

## 7. 原版 Git 使用约定

- 基线快照标签：`v4.5.4`
- 正式 4.6 内容尚未提交；审计结论来自工作树。
- 不把目前 1150 余个修改文件当作人工改动。
- 不提交、不清理、不重置原版仓库。
- 新版本到来时，先打新的本地 baseline tag，再更新工作树，随后重新执行本文清单。
