# Pandas 开发文档

本目录帮助维护者和 Agent 理解 Pandas 的开发约定，并按仓库现有方式完成任务。根入口为 [AGENTS.md](../../AGENTS.md)。

## 从任务开始

| 需要做什么 | 阅读入口 |
| --- | --- |
| 写提交信息 | [提交规范](standards/commits.md) |
| 修改自有功能、安排宏和数据覆盖 | [开发规范](standards/development.md) |
| 同步上游并检查本地改动是否丢失 | [上游同步规范](standards/upstream-sync.md) |
| 理解 Pandas 相对 rAthena 的主要结构差异 | [架构概览](architecture/overview.md) |
| 理解编译、符号、语言包和 SQL 发布目录 | [发布与数据库整理](architecture/release-pipeline.md) |
| 添加脚本指令、管理员指令、地图标记等 | [How-to](how-to/README.md) |

## 目录职责

- `standards/`：开发过程中必须遵循的约定。
- `architecture/`：已有实现的结构、调用关系及设计背景。
- `how-to/`：按主题组织的操作步骤，包含入口、联动文件、验证方法和历史样例。

现有 `doc/pandas_*.txt` 等文件继续承担用户功能说明。开发文档引用这些说明，避免复制指令清单。

架构文档最初核对的 Pandas 基点是 `e6c34462cbf30f291048ffe7ae7cec7dc0e7f75d`，对应已合入的 rAthena 基线为 `67431ee25c641a1064b07365f8a916cd26678fca`。后续变更应同步更新受影响的事实；历史基点用于追溯，不限定后续开发版本。

## 发布边界

本目录及根 `AGENTS.md` 不进入 Dawn 发布脚本生成的最终运行 ZIP；源码仓库与普通 Git 源码归档仍保留它们。排除发生在 `arrange_common()` 的发行覆盖文件合入之后，详见 [发布流程](architecture/release-pipeline.md)。
