# 新增 NPC 事件

先确定业务何时触发、关联哪个角色、脚本能读取什么，以及脚本是否能取消当前操作。Pandas 的标准事件走正常派发，Filter 和 Express 走实时派发。Filter 可用 `processhalt` 中断原操作，Express 适用于需要立即执行且原操作继续进行的场景。

## 生成事件登记

在干净的工作树中按[辅助脚本说明](../../../tools/python/README.md)准备 Pipenv，从 `tools/python` 运行 `pipenv run python create_npcevent.py`。按以下顺序填写[生成器](../../../tools/python/create_npcevent.py)的提示。

1. 类型。选 `0` 为 Event，选 `1` 为 Filter，选 `2` 为 Express。
2. 常量后缀。提示已带 `NPCE_`、`NPCF_` 或 `NPCX_`，输入自动大写。生成器由此推导 `Pandas_NpcEvent_*`、`Pandas_NpcFilter_*` 或 `Pandas_NpcExpress_*` 功能宏，以及 `*_event_name`、`*_filter_name` 或 `*_express_name` 字段。
3. 脚本标签。按选定类型填 `On...Event`、`On...Filter` 或 `On...Express`，前缀和后缀区分大小写。
4. 一句简短的触发说明。核对摘要中的常量、宏、标签和字段，输入 `Y` 写入。

[注入器](../../../tools/python/libs/InjectController.py)先检查 `PYHELP - NPCEVENT - INSERT POINT` 插入点是否齐全，包括其他类型的插入点。若报找不到或重复，检查标记和源码的 UTF-8 BOM。用 `git diff` 核对实际登记。

| 文件 | 生成内容 |
| --- | --- |
| [src/config/pandas.hpp](../../../src/config/pandas.hpp) | 按事件类型生成宏和说明 |
| [src/map/npc.hpp](../../../src/map/npc.hpp) | `npce_event` 枚举值 |
| [src/map/npc.cpp](../../../src/map/npc.cpp) | `npc_get_script_event_name()` 中枚举到字段的映射。Filter 和 Express 还分别加入 `npc_event_is_filter()` 和 `npc_event_is_express()` 的名单 |
| [src/map/script.hpp](../../../src/map/script.hpp)、[src/map/script.cpp](../../../src/map/script.cpp) | `Script_Config` 的字段和对应位置的默认标签字符串，检查两处顺序一致 |
| [src/map/script_constants.hpp](../../../src/map/script_constants.hpp) | 事件常量导出 |

## 接入业务并验证

1. 在实际业务入口用同一个功能宏包住触发逻辑。Event 和 Express 调用 `npc_script_event(*sd, 类型常量)`。Filter 调用 `npc_script_filter(sd, 类型常量)`，返回 `true` 时在修改状态前停止原操作并按业务约定返回。Filter 依赖 `Pandas_Struct_Map_Session_Data_EventHalt`，实时派发由 `Pandas_ScriptEngine_Express` 提供，脚本中的 `processhalt` 由 `Pandas_ScriptCommand_ProcessHalt` 启用。
2. 生成器只登记事件。关联对象、触发时机、参数设置和取消分支都在业务入口补齐。对于 `@变量` 等脚本输入，在每次触发前写入本次操作所需的值，并在 [pandas_events.txt](../../pandas_events.txt) 记录触发时机、角色、变量名称与含义以及中断效果。Filter 和 Express 的脚本需遵守 `script.cpp` 中 `run_func` 的实时事件指令限制，例如 `mes`、`menu`、`sleep` 等会暂停执行的指令。
3. 构建 map-server，加载含新标签的最小 NPC 脚本，通过真实操作检查触发次数、角色和参数。Filter 分别验证放行与 `processhalt`，并检查取消后状态和客户端反馈。Express 检查脚本在预期时点执行且原操作继续。再关闭功能宏构建，核对原业务路径仍可编译。

可用 `git show d509fe620e3003ea9ce1162c65781005e86b245a` 查看 `OnPCFavoriteDelFilter` 的完整链路。该实现先在 `clif_parse_MoveItem()` 填入 `@unfavorite_*` 参数，再用 Filter 决定是否修改收藏状态。
