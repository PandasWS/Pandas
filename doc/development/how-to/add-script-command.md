# 添加 Pandas 脚本指令

## 确定脚本接口

先查 [脚本指令实现和注册表](../../../src/map/script.cpp)、[上游指令文档](../../script_commands.txt) 和 [Pandas 指令文档](../../pandas_script_commands.txt)，确认名称、功能和目标对象。定下必填参数、可选参数及默认值、调用时是否需要已绑定的角色（RID），以及成功、无结果、失败各自的脚本返回值。需要写变量时，明确是角色变量、脚本局部变量还是全服共享变量，以及结果的有效范围。

## 生成并检查脚手架

按 [Python 辅助脚本说明](../../../tools/python/README.md) 准备依赖。以下命令从仓库根目录进入脚本目录并启动交互助手。

```sh
cd tools/python
pipenv run python create_scriptcmd.py
```

[助手](../../../tools/python/create_scriptcmd.py) 依次询问宏名后缀、`BUILDIN_FUNC` 中的处理函数名、公开名称是否与处理函数相同、参数模式。宏前缀 `Pandas_ScriptCommand_` 由助手补上。名称相同可以直接回车，选 `N` 后再输入公开名称。处理函数名和公开名称会转成小写。参数模式输入连续字符，例如 `v??`，无参数直接回车。最后核对摘要，输入 `Y` 才会写入。

社区版应产生三处修改。检查 diff 并保留原来的 `PYHELP - SCRIPTCMD - INSERT POINT` 标记和源码的 UTF-8 BOM。

1. [`pandas.hpp`](../../../src/config/pandas.hpp) 的 Section 1 在 `Pandas_ScriptCommands` 组内增加宏。补写用途，有前置功能时在这里表达依赖。
2. `script.cpp` 的 Section 2 增加受宏保护的 `BUILDIN_FUNC`。替换 TODO 和默认的 `SCRIPT_CMD_SUCCESS`。
3. `script.cpp` 的 Section 3 用同一宏保护注册项。同名用 `BUILDIN_DEF(handler, "参数模式")`，不同名用 `BUILDIN_DEF2(handler, "公开名称", "参数模式")`。核对名称冲突和函数是否指向预期实现。

## 实现和验证

1. 按 `script.cpp` 的 `parse_callfunc` 和 `script_check_buildin_argtype` 核对参数模式。`i`、`s`、`v`、`r`、`l` 分别表示数值、字符串、通用值、变量引用和标签；`?` 表示一个可选参数，末尾的 `*` 允许额外的可选参数。处理函数从索引 2 读取第一个参数，用 `script_hasdata` 判断可选值是否存在，并在读取或修改数据前校验类型、范围、角色及目标是否有效。需要当前角色时沿用 `script_rid2sd`；允许显式指定角色时参考现有目标解析助手。运行时类型告警受 `script_config.warn_func_mismatch_argtypes` 控制，检查遇到 `?` 或 `*` 就停止；可选参数类型及业务前置条件由处理函数保证。
2. 复用现有业务接口。用 `script_pushint` 等接口提供约定的脚本返回值，另按执行状态返回 `SCRIPT_CMD_SUCCESS` 或 `SCRIPT_CMD_FAILURE`。写出参数时校验变量引用、类型和作用域。补充 [`pandas_script_commands.txt`](../../pandas_script_commands.txt) 的签名、示例、返回值和副作用。
3. 编译 `map-server`，用实际 NPC 脚本调用指令，检查正常参数、缺省和无效参数、缺少角色或目标，以及可观察的返回值和副作用。能在初始化时稳定断言的回归可参照 [`npc/test/ci/`](../../../npc/test/ci/) 的脚本。现有 [NPC/DB 工作流](../../../.github/workflows/npc_db_validation.yml) 在 Renewal 和 Pre-Renewal 加载测试 NPC 并执行 `map-server --run-once`。需要真实玩家或交互触发的路径在测试服验证。涉及宏依赖时也检查关闭开关后的编译与注册。

历史参考可在本地运行 `git show db296a39dc`。该提交添加 `whodropitem` 的宏、处理函数、`v??` 注册和调用文档，展示如何同时维护可选角色、结果变量和返回值。
