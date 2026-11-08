# 如何维护数据库升级脚本

当本地功能或上游同步改变 SQL schema、初始化数据或可选 SQL 数据库结构时，需同时照顾全新安装与已有版本升级。先阅读 [发布目录映射](../architecture/release-pipeline.md)，确认改动属于主库、日志库还是 Web 库。

## 1. 确认起点和使用方式

记录支持的旧版本 schema、本次目标版本和对应上游提交。检查改动是否只用于 `use_sql_db`、是否区分 PRE/RE，以及社区和专业版是否存在不同输入。

查看 [sql-files/](../../../sql-files/) 的建库文件、[upgrades/](../../../sql-files/upgrades/) 的日期型上游变更及 [composer/community/](../../../sql-files/composer/community/) 的版本升级历史。对照历史 DDL 和当前结构，确定用户从旧版本升级时缺少哪些操作；不要仅按文件日期重复拼接。

## 2. 同步新建库和升级路径

1. 修改对应的 `main.sql`、`logs.sql`、`web.sql` 或物品、魔物等可选建库文件，使全新安装得到目标结构。
2. 在 `composer/community/{main,logs,web}/` 下维护对应版本的升级脚本，沿用 `upgrade_to_<版本>_<数据库>.sql` 命名。仅面向 SQL 数据库用法的脚本沿用已有 `_main_use_sql_db.sql` 分类。
3. 版本尚未发布时可以完善该版汇总脚本；已经发布的升级路径应通过后续版本脚本继续演进，避免已升级用户漏掉改动。
4. 对字段重命名、默认值、索引和数据迁移检查顺序与数据保留。若 YAML 与 SQL 表达同一数据，核对解析器、模式库、Pandas 补丁、转换工具和兼容视图。

专业版来源是 `composer/professional/`，按当前任务实际可用的来源维护。发布脚本只移动选中的版别目录；没有自动生成升级 SQL 或跨版别回退的步骤。

## 3. 更新发行说明和目录规则

[dawn_publish.py](../../../tools/python/dawn_publish.py) 已将所选版别的 `main`、`logs`、`web` 升级目录移动到运行包的同名数据库目录下。沿用现有分类添加版本文件时检查输出即可；引入新类别或新建库文件时才补相应的整理规则。

修改面向服主的 [发行版 SQL README](../../../tools/python/dist/sql-files/README.md)，说明版本顺序、必选或可选路径以及需要人工处理的数据事项。源码 [SQL README](../../../sql-files/README.md) 与发行版说明的目录用途不同。现行整理脚本没有创建 `use_sql_db/common/`，写说明时不要沿用这个过时路径。

## 4. 验证两种安装方式

在隔离数据库中分别执行全新建库和从支持的旧版本逐步升级，比较相关表、字段、索引、默认值与关键数据。覆盖受影响的 PRE/RE 或 `use_sql_db` 场景，核对读取、写入该数据的实际服务行为。测试数据迁移时保留能触发边界条件的旧记录。

运行 `python3 tools/python/tests/test_publish_layout.py` 检查公共目录整理；有新路径时同步补充断言。目录测试负责文件选择与布局，SQL 执行和数据一致性由上述数据库验证覆盖。实际运行包应包含需要的版本汇总升级，而原日期型 `upgrades/`、`composer/` 来源目录会被整理掉。

## 历史参考

- `e7dec3c8d5`：归纳数据库升级脚本以简化用户升级操作。
- `e929db7e91`：整理数据库创建脚本的目录结构。
- `fa54a7e4be`：区分社区版与专业版汇总脚本来源。
- `81987167a4`：同步 rAthena 时同时调整源码和 SQL，可用于检查联动范围。

用 `git show <提交号> -- sql-files tools/python` 查看对应改动；实现本次迁移时以当前 schema 和目标版本为准。
