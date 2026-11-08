# 新增地图标记

先确定地图标记是布尔开关，还是需要一至四个整数参数。列出受影响的单位、操作入口和关闭时的行为。已有标记可用 `rg -n 'MF_名称|Pandas_MapFlag_名称' src doc` 追踪登记和实际读取位置。

## 生成登记代码

在干净的工作树中按[辅助脚本说明](../../../tools/python/README.md)准备 Pipenv，从 `tools/python` 运行 `pipenv run python create_mapflag.py`。按以下顺序填写[生成器](../../../tools/python/create_mapflag.py)的提示。

1. `Pandas_MapFlag_` 的后缀，例如 `MyLimit`。这个后缀也会原样填入配置中的显示名称，生成后检查大小写和用户侧拼写。
2. `MF_` 的后缀，例如 `MYLIMIT`。输入会自动转成大写。两个提示都已带前缀。
3. 类型。选 `0` 生成开关式，选 `1` 生成带数值参数的标记。
4. 选 `1` 时再填参数数量（`1`–`4`），以及全部参数为默认值时是否关闭、是否禁止通过 `@mapflag` 调整。两项布尔选择都默认关闭。
5. 检查摘要，输入 `Y` 写入。

[注入器](../../../tools/python/libs/InjectController.py)按 `PYHELP - MAPFLAG - INSERT POINT` 定位四处插入点。缺失或重复时先检查标记和源码的 UTF-8 BOM。生成后用 `git diff` 逐处检查。

| 文件 | 生成内容 |
| --- | --- |
| [src/config/pandas.hpp](../../../src/config/pandas.hpp) | `Pandas_Mapflags` 组下的功能宏及占位注释 |
| [src/map/map.hpp](../../../src/map/map.hpp) | `e_mapflag` 中受功能宏保护的 `MF_*` |
| [src/map/script_constants.hpp](../../../src/map/script_constants.hpp) | `export_constant(MF_*)` |
| [src/map/map.cpp](../../../src/map/map.cpp) | `map_flags_init()` 中的 `s_mapflag_item`，含名称、开关策略和参数列表 |

## 接入功能并验证

1. 补全宏的用途说明。给每个参数写清默认值、上下界和单位，替换生成的 `{0, 0, INT_MAX}` 占位值。用 `map_getmapflag()` 判断状态，用 `map_getmapflag_param()` 读取数值。当前参数编号从 `1` 开始，编号 `0` 读取的是标记状态。
2. 在实际处理操作的入口读取标记，并为新引用加上与登记一致的功能宏。若存在多种单位、客户端请求和脚本入口，按该标记定义的作用范围逐条接入。生成器只登记标记。按需补充客户端提示和[消息配置](../../../conf/msg_conf)，并在 [pandas_mapflags.txt](../../pandas_mapflags.txt) 写明地图脚本写法、参数顺序及单位。
3. 构建 map-server，在测试地图用 `<地图><Tab>mapflag<Tab><名称><Tab><参数>` 加载标记。带多个数值时在最后一列用逗号分隔。用 `@mapinfo` 查看结果，按需试 `@mapflag` 或 `setmapflag`。验证正常操作、受限制操作、参数缺省和边界、解除标记后的行为。再关闭功能宏构建，检查对枚举和参数的引用都受保护。

可用 `git show 2fa02ee503b3bd285904113312ec1162613aea28` 查看 `noskill2` 的单位掩码如何接入多种技能入口和用户文档。现行多参数登记和索引用法以 `git show 1c48436f7d3451f624b16bf3c2113b5618a1a50a` 之后的源码为准。
