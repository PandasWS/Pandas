# 发布与数据库整理

Pandas 的运行包由 Dawn 脚本从源码、编译产物及发行覆盖文件组装。源码目录保留开发和上游同步所需的结构，交付目录按服主使用方式重新组织。

完整的升版、正式提交、构包、GitHub Release 与下一轮开发步骤见 [发布正式版](../how-to/publish-release.md)，其中附有历史版本的实际记录。

## 流程与入口

```text
Jenkins / 本地配置
    → dawn_compile.py：PRE / RE 编译，记录构建身份
    → dawn_symstore.py：可选符号归档
    → dawn_publish.py：导出 HEAD → 按模式、语言分别解包
        → imple_translate.py：文本本地化
        → arrange_common：发行覆盖、文件清理、SQL 目录重排
        → arrange_pre_renewal / arrange_renewal：模式筛选、补入二进制
        → ZIP → Jenkins 环境中额外复制到 artifacts/packages
```

[Jenkinsfile](../../../Jenkinsfile) 在 Windows 中准备 Pipenv；启用打包时，先运行 `tests/test_yaml.py` 检查实际 YAML 加载及双语言转译，通过后调用 [dawn_compile.py](../../../tools/python/dawn_compile.py)，再按参数选择符号归档和包生成；这两个后续任务可并行，成功后归档 `artifacts/**`。普通 GitHub Actions 编译验证另由 `.github/workflows/` 管理。

本地包输出目录由 `publish_path` 配置决定，空值时采用项目目录的 `../Release`。

编译脚本使用 Visual Studio 的 `rAthena.sln`，按配置构建 PRE/RE 的 Release Win32 产物；PRE 产物以 `-pre` 区分，并在后续清理前暂存。构建注入 Git 分支和提交以供诊断。[dawn_symstore.py](../../../tools/python/dawn_symstore.py) 收集 PE/PDB，按文件与标识组织符号；上传或外部归档由配置决定。

[config.yml](../../../tools/python/config.yml) 的默认模式为 PRE、RE，语言档为 `gbk`、`big5`，版别配置及 override 可调整取值。凭据由环境中的 Secret 配置提供，示例和开发文档只记录配置名，不保存实际值。

## 源码导出与语言处理

[dawn_publish.py](../../../tools/python/dawn_publish.py) 的 `export()` 使用 `git archive HEAD`，排除 `3rdparty`，再对每个模式和语言组合独立解包。导出使用已提交的 HEAD；本地未提交文件不会自动进入导出。二进制则来自编译目录，发布时应确保与导出提交匹配。

`GBK`、`BIG5` 是发行语言档名称。实际转译调用 [imple_translate.py](../../../tools/python/imple_translate.py)，两个档分别使用目标语言的对照表。繁体档在启用文档处理时，额外用 OpenCC 转换选定的配置、文档、SQL 和发行覆盖文件；相关文本仍按工具规则保存为 UTF-8 或 UTF-8 BOM。文件编码、运行时字符集和数据库字段编码应分别处理。

## 发行覆盖与排除

`arrange_common()` 先清理源码、测试等开发目录，将 [tools/python/dist/](../../../tools/python/dist/) 覆盖到包根目录，再整理用户文档和 SQL。覆盖树里的 SQL README 面向最终目录，根 [sql-files/README.md](../../../sql-files/README.md) 则说明源码布局。

开发规则集中在根 `AGENTS.md` 和 `doc/development/`，在覆盖树合入后从包中移除。已有用户文档继续按原规则保留。排除仅作用于运行包整理，普通 Git 源码归档保留这些开发资料。

PRE/RE 各自的整理函数随后移除另一模式的 `db/`、`npc/` 数据，补入对应程序与 DLL。`arrange_common()` 生成的两套 `use_sql_db` 建库目录都会保留在包中。

## SQL 从源码到运行包

下表路径均相对于 `sql-files/`：

| 源码来源 | 运行包目标 |
| --- | --- |
| `main.sql` | `main/creation/01.main.sql` |
| `roulette_default_data.sql` | `main/creation/02.roulette_default_data.sql` |
| `item_db*`、`mob_db*`、`mob_skill_db*` 的 PRE/RE 建库文件 | `main/creation/use_sql_db/{pre-renewal,renewal}/`，按脚本中的 01–09 顺序编号 |
| `compatibility/` 中相应物品兼容视图 | 各模式 `use_sql_db/.../optional/` |
| `upgrades/premium_storage.sql` | `main/creation/optional/premium_storage.sql` |
| `logs.sql` | `logs/creation/01.logs.sql` |
| `web.sql` | `web/creation/01.web.sql` |
| `composer/<版别>/main/` | `main/upgrades/` |
| `composer/<版别>/logs/` | `logs/upgrades/` |
| `composer/<版别>/web/` | `web/upgrades/` |

版别由 `is_commercial` 选择 `community` 或 `professional`，选中来源目录存在才移动；社区仓库跟踪的是 `composer/community/`。脚本支持专业版来源，专业版缺目录时没有回退到社区版的规则。

整理先保存 `premium_storage.sql`，随后删除原日期型 `upgrades/` 和 SQL `tools/`，最后移除 `composer/` 与 `compatibility/` 来源目录。当前代码没有创建 `use_sql_db/common/`；维护发行说明时以实际输出为准。

**版本汇总升级脚本是仓库中维护的输入。** 打包脚本只选择、移动它们。因此吸收上游 schema 变更时，需同时维护新建库文件与合适版本的汇总升级，操作见 [数据库升级 How-to](../how-to/update-database-upgrades.md)。

## 修改与验证

变更发布布局时，同时检查 `arrange_common()`、两个模式整理函数及 `dist/` 的用户说明。临时目录可以验证排除范围和 SQL 路径；真实发包还需要匹配的 Windows 编译产物、语言转换与配置。

开发文档排除及 SQL 布局的本地检查：

```sh
python3 tools/python/tests/test_publish_layout.py
```

此测试通过标准库从发布脚本提取目录整理函数，在临时目录处理仓库中的 SQL、批处理和发行覆盖文件；覆盖两个版别分支及覆盖树重新带入开发文档的情况。它不执行发布脚本顶层初始化、安装依赖、访问凭据或上传制品。专业版输入使用测试哨兵，仅验证目录选择。
