# 新增战斗配置

先确定可配置的行为、默认行为、合法范围和单位。Pandas 的战斗配置也用于控制脚本等其他 map-server 行为。配置项需要同时连到配置表和实际使用点。

## 生成配置登记

在干净的工作树中按[辅助脚本说明](../../../tools/python/README.md)准备 Pipenv，从 `tools/python` 运行 `pipenv run python create_battlecfg.py`。按以下顺序填写[生成器](../../../tools/python/create_battlecfg.py)的提示。

1. `Pandas_BattleConfig_` 的后缀。提示已显示前缀。
2. 配置键，例如 `my_feature_limit`，以及 C++ 字段名。字段名留空时使用配置键，键里的 `.` 会换成 `_`。检查字段名是合法 C++ 标识符。
3. 默认值、最小值、最大值。前两项留空为 `0`，最大值留空或输入 `0` 时会生成 `INT_MAX`。输入助手只接受数字，生成器要求最大值大于最小值。默认值范围、负数以及特殊边界需要在生成后按实际语义校正。布尔值可输入 `0` 或 `1`，范围填 `0`–`1`。
4. 核对摘要，输入 `Y` 写入。

[注入器](../../../tools/python/libs/InjectController.py)按 `PYHELP - BATTLECONFIG - INSERT POINT` 定位写入。缺失或重复时检查标记和源码的 UTF-8 BOM。用 `git diff` 核对四处。

| 文件 | 生成内容 |
| --- | --- |
| [src/config/pandas.hpp](../../../src/config/pandas.hpp) | `Pandas_BattleConfigure` 组下的功能宏及占位注释 |
| [src/map/battle.hpp](../../../src/map/battle.hpp) | `Battle_Config` 中受功能宏保护的 `int` 字段 |
| [src/map/battle.cpp](../../../src/map/battle.cpp) | `battle_data` 中配置键到字段的绑定，以及默认值和上下界 |
| [conf/battle/pandas.conf](../../../conf/battle/pandas.conf) | 配置项、默认值和占位说明 |

## 接入行为并验证

1. 在实际行为入口读取 `battle_config.<字段>`，与字段定义使用同一个功能宏。功能宏关闭时保留适用的原行为。若值有单位换算或特殊含义，明确在哪一层处理。
2. 补全 `pandas.conf` 的作用、单位、范围和默认值说明，并保持文件默认值与 `battle_data` 一致。布尔项可按邻近配置写成 `yes` 或 `no`。[battle_athena.conf](../../../conf/battle_athena.conf)已加载 `pandas.conf`，随后加载 `conf/import/battle_conf.txt`。测试环境可在后者覆写。若功能宏关闭而配置文件还带有该键，加载时会走未知配置项分支，验证禁用场景时一并检查。
3. 构建 map-server，检查缺省值、本地覆写和上下界。越界时 `battle_set_value()` 会记录警告并回退到表内默认值。通过实际业务入口验证配置效果，再关闭功能宏构建，检查原有路径。

可用 `git show 8201b8b0b0bfe5f805428b278cdeafe41a2a71ce` 查看 `strict_parameters_of_rand`。它把配置限制在 `0`–`1`，默认开启，并在 `rand` 的实际参数处理处读取该值。
