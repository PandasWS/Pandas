# Pandas 仓库协作规则

Pandas 是基于 rAthena 的中文 MMORPG 模拟器。本文件是仓库内 Agent 的规则入口，适用于整个仓库。开发文档集中在 [doc/development/](doc/development/README.md)。

## 开始工作

- 默认用简体中文交流、撰写新增说明和注释，保留已有 API、类型、命令与技术术语。
- 先检查当前分支、工作区状态及任务范围，保留已有修改。功能分支通常从 `develop` 建立；具体操作遵循用户当次授权。
- 先读 [开发规范](doc/development/standards/development.md)，按任务阅读下表对应文档。实现与文档不一致时先查源码和历史，修正文档中已经过时的事实。

| 任务 | 必读文档 |
| --- | --- |
| 编写提交信息 | [提交规范](doc/development/standards/commits.md) |
| 编写更新日志、整理 Release 正文 | [更新日志规范](doc/development/standards/changelog.md) |
| 合并、审查 rAthena 更新 | [上游同步规范](doc/development/standards/upstream-sync.md) |
| 了解仓库结构及 Pandas 差异 | [架构概览](doc/development/architecture/overview.md) |
| 修改发布、打包、SQL 目录整理 | [发布与数据库整理](doc/development/architecture/release-pipeline.md) |
| 添加指令、地图标记、事件或配置，维护升级 SQL | [How-to 导航](doc/development/how-to/README.md) |

## 核心约定

- Pandas 自有 C/C++ 功能差异应使用恰当的 `Pandas_*` 宏标识和控制，在 `src/config/pandas.hpp` 对应分组声明并说明用途、依赖。优先复用同一功能已有的宏；同时检查启用和关闭路径。具体边界见开发规范。
- 同步上游时保留 Pandas 已有行为及修复，沿入口、注册、实现、数据和反馈检查完整链路。上游新发现的问题按任务范围处理，不顺带扩大同步修复。
- 新写的提交信息遵循 Conventional Commits；正式版发布、下一轮开发升版和 rAthena 同步的汇总合并提交沿用专用标题格式。第一行标题末尾禁止使用英文句号 `.` 或中文句号 `。`。完整格式与例外见 [提交规范](doc/development/standards/commits.md)。
- 未经用户明确授权，不执行 commit、push、merge 或 release；这些操作分别确认授权范围。不得绕过 hook 或关闭提交签名来完成提交。
- `develop` 和 `master` 允许直接快进更新，禁止非快进改写和删除；PR 按协作需要使用。版本号、更新日志及下一轮升版可直接在 `develop` 提交，发布时用 `--ff-only` 将 `master` 推进到选定的正式提交。采用 PR 时，普通改动默认使用 Rebase and merge；`merge_rathena` 同步保留 merge commit，并遵循 [上游同步规范](doc/development/standards/upstream-sync.md) 的标题格式。具体步骤见 [开发规范](doc/development/standards/development.md#功能分支与-pr)。
- 本项目是开源项目。与本机或特定环境强绑定的配置、路径、主机地址、账号及操作记录应保存在仓库外，不写入或提交到源码、文档、注释和示例中；可复用的说明使用通用配置与占位符，提交前检查是否夹带环境信息或凭据。
- 遵循 `.editorconfig` 并保留既有文件编码和换行风格；不得破坏 `PYHELP` 注入标记。生成器产物须审查并补全业务行为。
- 交付前审查 diff，执行与变更相称的检查，报告实际结果和未完成验证。修复行为时优先补充可复现的行为验证，避免只测试实现的副本。

## 文档与发布边界

`doc/` 下现有说明面向服主和脚本作者；`doc/development/` 保存开发规范、架构认知及任务指南。改变这些约定或关键流程时，同步维护对应文档和入口链接。

根 `AGENTS.md` 和 `doc/development/` 保留在源码中，由 `tools/python/dawn_publish.py` 的 `arrange_common()` 从最终运行包中移除。维护发布规则时保留这一边界以及原有用户文档。
