dbghelp
==============

用于生成崩溃转储文件的 Windows 模块.
为了避免新版本的 dbghelp.dll 依赖更多的其他文件(导入表), 这里选用的是 2010 年的文件.

提取来源已经忘记了, 应该是某个版本的 Windbg 目录提取的.
现在 Windbg 都必须先下载 Windows SDK, 所以未来如果再次提取的话可能会麻烦一些.

修改说明
---------------

默认的 dbghelp.h 在 Visual Studio 2017 中编译时会出现两个警告信息:

warning C4091: “typedef ”: 没有声明变量时忽略“<unnamed-enum-hdBase>”的左侧
warning C4091: “typedef ”: 没有声明变量时忽略“<unnamed-enum-sfImage>”的左侧

为了解决这两个警告, Pandas 选择修改 dbghelp.h 来解决问题.
具体变更请打开 `include\dbghelp.h` 搜索 `C4091` 即可看到相关的代码注释.

依赖关系
---------------

Google Breakpad 中需要依赖 dbghelp 来进行 Windows 环境下的转储文件生成,
不过它默认情况下会引用 Windows SDK 中的 dbghelp 模块.

由于发布时我们需要将正确的 dbghelp.dll 放到安装目录下 (降低兼容风险),
直接使用 Windows SDK 的话不会有人帮你做这个事情.

为此, 我们将修改 Breakpad 工程中的代码, 使其优先使用 3rdparty 中的模块,
并在 map-server 工程中, 进行对应的文件复制工作.

最后更新
---------------
2019-6-22 by Sola丶小克 (CairoLee)
