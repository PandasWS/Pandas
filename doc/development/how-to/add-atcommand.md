# 添加 Pandas 管理员指令

## 确定指令入口

先查 [实现和注册表](../../../src/map/atcommand.cpp)、[帮助与别名](../../../conf/atcommands.yml) 和 [Pandas 管理员指令文档](../../pandas_atcommands.txt)。定下公开名称、参数、权限和操作目标。默认前缀下，`@指令` 的目标是发令角色，`#指令 <角色名> [参数]` 由分发器解析目标角色。根据需求明确是否允许这两种入口，以及是否允许脚本或控制台调用。

## 生成并检查脚手架

按 [Python 辅助脚本说明](../../../tools/python/README.md) 准备依赖。以下命令从仓库根目录进入脚本目录并启动交互助手。

```sh
cd tools/python
pipenv run python create_atcmd.py
```

[助手](../../../tools/python/create_atcmd.py) 依次询问宏名后缀、`ACMD_FUNC` 中的处理函数名、公开名称是否与处理函数相同。宏前缀 `Pandas_AtCommand_` 由助手补上，两个名称会转成小写。名称相同可以直接回车，选 `N` 后再输入公开名称。输入名称时省去 `@` 或 `#`，参数由处理函数解析。最后核对摘要，输入 `Y` 才会写入。

社区版应产生三处修改。检查 diff 并保留原来的 `PYHELP - ATCMD - INSERT POINT` 标记和源码的 UTF-8 BOM。

1. [`pandas.hpp`](../../../src/config/pandas.hpp) 的 Section 1 在 `Pandas_AtCommands` 组内增加宏。补写用途及必要的宏依赖。
2. `atcommand.cpp` 的 Section 2 增加受宏保护的 `ACMD_FUNC`。替换 TODO 和默认的 `return 0`。
3. `atcommand.cpp` 的 Section 3 用同一宏保护注册项。同名用 `ACMD_DEF(handler)`。当前助手生成的异名项在错误的参数上加了引号，须按源码宏定义改为 `ACMD_DEF2("公开名称", handler)`，第二项填写处理函数标识符。仅增加已有指令的普通别名时，可沿用 `conf/atcommands.yml` 的 `Aliases`。需要限制脚本或控制台入口时使用 `ACMD_DEFR`、`ACMD_DEF2R` 的相应限制位：`ATCMD_NOSCRIPT` 覆盖 `atcommand`、`useatcmd` 两种脚本来源，`ATCMD_NOCONSOLE` 覆盖控制台来源。

## 实现、配置和验证

1. 参照 `is_atcommand` 核对入口。分发器把执行者连接的 `fd`、目标的 `sd`、带前缀的 `command` 和参数串 `message` 交给处理函数。`command` 保留实际输入的别名；共用处理器若按名称分支，要同时覆盖这些别名。按实际语法检查缺参、非法数值、范围和目标状态，复用业务接口执行操作。用 `clif_displaymessage` 等既有接口反馈，按需在 [`conf/msg_conf/`](../../../conf/msg_conf/) 中维护同一编号的基础、简体和繁体消息。成功返回 0，非零返回会触发通用失败反馈。
2. 在 [`conf/atcommands.yml`](../../../conf/atcommands.yml) 添加适用的 `Help` 和别名，在 [`pandas_atcommands.txt`](../../pandas_atcommands.txt) 写明语法、目标和副作用。玩家权限来自 [`conf/groups.yml`](../../../conf/groups.yml) 的 `Commands`、`CharCommands` 及权限继承。检查 `all_commands` 的影响，仅在功能确实需要为特定组默认授权时修改仓库组配置。测试服可以通过 `conf/import/groups.yml` 设置最小权限。`is_atcommand` 中这组玩家权限检查针对 `type == 1`；脚本来源为 `0` 或 `3`，控制台为 `2`，需通过对应的来源限制及业务检查控制敏感操作。
3. 编译 `map-server`，在测试服用允许和拒绝该指令的账号走游戏内入口，检查正常操作、空或无效参数、目标不存在、反馈和有需要时的命令日志。涉及 `#`、别名、脚本、控制台或额外限制时分别验证。现有 [NPC/DB 工作流](../../../.github/workflows/npc_db_validation.yml) 使用 `map-server --run-once`，游戏内权限和交互需在测试服覆盖。

历史参考可在本地运行 `git show 86d055747b`。该提交把 `reloadbarterdb` 注册到已有的 `reload` 处理函数，并同时维护帮助、消息、操作文档和实际重载接口。
