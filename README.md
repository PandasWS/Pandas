
# 熊猫模拟器
[![Build Status](https://travis-ci.com/PandasWS/Pandas.svg?branch=master)](https://travis-ci.com/PandasWS/Pandas) [![Build status](https://ci.appveyor.com/api/projects/status/github/PandasWS/Pandas?branch=master&svg=true)](https://ci.appveyor.com/project/CairoLee/Pandas/branch/master) [![Total alerts](https://img.shields.io/lgtm/alerts/g/PandasWS/Pandas.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/PandasWS/Pandas/alerts/) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/PandasWS/Pandas.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/PandasWS/Pandas/context:cpp) [![Language grade: Python](https://img.shields.io/lgtm/grade/python/g/PandasWS/Pandas.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/PandasWS/Pandas/context:python) ![GitHub](https://img.shields.io/github/license/PandasWS/Pandas.svg) ![GitHub repo size](https://img.shields.io/github/repo-size/PandasWS/Pandas.svg)
> Pandas (熊猫模拟器) 是一个开源的多人协作的软件开发项目. 致力构建符合中文用户习惯的大型多人在线角色扮演游戏服务端. 该程序使用 C++ 编写, 遵循 C++11 标准. 程序的灵活性非常强大, 可以对 NPC, 传送点等一系列内容进行修改. 该项目由全球各地的志愿者小组, 以及提供质量检查和支持的庞大社区共同管理. Pandas 基于 rAthena 构建, 而 rAthena 是 eAthena 项目的延续.

### 目录
1. [配置要求](#1-配置要求)
2. [安装说明](#2-安装说明)
3. [故障排除](#3-故障排除)
4. [更多文档](#4-更多文档)
5. [修改声明](#5-修改声明)
6. [如何贡献](#6-如何贡献)
7. [许可协议](#7-许可协议)

## 1. 配置要求
在使用 Pandas (熊猫模拟器) 之前, 您需要确保拥有符合要求的硬件配置, 并准备好某些工具或者应用程序. 根据操作系统的不同, 所需的工具也可能不同, 请看下文.

### 硬件配置要求
硬件类型 | 最低配置 | 推荐配置
:------|:------|:------
处理器 | 1 核 | 2 核
内存容量 | 1 GB | 2 GB
磁盘空间 | 300 MB | 500 MB

### 操作系统 & 首选编译器
操作系统 | 编译器
:------|:------
Linux  | [gcc-5 或更新版本](https://www.gnu.org/software/gcc/gcc-5/) / [CMake >= 3.14 ](https://cmake.org/download/)
Windows | [MS Visual Studio 2015, 2017, 2019](https://www.visualstudio.com/downloads/)

> 熊猫模拟器依赖 Boost C++ Libraries 1.71.0 版本, 若使用 Visual Studio 作为编译器的话, 推荐大家直接上 VS2019 吧.

### 必备软件
应用程序 | 名称
:------|:------
数据库引擎 | [MySQL 5 或更新版本](https://www.mysql.com/downloads/) / [MariaDB 5 或更新版本](https://downloads.mariadb.org/)
Git | [Windows](https://gitforwindows.org/) / [Linux](https://git-scm.com/download/linux)

### 可选软件
应用程序 | 名称
:------|:------
数据库管理程序 | [MySQL Workbench 5 或更新版本](http://www.mysql.com/downloads/workbench/)
TortoiseGit | [TortoiseGit](https://tortoisegit.org/download/)

## 2. 安装说明 

  * [Windows](https://github.com/rathena/rathena/wiki/Install-on-Windows)
  * [CentOS](https://github.com/rathena/rathena/wiki/Install-on-Centos)
  * [Debian](https://github.com/rathena/rathena/wiki/Install-on-Debian)
  * [FreeBSD](https://github.com/rathena/rathena/wiki/Install-on-FreeBSD)

## 3. 故障排除
如果您在启动服务端程序时遇到问题, 首先要做的就是检查控制台(黑框)上正在发生的事情.
在大多数情况下, 所有常见的问题只需要查看给出的错误消息即可解决.
如果您有更多需要故障需要寻求技术支持, 可以查看我们的 [Wiki](https://github.com/rathena/rathena/wiki) 或 [论坛](https://rathena.org/forum).

## 4. 更多文档
熊猫模拟器在 doc 目录中拥有大量的帮助文档和 NPC 示例脚本.
其中包括 NPC 脚本指令, 管理员指令, 组别权限和封包结构等许多主题.
我们建议所有所有用户花点时间浏览该目录, 若问题无法解决再寻求其他帮助.

## 5. 修改声明
熊猫模拟器是开源软件, 您可以随意基于它修改出自己的模拟器.
但当您宣传自己的模拟器时, 请不要使用 `Pandas` 或 `熊猫模拟器` 作为产品名称.
您可以写: 基于 `Pandas` 或 `熊猫模拟器` 构建.
您能使用熊猫模拟器作为您构建新模拟器的基础, 是我们的荣幸, 感谢您的配合.

## 6. 如何贡献
有关如何为 Pandas 做出贡献的详细信息, 请参加 [CONTRIBUTING.md](https://github.com/PandasWS/Pandas/blob/master/.github/CONTRIBUTING.md)!

## 7. 许可协议
版权所有 (c) 熊猫模拟器开发团队 - 根据许可协议 [GNU General Public License v3.0](https://github.com/PandasWS/Pandas/blob/master/LICENSE)
