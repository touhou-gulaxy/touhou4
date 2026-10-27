# MOD 联动与兼容性

本文记录当前代码中存在实际入口、判定或专用内容的联动。

兼容判定主要使用以下全局标记：

- `has_ancient_empire_mod`：古代帝国
- `install_kuat_mod`：Star Wars: Kuat Ancient Empire
- `has_NYTO_mod`：GF: Nyto
- `UGRC_installed`：Universal Game Rules Patch 兼容框架

这些 MOD 均不是硬依赖。未安装对应 MOD 时，相关选项、事件和修正应被触发器跳过。

## 古代帝国 / Ancient Empire

主要实现位于：

- `events/th_ag_interaction_events.txt`
- `common/special_projects/th_ag_interaction_projects.txt`
- `common/traits/spth_ag_traits.txt`
- `common/inline_scripts/events/spth_ag_*`
- `common/scripted_triggers/spth_other_check_triggers.txt`

### 古代 AI

以下古代 AI 领袖可获得额外互动与“灵力理论学习”相关内容：

- 星光二号 / Starlight II
- 幻星一号 / Illustelia I
- 曙光之星 / Aurora
- 远星铸者 / Constructor of Distance Stars

相关功能包括：

- 特定条件下追加对话和外交事件。
- 学习灵力理论后获得专用领袖特质。
- 星光二号和幻星一号可分别协助研究 H 星域破解及孤星华光舰载数据库。
- 研究完成两名古代 AI 的灵力理论后，可快速兼容其余古代 AI。
- 古代 AI 特质覆盖科研、行政和舰队岗位的对应加成。

### 古代探索与舰船

- 古代探索舰队的建造界面可加入幻想乡相关舰船。
- 古代探索装备界面可加入隙间存储、幻想乡导航、结界护盾等选项。
- 幻想乡自动环形世界可使用古代帝国建设模板。
- 可研究并应用部分古代文明设施、虚境和灵能相关机制。
- 特殊领袖“幻想乡缘起”对部分古代设施与虚境敌军有额外伤害。

### 战斗与机制兼容

- 古代帝国单位使用专用舰船标签，供识别、伤害和交互触发器使用。
- 特定古代国家或隐形阵营不启动部分通用战斗系统，以减少冲突。
- 古代帝国的 disable/额外生命流程与幻想乡残机系统做了兼容处理。
- 古代 AI、异形和部分古代造物不会触发不适用的幻想乡通用战斗机制。
- `psionic_aura_monthly_decrease` 增加幻想乡灵能抑制站/增幅站的衰减修正。
- `shroud.4972` 终末循环毁灭系统时会保护古代帝国相关星系。
- 古代帝国 MOD 更新后，应优先检查 `ag_*` 事件目标、领袖标记和舰船标签。

## Star Wars: Kuat Ancient Empire

主要实现位于：

- `common/ship_sizes/th_ship.txt`
- `common/ship_sizes/lunar_capital_ships.txt`
- `common/ship_sizes/touhou_fe_union_ships.txt`
- `common/inline_scripts/ship_sizes/spth_kuat_*`
- `events/touhou_ship_actions_events.txt`
- `common/global_ship_designs/th_kuat_designs.txt`

### 舰船分类

- 可建造的幻想乡普通舰船注册为 Kuat Standard Ship。
- 旗舰和支援旗舰分别注册为 Kuat Flagship/Sub-Flagship 类别。
- 超级旗舰注册为 Kuat Super Flagship。
- 远程轰炸舰识别为 Kuat Support Ship。

### 特殊交互

- 舰队界面可开关 Kuat Support Nexus 风暴。
- 可在相关条件满足时生成、扩展或关闭该风暴。
- 可使用专属指令生成一支符合 Kuat 舰船分类的幻想乡召集舰队。
- 特殊领袖具备针对 Kuat 国家的额外伤害修正。
- 因 Kuat 已有较强索敌权重下限，部分幻想乡索敌压制机制会主动关闭以避免叠加异常。

## GF: Nyto

主要实现位于：

- `common/scripted_triggers/spth_other_check_triggers.txt`
- `common/script_values/spth_nyto_values.txt`
- `events/spth_crisis_events.txt`
- `common/static_modifiers/TH_dynamic_modifiers.txt`
- `common/button_effects/GF_nyto_effect_button.txt`
- `localisation/*/replace/spth_cosmo_replace_l_*.yml`

联动内容：

- 检测 `has_NYTO_mod` 后启用 Nyto 难度系数。
- 隙间之里天灾的加密难度会参考 Nyto 情报/解密修正。
- 提供 Nyto Boss 血量条按钮资源。
- 覆盖 `crisis.7516.desc.worm` 与 `crisis.7516.desc.greatwound`，使其显示联动文本。

## Universal Game Rules Patch 与其他兼容层

`common/game_rules/zzzz_toho_game_rules.txt` 和
`common/scripted_triggers/!!!ph_MOD_triggers@FW.txt` 提供较广泛的
game-rule 兼容层。

当前规则中明确包含或引用了以下类别的兼容判定：

- Universal Game Rules / UGRC 通用国家标记
- Ancient Empire
- Kuat Ancient Empire
- ACOT / Gigastructural Engineering 相关国家类型
- SOFE、BIM/BWM、MEM 等大型兼容体系
- WSG/SWG 舰娘类 MOD
- Voyage to Eternity / 久远
- Azur Lane 相关物种与政体
- 多彩银河、精灵、SC、SRA、THL、GF 等兼容标记

这部分主要保证轨道轰炸、跃迁、物种组装、领袖资格、传感器、虚境探索等
通用规则在多个大型 MOD 同时存在时不会互相锁死。

该文件不是“与所有这些 MOD 都有完整剧情联动”的声明，而是：

1. 原始 MOD 未安装时，占位 trigger 保持安全默认值。
2. 已安装时，允许对应国家类型或机制通过通用规则。
3. 对原版 game rule 做整体替换，因此每次群星更新后必须重新比对。

## Dark Blue UI / 舰队界面

- 幻想乡特殊舰船动作通过 fleet view 的 scripted action 按钮提供。
- `common/inline_scripts/ship_components/touhou_ship_action_button.txt` 定义通用按钮。
- `interface/touhou_ship_actions_menu.gui` 提供相关菜单。
- 暗蓝 UI 更新后应验证按钮位置、图标和 `custom_gui_option` 是否仍正常显示。

## 原版与其他内容的兼容修改

- 轨道轰炸 entity 覆盖兼容古代帝国、ACOT、灰烬、星舰少女等 MOD。
- 轨道轰炸、跃迁、传感器和物种组装规则兼容多个大型 MOD 的国家类型。
- 终末循环、虚境终局、天灾清场和天灾星系毁灭逻辑增加了幻想乡保护判定。
- `grand_archive`/`nomads` 的高频 `on_damage_taken` 事件被禁用或精简，以降低战斗性能开销。
- 部分议会职位与 Voyage to Eternity、古代帝国议会定义兼容。

## 维护检查

更新任一联动 MOD 后至少检查：

```text
has_ancient_empire_mod
install_kuat_mod
has_NYTO_mod
UGRC_installed
```

同时检查对应事件目标、global flag、国家类型、领袖 flag 和组件标签是否仍存在。
这部分错误通常不会阻止游戏启动，只会使联动选项静默消失或产生 scope 错误。
