# Pandas 相对 rAthena 的架构差异

Pandas 沿用 rAthena 的登录服、角色服、地图服、Web 服务及主要游戏逻辑，在其上维护中文环境适配、自有功能和开发发布设施。本页关注这些结构关系；功能开关详见 [pandas.hpp](../../../src/config/pandas.hpp)，具体指令说明仍在 `doc/pandas_*.txt`。

初次核对使用 Pandas `e6c34462cbf` 与已合入的 rAthena `67431ee25c`，完整基点见 [开发文档入口](../README.md)。

## 代码和数据入口

| 路径 | 职责与关注点 |
| --- | --- |
| `src/common/` | 各服务共享的 Core、SQL、编码、翻译、回调和崩溃诊断设施 |
| `src/login/`、`src/char/`、`src/map/`、`src/web/` | 服务实现；修改共享数据或协议时检查跨服务使用者 |
| `src/config/pandas.hpp` | Pandas 功能分组、细项开关和依赖关系 |
| `db/`、`conf/`、`npc/` | 默认数据库、配置、脚本及用户覆盖入口 |
| `sql-files/` | 新建库、上游日期型升级及 Pandas 版本汇总升级 |
| `tools/python/` | 代码注入助手、版本与语言工具、Dawn 编译发布脚本 |
| `.github/workflows/`、`tools/integration/` | 编译及 NPC/数据库验证入口 |
| `Jenkinsfile` | Windows 发布编译、可选符号和包产出、制品归档 |

## 功能差异集中在宏体系中

[pandas.hpp](../../../src/config/pandas.hpp) 从 `Pandas` 总开关进入功能组，再定义细项。源码用这些宏关联字段、实现和注册；部分替换逻辑在 `#else` 保留上游行为。宏之间也有依赖，例如异步脚本查询依赖 Future 回调支持。

这一布局使上游合并时可从宏定位 Pandas 的修改意图。维护原则见 [开发规范](../standards/development.md)；构建、数据与工具的独立差异需要另外检查。

## 本地化分成文件读取、消息和输出三个层次

- **文件读取**：`Pandas_Support_UTF8BOM_Files` 下，[utf8_defines.hpp](../../../src/common/utf8_defines.hpp) 将选定的 C I/O 调用映射到 `PandasUtf8`；[utf8.cpp](../../../src/common/utf8.cpp) 处理 UTF-8 检测、BOM 和本地旧编码兼容。读取文本时需留意转换入口，二进制文件另有跳过规则。
- **游戏消息**：[msg_conf.cpp](../../../src/common/msg_conf.cpp) 在 `Pandas_Adaptive_Importing_Message_Database` 下按系统语言选择简繁消息文件。
- **控制台消息**：[translate.cpp](../../../src/common/translate.cpp) 的 `TranslateDB` 加载译文字典，[showmsg.cpp](../../../src/common/showmsg.cpp) 在格式化前根据原文取译文；终端字符集转换还有独立开关。修改带格式占位符的原文时需核对译文和参数。

发布脚本里的简繁转译是另一条流程，见 [发布文档](release-pipeline.md)。

## SQL 连接与 Web 字符集有单独边界

`Pandas_SQL_Configure_Optimization` 将 [inter_athena.conf](../../../conf/inter_athena.conf) 的默认和分连接编码设置接入 [sql.cpp](../../../src/common/sql.cpp) 的 `Sql_SetEncoding`：分连接值优先，空值继承默认，`auto` 根据数据库及语言环境选择，最终设置 MySQL 连接字符集。

Web 端在 [web.cpp](../../../src/web/web.cpp) 获取连接编码，通过 [web.hpp](../../../src/web/web.hpp) 的 `U2AWE/A2UWE` 等转换宏处理客户端 UTF-8 与数据库、控制台之间的边界。维护时分别核对文件编码、连接字符集与表字段字符集。

## 异步 SQL 经主循环恢复脚本

地图服的异步查询链路为：

```text
script.cpp 提交查询
    → asyncquery.cpp：专用数据库连接，单工作线程顺序处理
    → future_add：结果与回调进入队列
    → Core::handle_main → do_future：地图服主线程执行回调
    → 缓存结果 → 恢复脚本并重跑当前指令 → 写回脚本变量
```

[asyncquery.cpp](../../../src/map/asyncquery.cpp) 管理主库和可选日志库连接；[future.cpp](../../../src/common/future.cpp) 由 [Core 主循环](../../../src/common/core.cpp) 消费。[script.cpp](../../../src/map/script.cpp) 中带返回变量的查询会挂起脚本，并用 `script_batch` 检查重载前的脚本上下文。具体成功、失败、取消和释放行为需沿这条链路维护。

这一设施的引入可追溯至 `23c0ada3c8`，其作用是把查询执行移出地图服主线程，再在主线程接续脚本。

## 数据补丁和用户覆盖保持分层

YAML 数据库及 `Footer.Imports` 来自 rAthena。Pandas 使用这一机制，在 [item_db.yml](../../../db/item_db.yml) 和 [status.yml](../../../db/status.yml) 等入口中按模式库 → `db/pandas/` 补丁 → `db/import/` 用户数据的顺序加载，使用户覆盖保持最后优先级。

`Pandas_Deploy_Import_Directories` 通过 [assistant.cpp](../../../src/common/assistant.cpp) 在启动时部署缺失的导入目录和模板文件，并用跨进程锁协调多个服务启动。数据变化时同时检查默认数据、补丁层和模板。

SQL 的分层发生在发布目录整理中：源码保留上游布局，同时维护 Pandas 版本汇总升级，运行包重排成按数据库和用途分类的目录。

## 崩溃诊断与符号归档相连

`Pandas_Google_Breakpad` 在 `Core::start` 安装 [crashdump.cpp](../../../src/common/crashdump.cpp) 的处理器，生成 `dumps/` 下的 minidump 和诊断信息。Dawn 编译记录版本、Git 分支及提交，符号归档保存相应 PE/PDB。分析崩溃时需匹配构建与符号，发布流程见下页。

## 生成器和构建入口属于维护设施

[create_*.py](../../../tools/python/README.md) 通过 [InjectController.py](../../../tools/python/libs/InjectController.py) 定位 `PYHELP` 标记，检查标记唯一、齐全以及源码编码，向宏、定义、注册等位置插入模板。它们帮助重复性工作，生成后的实际行为与文档仍由开发者完成。按任务使用 [How-to](../how-to/README.md)。

Pandas 在 `ba97dab414` 移除了 Autotools，当前入口是 [CMakeLists.txt](../../../CMakeLists.txt) 与 [rAthena.sln](../../../rAthena.sln) 及各项目文件。共享库还接入 Breakpad、OpenCC、uchardet 等依赖，新增文件或依赖时检查对应构建入口。

## 发布结构的历史动机

以下提交可通过 `git show <提交号>` 查看，提交说明明确记录了目录调整目的：

| 提交 | 结构调整 |
| --- | --- |
| `e7dec3c8d5` | 归纳数据库升级脚本，简化用户升级操作 |
| `e929db7e91` | 整理数据库创建脚本的目录结构 |
| `fa54a7e4be` | 将汇总升级脚本区分为社区版和专业版来源 |
| `81987167a4` | 适配所选 rAthena 基线，同时处理源码、宏和数据库变更 |

源码到运行包的实际目录映射与维护步骤见 [发布与数据库整理](release-pipeline.md)。
