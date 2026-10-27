# 东方繁星录 ~ Symphony of Phantasm

这是一个面向群星 4.5 的大型玩法 MOD，核心内容包括东方 Project 物种、领袖、
事件链、舰船与武器、灵力经济体系以及天灾内容。

本仓库保存 MOD 源文件，不需要单独编译。游戏会直接读取其中的
`.txt`、`.yml`、`.gfx`、`.asset` 和 `.gui` 文件。

## 环境要求

- 群星 `v4.5.*`
- 对应玩法文件所需的其他 MOD 依赖
- 建议安装 CWTools 与 Stellaris Language 编辑器插件

当前 MOD 元数据见 [`descriptor.mod`](descriptor.mod)。

## 目录说明

- `common/`：组件、舰船、科技、规则、修正、脚本效果等定义
- `events/`：事件链、舰队系统、天灾和动态战斗逻辑
- `localisation/`：主仓库中的本地化文件
- `gfx/`、`interface/`、`sound/`、`music/`：图像、界面、声音和音乐资源
- `utils/`：代码生成器与维护脚本
- `backup/`：本地备份和参考内容，不应视为活动 MOD 数据

## 配套资源

素材、音乐、创意工坊描述和未完成的英文本地化通常位于相邻目录：

```text
../touhou_resources/
```

创意工坊描述文件约定如下：

- `desc.txt`：英文创意工坊物品信息
- `desc_cn.txt`：中文创意工坊物品信息
- 第一行：创意工坊物品名称
- 其后的空行之后：完整的创意工坊物品描述

当前资源目录中的中文文件实际名为 `desc_zh.txt`，而文档约定的规范名称是
`desc_cn.txt`。

`../touhou_resources/utf8/` 保存部分翻译的英文本地化。翻译时必须保留
`$变量$`、`§Y...§!`、本地化命令和换行等占位与格式标记。

更多工程约定、性能分析入口和已知问题见
[.agent/HANDOFF.md](.agent/HANDOFF.md)。

## 开发与验证

常用静态检查：

```bash
git diff --check
```

随后运行 `.vscode/` 中配置的 CWTools 检查，并进入游戏验证。错误日志位于：

```text
~/.local/share/Paradox Interactive/Stellaris/logs/error.log
```

本仓库没有完整的自动测试套件。事件 scope、修正递归、本地化占位符和战斗性能
都需要在实际存档中验证。

## 工具脚本

例如：

```bash
./utils/gen_component_vars.sh --dry-run
./utils/ships/gen_ags_psionic_variants.sh --dry-run
```

生成文件在提交前应先检查差异。

## 性能分析

常用外部路径：

- 游戏安装目录：
  `/home/koishi/SteamLibrary/steamapps/common/Stellaris/`
- 4.5 反编译源码：
  `/home/koishi/ghidra_decompiled/stellaris4.5/`
- 注入式性能报告：
  `/home/koishi/ghidra_decompiled/stellaris4.5/perf/pdxperf-report.txt`

`script_profiler` 适合分析事件、触发器和脚本值热点，但不是完整 C++ 性能分析器。
武器 `on_hit`、scope 构造与深拷贝、内存分配必须结合 `pdxperf`、Linux `perf`
或控制变量 A/B 测试判断。

