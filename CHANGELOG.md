# 更新日志

此仙境传说模拟器中值得注意的改动都将被记录到本文档.

虽然 Pandas 是基于 rAthena 构建的, 但我们不会刻意在此文档中强调 rAthena 官方的改动, 除非开发者认为 rAthena 的改动值得在此重点提出 (例如遇到兼容性问题时).

本文档遵循 [维护更新日志](https://keepachangelog.com/zh-CN/1.0.0/) 提及的格式标准, 但并不遵循 [语义化版本](https://semver.org/lang/zh-CN/) 版本号制定标准.

# 注意事项

若您运行本程序时遇到提示丢失 `VCRUNTIME140.dll` 等文件导致无法启动时, 请下载安装 [Microsoft Visual C++ 2015 Redistributable](https://www.microsoft.com/zh-CN/download/details.aspx?id=52685) 的 x86 版本后重试.

## [v1.0.0] - `2019-10-07`

### 添加

- 能够读取 `UTF8-BOM` 编码的 db, npc, conf 文件 (0a0606c)
- 实现护身符类型的道具, 只要道具在身上就能持续发挥效果 (#112)
- 实现魔物道具固定掉率数据库, 可用于设置道具的全局固定掉落概率 (#119)
- 使 `pointshop` 类型的商店能支持指定变量别名, 改善玩家的游戏体验 (#126)
- 使用 `Google Breakpad` 捕捉程序的崩溃转储文件 (#150)
- 能够检测 `import` 目录是否存在, 若不存在能自动复制一份出来 (#173)
- 重新梳理数据库连接配置并重写编码自动判定策略 (#174)
- 能够在 `map_athena.conf` 中设置封包混淆密钥 (a8d9e84)
- 能够在 `login_athena.conf` 中设置隐藏角色服务器的在线人数 (9291f57)
- 能够在 `char_athena.conf` 中设置禁止创建杜兰族角色 (30bfe00)
- 实现或拓展共计  1 个 GM 指令, 详见 `doc/pandas_atcommands.txt` 指令文档
- 实现或拓展共计 40 个脚本指令, 详见 `doc/pandas_script_commands.txt` 指令文档
- 实现或拓展共计 12 个地图标记, 详见 `doc/pandas_mapflags.txt` 说明文档
- 实现或拓展共计 15 个脚本事件, 详见 `doc/pandas_events.txt` 说明文档

### 调整

- 使影子装备可以支持插卡, 而不会因插卡而被强制转换成普通道具 (#64)
- 在使用 _M/_F 注册的时候, 能够限制使用中文等字符作为游戏账号 (09068b8)

### 修正

- 修正读取 `exp_homun.txt` 时提示信息不正确的问题 (#17)
- 修正部分简体、繁体中文字符作为角色名时, 会被变成问号的问题 (#50)
- 修正 `item_trade` 中限制物品掉落后, 权限足够的 GM 也无法绕过限制的问题 (#54)
- 修正使用 `sommon` 脚本指令召唤不存在的魔物, 会导致地图服务器崩溃的问题 (#65)
- 修正给予 `instance_create` 无效的副本名称会导致地图服务器崩溃的问题 (#113)
- 修正 `reloadnpc` 时文件路径前后有空格所带来的不良影响 (#139)
- 修正使用 `pointshop` 操作 `#CASHPOINTS` 变量时可能导致的双花攻击的问题 (#138)
- 修正多层脚本调用导致的程序崩溃问题 (#163)
- 修正部分情况下 `getd` 脚本指令会导致地图服务器崩溃的问题 (#175)
- 修正在部分情况下角色公会图标刷新不及时的问题 (663b9d4)

[v1.0.0]: https://github.com/PandasWS/Pandas/releases/tag/v1.0.0
