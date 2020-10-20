熊猫辅助脚本说明
=============================================

在日常维护 Pandas 的过程中, 有大量重复的工作需要被经常执行, 这些工作通常都有既定的流程或者固定的模式.

为了便于提高工作效率, 我们实现了一些辅助脚本用来进行大量的半自动化工作.

脚本功能简介
--------------------------------------------

- 使用 `create_atcmd.py` 可以轻松的创建一个新的管理员指令
- 使用 `create_scriptcmd.py` 可以轻松的创建一个新的脚本指令
- 使用 `create_battlecfg.py` 可以轻松的创建一个新的战斗配置选项
- 使用 `create_mapflag.py` 可以轻松的创建一个新的地图标记
- 使用 `create_npcevent.py` 可以轻松的创建一个新的 NPC 事件

- 使用 `imple_src2utf8.py` 可以将 src 中的代码全部转换成 UTF8 编码
- 使用 `imple_versions.py` 可以用来快速调整多个工程的版本号
- 使用 `imple_translate.py` 可以根据对照表快速翻译部分 DB 信息
- 使用 `imple_extracter.py` 可以提取源码中的字符来构建翻译数据库

- 使用 `dawn_compile.py` 可以执行发布新版本之前的编译工作
- 使用 `dawn_symstore.py` 可以将 `dawn_compile.py` 生成的符号文件归档
- 使用 `dawn_publish.py` 可以构建发布新版本使用的压缩包 (包括自动汉化)


可预见的未来我们还会陆续的添加各种不同的半自动辅助脚本,

这些脚本使用 Python 3.x 版本进行编写, 你需要安装 Python 系统才能理解这些脚本.

第一步: 下载 Python 3.x
--------------------------------------------

请直接前往 https://www.python.org/downloads/windows/ 下载安装程序.

通常我们优先选用版本号较高的稳定版本(Stable Releases).

- 若您是 64 位系统, 下载 `Windows x86-64 executable installer` 即可.
- 若您是 32 位系统, 下载 `Windows x86 executable installer` 就可以了.

第二步: 安装 Python 3.x
--------------------------------------------

下载完成安装程序后, 直接运行启动它, 在第一个界面中按以下说明进行安装:

![第一步](./doc/images/python_installer/1.png)

若您是第一次安装, 那么安装完成到末尾会有一个选项, 需要您启用它:

![第二步](./doc/images/python_installer/2.png)

最终如果你看到如下图的选项, 那么就说明安装过程已经成功结束了:

![第三步](./doc/images/python_installer/3.png)

第三步: 确认 Python 能正常工作
--------------------------------------------

安装完成之后, 打开一个新的 `终端` 或者 `cmd` 窗口, 输入以下两个指令进行测试.

我们先输入以下指令来测试 python 指令是否可用:

```
python --version
```

如果 python 指令可用, 那么会显示当前安装的 python 版本号. 例如:

```
C:\Users\Sean>python --version
Python 3.7.1
```

接下来我们输入以下指令来测试 pip 指令是否可用:

```
pip --version
```

如果 pip 指令可用, 那么会显示当前安装的 pip 版本号和安装位置. 例如:

```
C:\Users\Sean>pip --version
pip 19.1.1 from c:\users\sean\appdata\local\programs\python\python37\lib\site-packages\pip (python 3.7)
```

第四步: 安装 pipenv 虚拟环境管理工具
--------------------------------------------

请用在你的 `终端` 或者 `cmd` 中使用 cd 指令切换到 `tools\python` 目录中去,

然后使用以下指令来安装 pipenv 虚拟环境管理工具:

```
pip install pipenv
```

> 在这一步中如果执行速度太慢的话, 可以把你的 pip 设置为从`阿里云`镜像来下载各种库文件, 这样速度会有大幅提升: https://www.jianshu.com/p/e2dd167d2892 (请参考其中的"永久修改"章节, 后面的 "命令行操作" 章节不用看)

第五步: 创建 pipenv 虚拟环境
--------------------------------------------

请用在你的 `终端` 或者 `cmd` 中使用 cd 指令切换到 `tools\python` 目录中去,

然后使用以下指令来让 pipenv 自动根据 Pipfile 中的配置来创建虚拟环境:

```
pipenv install
```

执行的过程中, 程序会自己下载全部需要的依赖, 请保持网络畅通并耐心等待.

第六步: 试试执行某个辅助脚本
--------------------------------------------

当上述步骤全部准备完毕之后, 就可以运行我们的辅助脚本了.

遗憾的是, 在 Windows 操作系统中你并不能再直接双击 py 文件来启动对应的脚本了, 而是需要使用命令行来启动他们.

请用在你的 `终端` 或者 `cmd` 中使用 cd 指令切换到 `tools\python` 目录中去, 

然后使用以下指令来运行辅助脚本:

```
pipenv run imple_src2utf8.py
```

如果你可能需要运行一系列脚本, 且讨厌每次都加 `pipenv run` 前缀的话, 

可以使用 `pipenv shell` 进入虚拟环境的控制台中, 在里面执行的全部 python 脚本都是我们专属的虚拟环境:

```
pipenv shell

imple_src2utf8.py
dawn_compile.py
dawn_symstore.py
dawn_publish.py
...
```

关于未来更新
--------------------------------------------

如果你某天执行一个辅助脚本时提示有模块没找到, 

请用在你的 `终端` 或者 `cmd` 中使用 cd 指令切换到 `tools\python` 目录中去, 

然后再次执行 `pipenv install` 即可.

通常每当我们新开发一个辅助脚本, 就会在 Pipfile 中添加对应的依赖. 

执行 `pipenv install` 就会根据 Pipfile 中的信息重新构建虚拟环境, 缺少的模块也会被自动安装.

在 Windows 环境下的小技巧
--------------------------------------------

如果你觉得每次打开 `cmd` 然后切换到 `tools\python` 目录非常麻烦, 

那么可以试试直接在我的电脑中进入 `tools\python` 目录, 然后在地址栏中直接输入 `cmd` 直接回车. 

系统会启动一个普通权限的 `cmd` 并自动切换到 `tools\python` 目录, 

这样就不用每次手动使用 `cd` 指令来进行切换了.
