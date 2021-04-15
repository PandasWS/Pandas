<h1 align="center">熊猫模拟器</h1>

<div align="center">
  基于 <code>rAthena</code> 构建的开源跨平台中文仙境传说模拟器
</div>

<br />

<div align="center">
  <!-- Build Status -->
  <a href="https://ci.appveyor.com/project/PandasWS/Pandas/branch/master">
    <img alt="Build Status" src="https://img.shields.io/appveyor/build/PandasWS/Pandas/master?style=flat-square">
  </a>

  <!-- LGTM Alerts -->
  <a href="https://lgtm.com/projects/g/PandasWS/Pandas/alerts/">
    <img alt="LGTM Alerts" src="https://img.shields.io/lgtm/alerts/github/PandasWS/Pandas?style=flat-square">
  </a>

  <!-- LGTM Cpp Grade -->
  <a href="https://lgtm.com/projects/g/PandasWS/Pandas/context: cpp">
    <img alt="LGTM Cpp Grade" src="https://img.shields.io/lgtm/grade/cpp/github/PandasWS/Pandas?style=flat-square">
  </a>

  <!-- LGTM Python Grade -->
  <a href="https://lgtm.com/projects/g/PandasWS/Pandas/context: python">
    <img alt="LGTM Python Grade" src="https://img.shields.io/lgtm/grade/python/github/PandasWS/Pandas?style=flat-square">
  </a>

  <!-- GitHub contributors -->
  <a href="https://github.com/PandasWS/Pandas/graphs/contributors">
    <img alt="GitHub contributors" src="https://img.shields.io/github/contributors/PandasWS/Pandas?style=flat-square">
  </a>

  <!-- GitHub license -->
  <a href="https://github.com/PandasWS/Pandas/blob/master/LICENSE">
    <img alt="GitHub license" src="https://img.shields.io/github/license/PandasWS/Pandas?style=flat-square">
  </a>
</div>

<div align="center">
  <h3>
    <a href="https://pandas.ws">
      访问官网
    </a>
    <span> | </span>
    <!-- <a href="https://github.com/choojs/choo-handbook">
      Handbook
    </a>
    <span> | </span> -->
    <a href="https://github.com/PandasWS/Pandas/releases">
      下载
    </a>
    <span> | </span>
    <a href="https://github.com/PandasWS/Pandas/graphs/contributors">
      贡献者
    </a>
    <!-- <span> | </span>
    <a href="https://www.reddit.com/r/choojs/">
      Reddit
    </a>
    <span> | </span>
    <a href="https://webchat.freenode.net/?channels=choo">
      Chat
    </a> -->
  </h3>
</div>

<div align="center">
  <sub>Craft with ❤︎ by
  <a href="https://twitter.com/yoshuawuyts">CairoLee / Sola 丶小克</a> and
  <a href="https://github.com/PandasWS/Pandas/graphs/contributors">
    Contributors
  </a>
</div>

## 神奇的小目录
-   [背景](#背景)
-   [模拟器特色功能](#模拟器特色功能)
-   [配置要求](#配置要求)
-   [编译说明](#编译说明)
-   [常见问题](#常见问题)
-   [加入爱好者社区](#加入爱好者社区)
-   [模拟器相关项目推荐](#模拟器相关项目推荐)
-   [维护者](#维护者)
-   [如何作出贡献](#如何作出贡献)
-   [许可协议](#许可协议)

## 背景
仙境传说是一款非常经典的游戏，伴随着绝大多数玩家们的成长。感谢这款游戏，我们在茫茫人海中
认识了彼此，虽然我们不在同一个城市，但都彼此各自成长。从商业上看她可能不是一款最好的游戏，
但是情感上她已经成为了我们记忆中的一部分。

对这款游戏的热爱使得全世界的爱好者们为她打造出了一系列的模拟器，其中以 Athena 系列模拟器
尤为健壮，还在维护的 Athena 系列模拟器中又以 rAthena 最为活跃。

但 rAthena 是一个国际开源项目，对中文并不是特别友好。再加上其定位非常明确，即：只做尽可
能接近官方的模拟器 ( 意味着其改动依据都将以 kRO 为准；而非官方的改动都难以通过 ) 。

因此我们将站在 rAthena 的巨人肩膀上二次开发了 Pandas 模拟器 ( 熊猫模拟器 ) ，我们的定位
也非常朴素和简单：对中文用户友好的仙境传说模拟器。

> 熊猫模拟器的前身是 rAthenaCN 模拟器，rAthenaCN 模拟器闭源且仅支持 Windows 环境，当前
已经不再进行维护。而熊猫模拟器的所有基础功能和代码都将毫无保留直接开源。欢迎对源代码感兴趣
的朋友一起来进行学习探讨，或许几年之后社区需要靠你们来延续。

## 模拟器特色功能
主要是与 rAthena 模拟器相比的差异化功能，这些功能可能其他闭源模拟器已经实现，但在熊猫里面
相关代码是开源的，方便您学习或者随意修改。欢迎您阅读熊猫模拟器的 [功能配置文件]
来了解我们做过的所有功能改动。甚至通过此文件，您也可以直接关闭熊猫模拟器对某些功能的更改，
使之恢复到与 rAthena 行为一致。

![启动截图](.github/SCREEN_SHOT.gif)

### 值得一提的功能
-   __终端信息汉化__ —— 汉化绝大部分终端提示的文本信息，兼容简体和繁体中文
-   __变态服拓展包__ —— 解锁角色的素质上限， 可以给角色几百万的 STR 等六维素质值
-   __疾风缓存__ —— 大幅加速 YAML 数据文件的读取速度 ( 大幅缓解焦虑， 提高调试效率 )
-   __单位光环系统__ —— 可以为任意游戏单位设置光环特效，可将多个视觉效果组合成光环
-   __简易挂机机制__ —— 支持：离开 / 挂机两种模式， 挂机单位可被 Unit 系列脚本指令控制
-   __护身符道具__ —— 能够将道具设置为护身符，放在背包即可生效
-   __效果脚本拓展__ —— 使效果脚本 ( Bonus Script ) 拥有唯一编号， 可被单独移除
-   __MySQL 8.0 支持__ —— 能够支持 MySQL 8.0 及更新的数据库而不会出现连接错误
-   __崩溃转储生成__ —— 程序崩溃时能自动生成并上报转储文件，帮助研发人员解决问题

### 指令集和事件拓展
在 rAthena 原来的指令集和事件基础上我们拓展了更多的小玩意，使脚本作者们能够充分发挥自己
的想象力，给玩家带来有趣和好玩的游戏体验。具体详情请见对应的说明文档：

-   [管理指令 ( Atcommands )](doc/pandas_atcommands.txt)
-   [脚本指令 ( Script Commands )](doc/pandas_script_commands.txt)
-   [事件标签 ( NPC Event Label )](doc/pandas_events.txt)
-   [地图标记 ( Mapflags )](doc/pandas_mapflags.txt)
-   [效果调整器 ( Bonus )](doc/pandas_bonus.txt)

> 若您有拓展需求或者想法，劳驾请您 [建立新的反馈] 给我们，或者
 [加入爱好者社区](#加入爱好者社区) 给管理员进行反馈，我们将挑选优质且有远见的需求优先进行实现。

## 配置要求
使用此模拟器会有两套不同的使用场景：__开发者场景__ 和 __使用者场景__，其中开发者场景下
的配置要求会更高一些，以下为不同场景下要求的软硬件配置信息，请根据使用场景对号入座。

| Item / Scenes | Developer - Win | User - Win | Developer - Unix | User - Unix |
| :------------ | :-------------: | :--------: | :--------------: | :---------: |
| CPU           | ≥4 Threads      | ≥2 Threads | ≥4 Threads       | ≥2 Threads  |
| Memory        | ≥8 GB           | ≥4 GB      | ≥4 GB            | ≥4 GB       |
| Free Space    | ≥6 GB           | ≥2 GB      | ≥6 GB            | ≥2 GB       |

> 上表是各个场景下的最低配置，如果条件允许的话，可以在上表中的数值基础上直接乘以 2 得到
推荐的配置信息。有条件的同学一定要换成 SSD 固态硬盘！

## 编译说明
熊猫模拟器是一个开源项目，因此当您获取源码后需要手动进行编译才能获得可以运行的二进制文件；
亦或者您可以前往 [下载页面] 直接下载预编译好的版本，直接使用 ( 新手推荐 ) 。

> 注意：此处的编译说明仅提供最精简的指引信息，完成编译之后还需要搭建 MySQL 服务器并进行
配置才能使模拟器成功运行起来。关于从编译到完成配置的全环节详细信息请见 [完整版说明文档] 。

### Windows 环境下编译

-   安装 Visual Studio 2019 社区版，安装时请勾选 __「使用 C++ 的桌面开发」__ 模块
-   获取模拟器源代码 ( 推荐使用 Git 克隆而不是直接下载源代码，因为会丢失仓库信息 )
  -   建议将仓库存放到纯英文路径，因为某些第三方库对中文支持不友好 ( 即全路径中不要出现中文 )
-   解压源代码后先双击 `3rdparty\boost\bootstrap.bat` 编译 `Boost Libraries`
-   打开 `rAthena.sln` 选择编译模式 ( 调试用 Debug，正常用 Release ) 并生成解决方案

> 您可能会发现仓库中 `db` 目录里的各类信息并未汉化，而且与直接从 [下载页面]
获取到的压缩包目录结构并不一致 ( 有很多散乱的文件 ) 。这是为了与 rAthena 仓库保持最小
改动而有意为之，您可以通过预置的辅助脚本来完成编译和发布打包操作，以此来获得一个对使用者
友好的压缩包。

### Unix 环境下编译

熊猫模拟器并不支持 rAthena 官方的 `.\configure && make` 编译方式，仅留下了相对好维护的
CMake 编译方式，对于这一点有经验的同学注意一下，避免被坑。在本例子中，我们将使用
`Ubuntu 18.04` 发行版进行演示。

```shell
## 获取最新的 package list 以及更新本地程序包
$ sudo apt-get update -y && sudo apt-get upgrade -y

## 安装 git / git-lfs / wget / gcc 编译组件
$ sudo apt install git git-lfs wget build-essential -y

## 确认 gcc 的版本 ( 预期返回版本号 ≥ 7.4.0 )
$ gcc --version

## 安装编译 cmake 的所需依赖库 ( openssl )
$ sudo apt install openssl libssl-dev -y

## 接下来下载、解压、配置，并编译安装 cmake 3.16 版本
$ wget https://github.com/Kitware/CMake/releases/download/v3.16.0-rc1/cmake-3.16.0-rc1.tar.gz \
  && tar -xzvf cmake-3.16.0-rc1.tar.gz \
  && cd cmake-3.16.0-rc1 \
  && ./bootstrap && make -j4 && sudo make install

## 此时你可以移除上一步中下载的 cmake 压缩包以及解压后的目录
$ cd .. && rm -rf cmake-*

## 确认 cmake 的版本 ( 预期返回版本号 ≥ 3.16.0 )
$ cmake --version

## 找个位置，然后克隆熊猫模拟器源代码 ( 此处我们将其放在用户主目录中 )
$ git clone https://github.com/PandasWS/Pandas.git ~/Pandas

## 安装熊猫模拟器的依赖
$ sudo apt install libmysqlclient-dev zlib1g-dev libpcre3-dev -y

## 编译熊猫模拟器自带的 Boost Libraries
## 下列指令中的第一个 cd 若您保存熊猫模拟器仓库的位置有变，请自行更改
$ cd ~/Pandas/3rdparty/boost/ && bash bootstrap.sh && ./b2

## 编译熊猫模拟器: 先建立 cbuild 临时目录并进入到目录中
$ cd ~/Pandas && mkdir cbuild && cd cbuild

## 生成 makefile 文件
$ cmake -G "Unix Makefiles" ..

## 执行编译 ( 以后想重新编译只需进入 cbuild 目录执行 make 即可 )
$ make

## 返回仓库根目录即可看到编译产物
$ cd ~/Pandas

## 配置正确的数据库连接后，就可以启动模拟器啦
$ ./athena-start start
```

## 常见问题

### 请问有一键开服端吗？

很遗憾，熊猫模拟器当前并没有官方的一键开服套件。熊猫模拟器当前面向的主要用户，并非是一窍
不通的萌新玩家，毕竟搞懂一个开源项目需要大量的背景知识。我们亦希望通过早期维持使用难度
以便召集一群知识覆盖面相当，有经验的使用者。为以后熊猫模拟器的迭代做储备。

### 当前对应仙境传说的哪个 EP 版本？

此问题无法答复，因为模拟器并非官方服务端，我们游戏中的所有功能都是「尾随」官方之后通过一些
爱好者在官服的体验游玩，分析和观察后模拟实现的；官服的每个 EP 版本都是无数小调整组合在
一起的 「集合包」，这些调整中有的模拟起来比较简单而有的非常复杂，这就导致了模拟器实装这些
改动的时间的无法被统一。

玩家若能看到某个模拟器搭设的服务器中，宣传语提到了所谓实装 EP XX.X 资料片，绝大多数都
只是实装了一些关键特征而已。比如：某 EP 版本才拥有的副本，出现在了该服务器中，因此宣传的
时候以：实装 EP XX.X 来吸引玩家罢了！

当然，凡事总有例外：有的爱好者有着自己的追求，在掌握大量的数据和资料的情况下，甚至能够模拟
到玩家几乎无感知，但这必定花费了大量的心血。

### 支持的客户端封包版本是哪个？

截止 `2021 年 4 月 11 日 ` 熊猫模拟器 v1.1.0 版本，默认的客户端封包版本是 `20180620`，
且支持到 rAthena 官方等同的 `20200409` 客户端封包版本。之所以默认的客户端封包版本不使用
`2020` 年是因为 `20180620` 是近几年较为稳定，服务端对客户端的支持相对成熟的版本。

在 `2020` 年之后的客户端中虽然有非常多的新特效和功能，但是其中有些功能并没有被模拟器很好
的支持，因此可能会在使用的过程中发现一些功能缺失；不过别担心，这些功能也不影响玩家的主要
操作，若您希望客户端封包使用 `20200409`，那么只需下载源代码后通过修改 `src/config/packet.hpp`
文件中 `PACKETVER` 的定义，随后重新编译即可。

### 哪里能获取配套客户端？

若您有能力，我们建议您自己整理一套客户端，这才能最好的符合您自己的使用需求。通常所谓的配套
客户端其实通常只是封包版本匹配，能成功进入游戏罢了。但是游戏中还会有无数的道具、魔物、地图，
这些内容必须让它与模拟器服务端中的数据完全匹配才能算真正意义上的配套，但很难。

> 尽管如此：我们将全球各个官服的资源文件进行提取和混合，创建了名为 [LeeClient]
的客户端整合项目，在这个项目中你可以随意切换客户端版本来进行调试。

__此外这套 LeeClient 客户端依然存在很多缺陷：__

-   对繁体中文环境支持不够友好 ( 当前几乎不能使用 )
-   支持从 `20130807` 到 `20180620` 的几个主要客户端封包版本，但不支持 `2020` 年的客户端
-   它是面向 GM 设计的，而不是面向玩家设计的，因此无法轻松分发给玩家使用
-   虽然实现了自动汉化，甚至能自动汉化图片，但是翻译对照表并不完整，很多内容都是韩文乱码

### 有全汉化的官方 NPC 脚本文件吗？

没有！整个模拟器截止 `2021 年 4 月 11 日 ` 有大约 38 万条文本，这是社区十几年无数人的积累。
对这么大量的文本进行汉化和校对不是简单的「懂英文」就搞得定的，特别费时费力！

想找到精校版免费的官方 NPC 脚本，几乎不太可能。如果是在有需求的话，还是建议直接找熟人或者
网上爬其他人汉化好的脚本来用吧。但需要注意的是，由于 NPC 脚本中混合了对话文本和与玩家的
交互逻辑。别人分享的汉化脚本如果全盘接收不加审阅的话，可能会存在刻意预埋的后门，导致服务器
被提权或者被黑。

## 加入爱好者社区
我们的主要受众是中文用户，因此我们使用 [腾讯 QQ] 进行即时通讯。

熊猫模拟器交流群的群号码是：`928171346` 若您已安装 QQ 客户端那么请直接：
<a target="_blank"
  href="https://qm.qq.com/cgi-bin/qm/qr?k=IgPtPLCkZh0RbFA_MzK2ny76iX_phO2P&jump_from=webapi">
  <img border="0" src="https://pub.idqqimg.com/wpa/images/group.png"
    alt="PandasWS" title="PandasWS">
</a>

## 模拟器相关项目推荐

-   [rAthena] - 熊猫模拟器、BetterRA、99MaxEA 等众多模拟器的共同的「祖先」
-   [Hercules] - 基于 rAthena 构建的 C++ 模拟器，使用 OPP 思想和 libconfig 作为数据文件解析库
-   [NEMO] - 老牌的仙境传说客户端 DIFF 工具，目前由 Andrei Karas 维护
-   [WARP] - 由 NEMO 的原作者建立的新 DIFF 工具，或许未来有一天会替代 NEMO？
-   [roBrowser] - 网页版的仙境传说客户端，虽然现已停止维护，但非常具有参考价值
-   [roint] - 客户端常见文件格式 ( 如：gat、rsw、rsm、spr、rgz ) 的解析代码，具有参考价值
-   [unityro] - 基于 Unity3D 实现的仙境传说客户端 ( 近期活跃，值得关注 )

## 维护者
-   [CairoLee / Sola 丶小克] —— 项目主持人

## 如何作出贡献
有关如何为熊猫模拟器做出贡献的详细信息， 请参见 [贡献说明] 。

### 主要贡献者
- [HongShin] —— 积极的发现了很多缺陷和故障，使得项目的稳定性、健壮性得以提升
- [西瓜] 与 [小纪] —— 无条件捐献了熊猫模拟器若干个月的服务器租赁费用

### 活跃贡献者
特别感谢近期活跃的贡献者，排名不分先后（通常后加入的写最前面）

[Renee]、[NIFL]、[人鱼姬的思念]、[♬喵了个咪]、[文威]

## 许可协议
版权所有 © 熊猫模拟器开发团队 - 授权许可协议 [GNU General Public License v3.0](LICENSE)

[下载页面]: https://github.com/PandasWS/Pandas/releases
[功能配置文件]: src/config/pandas.hpp
[建立新的反馈]: https://github.com/PandasWS/Pandas/issues/new/choose
[完整说明文档]: https://www.wolai.com/pandasws/8vULeuLGhiVqDA3EbfnUum?theme=light
[LeeClient]: https://github.com/PandasWS/LeeClient
[腾讯 QQ]: https://im.qq.com
[贡献说明]: .github/CONTRIBUTING.md

[rAthena]: https://github.com/rathena/rathena
[Hercules]: https://github.com/HerculesWS/Hercules
[NEMO]: https://gitlab.com/4144/Nemo
[WARP]: https://gitlab.com/Neo-Mind/WARP
[roBrowser]: https://github.com/vthibault/roBrowser
[roint]: https://github.com/open-ragnarok/roint
[unityro]: https://github.com/guilhermelhr/unityro

[CairoLee / Sola 丶小克]: https://github.com/CairoLee
[HongShin]: https://github.com/Hong-Shin
[人鱼姬的思念]: mailto://327945477@qq.com
[Renee]: mailto://rne0430@gmail.com
[♬喵了个咪]: mailto://82558223@qq.com
[文威]: mailto://gods.cwk@gmail.com
[小纪]: mailto://a659347@gmail.com
[西瓜]: mailto://x79515@gmail.com
