dbghelp
==============

用于生成崩溃转储文件的 Windows 模块.
为了能够正确获得操作系统信息, 这里选用的是 10.0.18362.1 版本 (从 Windows SDK 中提取).

依赖关系
---------------
Google Breakpad 中需要依赖 dbghelp 来进行 Windows 环境下的转储文件生成,
不过它默认情况下会引用 Windows SDK 中的 dbghelp 模块.

由于发布时我们需要将绝对符合程序预期的 dbghelp.dll 放到安装目录下 (降低兼容风险),
直接使用 Windows SDK 的话, 版本兼容性无法得到保证.

为此, 我们修改了 Google Breakpad 工程中的代码, 使其优先使用 3rdparty 中的模块,
并在 map-server 工程完成编译后, 进行对应的文件复制工作.

最后更新
---------------
2019-08-11 by Sola丶小克 (CairoLee)
