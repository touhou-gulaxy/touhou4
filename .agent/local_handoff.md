# 本地开发交接

最后更新：2026-11-15 Asia/Shanghai

本文只记录当前设备和当前工作树附近的状态。开始前执行：

```bash
git status --short --branch
git log -5 --oneline --decorate
```

## 1. Git 与版本

- 工作目录：`/home/koishi/IdeaProjects/touhou4`
- 分支：`dev/4.6`
- 当前基线：`f5e08448 update version number`
- 当前工作树：约 146 个已修改或未跟踪状态条目，包含大量新增 DDS
- 上游：`origin/dev/4.6`
- 当前 MOD 版本：`2.5.3`
- `descriptor.mod`、`spth_version`、`spth_version_date` 和更新日志已同步

当前工作树包含尚未分配版本号的下一轮功能，不能把 `HEAD` 的 2.5.3 发布日志
视为已经包含这些改动。实际状态以 `git status` 为准。

当前未提交功能主要包括：

- 四阶段幻想星环方舟巨构及完工世界舰
- 标准方舟 40 个普通挂载和 7 个轴基挂载
- 无灵力方舟 21 个普通挂载和 2 个轴基挂载
- 无灵力武器倍率、弱化命中效果、图标和太阳反应堆
- 方舟/月都/战斗神社/夜雀食堂/突触凝练舰数值调整
- 东方国家认证通用神隐星门权限
- 新增舰船、组件、巨构的本地化与合成 DDS 资源

## 2. 当前设备路径

主要路径记录在 [local_config.json](local_config.json)：

| 用途 | 当前路径 |
|---|---|
| 游戏安装 | `/home/koishi/SteamLibrary/steamapps/common/Stellaris/` |
| 4.5 反编译源码 | `/home/koishi/ghidra_decompiled/stellaris4.5/` |
| 游戏日志 | `/home/koishi/.local/share/Paradox Interactive/Stellaris/logs/` |
| 注入式性能报告 | `/home/koishi/ghidra_decompiled/stellaris4.5/perf/pdxperf-report.txt` |
| 配套资源 | `/home/koishi/IdeaProjects/touhou_resources/` |
| Workshop 内容根目录 | `/home/koishi/SteamLibrary/steamapps/workshop/content/281990/` |

注入式性能报告可能较旧；使用前检查文件时间和对应游戏版本。

## 3. 最近一次游戏加载

- 游戏版本：Cygnus `v4.5.4`
- `game.log` 启动时间：2026-11-15 18:47:09
- `error.log` 最后写入：2026-11-15 18:51:24
- 本次启动未生成新的 crash 目录
- `error.log` 未出现 `touhou_non_genso_parts.txt`、新增方舟挂载、
  `spth_component_attributes_non_genso` 或对应 GFX 的路径错误
- 本次加载早于最后的无灵力方舟挂载文件加入，因此这些挂载仍需实测

启动无解析错误不等于功能已实测。以下功能仍需游戏内逐项验证：

- 幻想星环方舟四个阶段、第二个轴基槽、完工舰生成与殖民地功能
- 标准及无灵力方舟挂载是否在正确 section 出现、可安装并显示合成图标
- 无灵力武器 `on_hit`、统一倍率变量、无灵力太阳反应堆和隙间引擎
- 方舟、战斗神社、夜雀食堂和突触凝练舰的新容量/生存数值
- 东方国家认证使用神隐星门的权限
- `hmx_hellsing` 的启动、120 天到期、全舰坞达标终止和 360 天冷却
- 自定义科研/工程舰船执行迁移后的特殊项目
- 统治者领袖手动复活
- `touhou_shroud.2xxx` 自定义聊天 UI
- 大规模舰队选择性能的实际改进

## 4. 当前日志中的项目相关问题

很多错误来自并存的大型 MOD，不能全部归因于本仓库。以下是当前最值得继续处理的
项目路径。

### 4.1 `touhou_ship_combat_power` 图标

当前隐藏资源的占位实现会产生：

```text
Did not find an icon for resource: touhou_ship_combat_power
Did not find a greyscale icon for resource: touhou_ship_combat_power
Missing modifier icon "gfx/interface/icons/modifiers/mod_resource_touhou_ship_combat_power_mult.dds"
Missing modifier icon "gfx/interface/icons/modifiers/mod_touhou_ship_combat_power_*_*.dds"
```

资源项位于：

- `common/strategic_resources/touhou_advanced_strategic_resources.txt`
- `common/inline_scripts/ship_sizes/touhou_optimize_evaluation_resource.txt`
- `common/inline_scripts/ship_sizes/touhou_optimize_evaluation_resource_upkeeps.txt`

需要判断这些图标是否只影响日志。隐藏资源如果仍参与资源表工具提示，可能应提供
占位图标，而不是长期保留 missing texture 警告。

### 4.2 `toho.8301` 重复 triggered localization

当前仍报告：

```text
toho.8301.desc.14
toho.8301.desc.16
toho.8301.desc.17
toho.8301.desc.18
toho.8301.desc.20
toho.8301.desc.21
toho.8301.desc.22
```

第一个 desc 已经正确排除“月都灵能危机结束或 AG Zeta 危机结束”。
剩余问题位于第二个和第三个 desc：当两个结束旗标同时存在时，两者会同时命中。
应把这两个 desc 改为互斥，或在同一个 desc 内根据两个旗标选择文本。

### 4.3 `th_spawn_explosion_particle` scope 警告

当前仍有：

```text
Wrong scope for trigger 'has_ship_flag'
Wrong scope for effect 'set_timed_ship_flag'
Wrong scope for trigger 'is_ship_size'
Current Scope: ...
Supported Scopes: ship
```

调用点包括特殊项目、按钮 effect、事件和危机 effect。脚本已经在最外层增加
`is_scope_type = ship`，但引擎静态检查仍会报告内层 ship-only 字段。不要简单
把警告当作已修复；需要确认调用点实际 scope，并把 ship-only 部分放进明确的
ship scope。

### 4.4 `th_ship.txt` 的 ship growth stage

`touhou_optimize_evaluation_resource_upkeeps.txt` 在部分 ship size 中被
`ship_growth_stage` scope 解析，日志出现：

```text
Invalid scope for event target 'owner'
Got ship_growth_stage
```

需要确认这些 ship size 是否真的会进入 growth stage，以及 `exists = owner`
在该静态资源表求值阶段是否可靠。不要仅因为舰队 UI 已加速就忽略。

### 4.5 AG 隐藏修正重名

`ship_is_constructor`、`ship_is_science_ship`、`ship_is_colonizer`、
`ship_is_transport` 与 AG Mod 同名并报告重复定义。当前定义内容相同，但存在
后续漂移风险。若修改其定义，必须同时检查 AG 版本和加载顺序。

### 4.6 Paragon 事件 ID 冲突

当前日志仍报告：

```text
paragon_2.1005 already exists
paragon_2.1020 already exists
paragon_2.1021 already exists
paragon_2.2000 already exists
paragon.403/404/405/549 already exists
```

其中本项目覆盖位于
`events/!!!!_touhou_vanilla_overwrite_events.txt`。这些 ID 与当前加载的原版或
其他 MOD 事件冲突，不能仅依赖 `!!!!` 文件名排序。发布前应确认覆盖是否仍符合
当前游戏版本，优先改为 MOD 命名空间并通过 on_action 或 trigger 转发。

## 5. 4.6 preview 状态

此前 `5c78ae2d` 曾临时 revert 4.6 覆盖兼容，但随后 `16e488f4` 已重新应用，
当前代码以重新应用后的版本为准。不要在后续发布准备中再次自动 revert。

4.6 preview 的静态对比、伦理变化、on_action 改名和巨构模块资料见：

- [STELLARIS_4.6_PREVIEW_COMPARISON.md](STELLARIS_4.6_PREVIEW_COMPARISON.md)

当前稳定发布目标仍是 `4.5.*`。4.6 兼容层属于内部前瞻内容，发布前应确认目标
版本和创意工坊版本描述。

## 6. 本地化同步

工具：

```text
utils/loc/sync_english_localisation.sh
```

用法：

```bash
utils/loc/sync_english_localisation.sh --dry-run
utils/loc/sync_english_localisation.sh
```

规则：

- 源目录：`localisation/english/`
- 目标：除 `simp_chinese` 外的八种语言
- 排除：
  `localisation/russian/replace/spth_vanilla_repair_l_russian.yml`
- 正文同步英文，仅把首行语言头改成目标语言
- 工具是幂等的，重复执行不会重写相同文件

该工具会覆盖目标语言现有正文，这是当前发布策略；运行前先做 `--dry-run`，并确认
没有需要保留的独立翻译。

## 7. 常用检查命令

```bash
git diff --check
rg -n "Unknown inline_script|Wrong scope|Invalid scope|Cannot divide by zero" \
  "/home/koishi/.local/share/Paradox Interactive/Stellaris/logs/error.log"
rg -n "common/special_projects|common/inline_scripts/special_projects|events/th_hellsing|common/inline_scripts/ship_sizes/touhou" \
  "/home/koishi/.local/share/Paradox Interactive/Stellaris/logs/error.log"
```

检查性能日志前先复制，因为游戏重启可能截断文件：

```bash
cp script_profiling_summary.log /tmp/script_profiling_summary.before.log
```

## 8. 当前未提交/未验证事项

- 本文的修改和 `.agent/skills/` 下的新增项目技能尚未提交
- 当前工作树的大批新功能、GFX、DDS 和本地化均尚未提交
- 四阶段幻想星环方舟及其完工世界舰尚未完成完整运行测试
- 标准 47 个、无灵力 23 个方舟挂载尚未在游戏内逐槽验证
- 无灵力武器弱化 `on_hit` 与无灵力太阳反应堆尚未完成实战验证
- 方舟/月都/恒星基地的新容量和生存数值尚未完成平衡验证
- `toho.8301` 第二、第三个 desc 的重叠尚未修复
- 隐藏资源 `touhou_ship_combat_power` 的资源和 modifier 图标缺失
- `th_spawn_explosion_particle` 的静态 scope 警告仍在
- ship growth stage 下的 owner scope 警告仍在
- Paragon 覆写事件 ID 与其他内容冲突
- Hellsing 设计目标曾讨论 90 天，但当前代码仍为 120 天
- 2.5.3 新功能尚未全部完成游戏内实测
