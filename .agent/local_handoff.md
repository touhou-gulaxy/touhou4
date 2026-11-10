# 本地开发交接

最后更新：2026-11-10 20:47 Asia/Shanghai

本文只记录当前设备和当前工作树的状态。接手前先执行：

```bash
git status --short --branch
git log -5 --oneline --decorate
```

## 1. 当前 Git 状态

- 工作目录：`/home/koishi/IdeaProjects/touhou4`
- 分支：`dev/4.6`
- HEAD：`5c78ae2d9e83ed343445d5ce01093dd0efd8af9d`
- 相对 `origin/dev/4.6`：领先 1 个提交，尚未 push
- 已跟踪文件改动：
  - `events/02_th_events.txt`
  - `events/touhou_shroud_events.txt`
- 已有未跟踪文件，勿删除或覆盖：
  - `utils/automation/util.hpp`
  - `utils/automation/util.inl`

### 4.6 preview 暂时还原

为方便先把兼容版本发布到创意工坊，已执行：

```bash
git revert --no-edit 495a9af42ae585c811e4666c3759258a096a830d
```

生成的 revert 提交：

```text
5c78ae2d9e83ed343445d5ce01093dd0efd8af9d
Revert "port 4.6 overwrite and new generic compatibility"
```

反向提交包含 8 个文件，`5 insertions / 188 deletions`。已核对这些路径在
revert 后与 `495a9af4^` 一致。

创意工坊发布完成并确认不再需要发布当前状态后，恢复内部 preview 改动：

```bash
git revert --no-edit 5c78ae2d9e83ed343445d5ce01093dd0efd8af9d
```

执行前重新确认没有新的相关冲突。该恢复操作会创建新提交，不会自动 push。

## 2. 灵潜事件重写

### 原版候选事件

4.5.3 与当前可访问的 4.6 preview 中，
`on_patron_delve_failed` 的候选集合一致：

- 28 个真实 `shroud.*` 事件
- 1 个 fallback：`utopia.3002`
- `0 = shroud.3` 只是零权重占位，不应视为真实候选

相关路径：

- 原版 4.5.3：
  `/home/koishi/SteamLibrary/steamapps/common/Stellaris/common/on_actions/00_on_actions.txt`
- 4.6 preview：
  `/home/koishi/ghidra_decompiled/Stellaris4.6_preview/common/on_actions/00_on_actions.txt`
- MOD override：`common/on_actions/th_on_actions.txt`
- MOD 重写事件：`events/touhou_shroud_events.txt`
- 入口事件：`events/02_th_events.txt` 中的 `toho.8300`

### 已修正的转发

`events/02_th_events.txt` 当前有三项未提交改动，分别把错误指向原版事件的分支
改为 MOD 重写事件：

```text
shroud.3060 -> touhou_shroud.2060
shroud.3065 -> touhou_shroud.2065
shroud.3070 -> touhou_shroud.2070
```

这三项改动不是本轮自定义 UI 修改产生的，工作区中已经存在，未回退。

`toho.8300` 本体仍逐项保留全部 28 个原版 `shroud.*` ID 和
`utopia.3002` fallback。按其原始 ID 计算没有遗漏；MOD 重写路径的上述三项已补上。

## 3. 灵潜自定义 UI

### 已完成

`events/touhou_shroud_events.txt` 已对全部可见事件补齐：

```txt
custom_gui = spth_special_diplomacy_window
custom_gui_option = spth_special_diplomacy_option
```

字段统一放在 `force_open = yes` 前。

结构统计：

- 顶层 `country_event`：84
- 可见事件：81
- 隐藏事件：3（`touhou_shroud.10000`、`.10001`、`.2064`）
- 81/81 可见事件都有 `diplomatic = yes`
- 81/81 都有 `force_open = yes`
- 81/81 都有 `picture_event_data`
- 81/81 的 `picture_event_data` 都包含
  `room = spth_ag_excited_shroud_room`
- 81/81 都有成对的 `custom_gui` 和 `custom_gui_option`
- 原版 `picture` / `event_picture` / `event_picture_background` 字段数量：0
- 81/81 可见事件都只有一个 `diplomatic_title`

`.2281` 至 `.2288` 原先各有两个 `diplomatic_title`。第二个
`diplomatic_title = BLANK_STRING` 已删除，保留：

```txt
diplomatic_title = shroud.3.diplo_title
```

本轮 diff：

- 新增 162 行（81 × 2 个 UI 字段）
- 删除 8 行重复标题
- `git diff --check` 通过

UI 定义：

- `interface/spth_special_diplomacy_ui.gui`
- 窗口名：`spth_special_diplomacy_window`
- 选项名：`spth_special_diplomacy_option`

可参考写法：

- `events/02_th_events.txt` 的 `toho.8200`
- `events/spth_astral_rift_events.txt` 的 `spth_astral_rift.129`

### 运行验证状态

当前 `error.log` 生成于 2026-11-10 20:44:28。
`events/touhou_shroud_events.txt` 的最后修改时间为 20:41:12，因此本次启动确实
加载了加入 UI 字段后的版本。

日志中没有以下项目的解析错误：

- `events/touhou_shroud_events.txt`
- `custom_gui = spth_special_diplomacy_window`
- `custom_gui_option = spth_special_diplomacy_option`
- `picture_event_data`

但这不等于 UI 已完成运行时验证：`game.log` 本次只记录了
`gensokyo_start.1145` 和 `operation.1001` 的选择，没有实际触发灵潜重写事件。
下次需要在游戏内或控制台触发至少一个 `touhou_shroud.2xxx` 事件，确认聊天框、
选项按钮、背景房间、人物和被截断文本的布局。

建议抽查：

```text
touhou_shroud.2001
touhou_shroud.2100
touhou_shroud.2281
```

## 4. error.log 分析

日志路径：

```text
/home/koishi/.local/share/Paradox Interactive/Stellaris/logs/error.log
```

本次运行信息：

- 游戏版本：`Cygnus v4.5.3`
- 启动时间：20:42:48
- 日志最后时间：20:44:28
- 文件大小：5,140,192 bytes
- 行数：25,909
- 启动耗时：47.291 秒
- 首次缓存重算：1.844 秒

日志包含多个大型兼容 MOD 和本地启用的其他 MOD。很多错误来自公共兼容层或
外部内容，不能全部归因于本仓库。以下只列对本项目最有价值的项目。

### 4.1 当前 UI 改动结论

本次自定义 UI 字段没有引入启动或解析错误。81 个可见事件在引擎加载阶段通过。
真实界面仍需触发事件后目测。

### 4.2 `toho.8301` 重复 triggered localization

日志共记录 7 条：

```text
toho.8301.desc.14
toho.8301.desc.16
toho.8301.desc.17
toho.8301.desc.18
toho.8301.desc.20
toho.8301.desc.21
toho.8301.desc.22
```

都在 `events/02_th_events.txt` 的 `toho.8301` 描述块中。原因是第一个
`desc` 只检查虚境冷却，而另外两个 `desc` 也包含相同冷却条件，并分别叠加
“月都灵能危机结束”和“Zeta 危机结束”标记。因此多个描述块可能同时命中并输出
相同本地化键。

建议将三个描述条件改为互斥，或在同一个 triggered description 内用
`if / else_if / else` 选择文本。不要仅删除日志输出，否则文本仍可能重复。

### 4.3 `paragon_2.1005` 重复事件 ID

本项目：

```text
events/!!!!_touhou_vanilla_overwrite_events.txt line 402
```

原版 4.5.3：

```text
events/paragon_events_2.txt line 11
```

引擎报告事件 ID 已存在。需要确认这里的“覆盖”是否依赖加载顺序，最好改成不冲突
的 MOD 命名空间并通过 on_action/trigger 转发，或明确采用可维护的覆盖机制。

### 4.4 莉莉白的错误效果名

`events/02_th_events.txt:1798` 调用：

```txt
touhou_effect_init_lilywhite_effect
```

实际定义名是：

```txt
spth_effect_init_lilywhite_effect
```

定义位于 `common/scripted_effects/th_special_leader_effects.txt:811`。
这是明确的拼写错误，日志报 `Invalid scripted effect`。

### 4.5 `th_spawn_explosion_particle` 的 scope 噪声

日志中有 23 条与该效果相关的错误，集中为：

```text
Wrong scope for trigger 'has_ship_flag'
Wrong scope for effect 'set_timed_ship_flag'
Wrong scope for trigger 'is_ship_size'
```

效果位于 `common/scripted_effects/th_battle_effects.txt:116`。它在同一个
`if` 的 limit 中同时写 `is_scope_type = ship` 和 ship-only 条件；脚本条件不应
假定短路求值。调用点又包括 planet、megastructure、system 等非 ship scope：

- `common/special_projects/spth_projects.txt`
- `common/button_effects/spth_button_effects.txt`
- `events/touhou_ship_actions_events.txt`
- `events/02_th_events.txt`

建议外层只判断 scope，例如：

```txt
if = {
    limit = { is_scope_type = ship }
    if = {
        limit = { NOT = { has_ship_flag = ... } }
        set_timed_ship_flag = { ... }
        if = { limit = { is_ship_size = ... } ... }
    }
}
```

非 ship scope 仍应执行通用的 ambient object 创建分支，但不应求值
ship-only trigger/effect。

### 4.6 4.5.3 与 4.6-only 语法并存

当前日志来自 4.5.3，但仍出现大量类似：

```text
Unexpected token: ai_update_type
Unexpected token: inherit_name
Unexpected token: is_capped_by_modifier
Unexpected token: total_fire_time
Unexpected token: condition_string
Unexpected token: min_range
Unexpected token: windup
Unexpected token: tracking
```

统计：

- `persistent.cpp` 结构解析错误：1,277 条
- `reader.cpp` 高级语句/变量解析错误：4,282 条
- `parser_deferred_database_objects.cpp` 延迟引用失败：2,644 条
- 重复事件 ID：105 条
- 缺失本地化键引用：705 条，去重后 364 个键
- `Wrong scope for trigger`：313 条
- `Invalid scope for event target`：91 条
- 除零错误：3 条
- `Undefined event target`：4 条
- 各类 `Invalid scripted effect`：15 条

这并不表示上述全部是新增 4.6 内容；仓库里已有部分 4.6 风格字段，同时加载了
其他兼容 MOD。发布 4.5 内容前，不应仅凭当前 `error.log` 就认为兼容完成。
恢复 4.6 preview 改动后，应使用真实 4.6 preview 再跑一次并单独对比日志。

### 4.7 已知但优先级较低的问题

- `events/02_th_events.txt` 仍有对不存在的 SRA 建筑、科技和市政引用。
- `common/scripted_effects/spth_planet_effects.txt` 中出现不存在的 SRA 建筑。
- `events/lunar_capital_psi_crisis_events.txt` 有损坏 event target 和缺失图片。
- `events/crisis.211` 日志报 `has no valid options`。
- 资源侧有大量缺失音频、纹理、mesh、attach point、粒子时间戳和字体/UI 解析错误。
- `ag_system_selector`、Ancient Empire 舰船、Kuat 相关调用贡献了大量延迟引用失败。

这些应单独建任务清理，避免和灵潜 UI 验证混在一起。

## 5. 建议下一步

优先级从高到低：

1. 重启游戏并用控制台触发 `touhou_shroud.2001`、`.2100`、`.2281`，确认自定义
   UI 和背景房间的实际显示。
2. 修复 `toho.8301` 三个描述条件重叠导致的重复本地化警告。
3. 修复 `touhou_effect_init_lilywhite_effect` 拼写。
4. 重构 `th_spawn_explosion_particle` 的 ship-only scope 判断。
5. 决定 `paragon_2.1005` 的覆盖策略。
6. 发布创意工坊后，执行 preview revert 的反向提交恢复命令。
7. 使用真实 4.6 preview 重新运行，和本次 4.5.3 日志分组对比。

## 6. 其他未完成/需重新确认的事项

此前提出的 `toho.4009` 拆解巨构选项及
`touhou_gateway_network_quantum_catapult` 的清理模块/flag 逻辑，当前工作树中
没有对应改动。不要假定已经完成；继续处理前先重新读取事件和相关 megastructure
定义。

当前 `script_profiling.log` 和 `script_profiling_summary.log` 都是 0 bytes，
本次启动没有可用于战斗性能分析的 profiler 数据。

## 7. 关键文件

- 4.6 preview 对比资料：`.agent/STELLARIS_4.6_PREVIEW_COMPARISON.md`
- 基础项目交接：`.agent/HANDOFF.md`
- 本机路径配置：`.agent/local_config.json`
- 灵潜重写事件：`events/touhou_shroud_events.txt`
- 灵潜入口和分发：`events/02_th_events.txt`
- 灵潜自定义 UI：`interface/spth_special_diplomacy_ui.gui`
- 战斗爆炸效果：`common/scripted_effects/th_battle_effects.txt`
- 莉莉白特殊效果：`common/scripted_effects/th_special_leader_effects.txt`

## 8. 提交前检查

最低限度执行：

```bash
git diff --check
git status --short
```

当前改动尚未提交。不要覆盖 `events/02_th_events.txt` 中已有的三项事件转发修正，
也不要删除两个未跟踪的 automation 文件。
