# 发布正式版

本指南依据 v1.2.2、v1.2.3、v1.2.4 的 Git 历史、GitHub Release 及仓库发布工具还原，于 2026-11-08 核对。流程分为版本准备、正式提交进入 `master`、Windows 制品构建、GitHub Release 发布和下一轮开发升版。

## 历史上的一次完整发布

以最近的 [v1.2.4](https://github.com/PandasWS/Pandas/releases/tag/v1.2.4) 为例，Git 与 GitHub 时间换算为 UTC+8；ZIP 文件名采用构建机本地时间，本身不含时区：

| 时间 | 记录 |
| --- | --- |
| 2024-09-22 20:33:13 | `c5232b2dca` 修正复兴后任务数据的 YAML 语法 |
| 20:33:30 | `2528d02d92`「发布 v1.2.4 正式版」，版本 `1.2.4.1 → 1.2.4.0`，同时更新 `CHANGELOG.md` |
| 21:36–21:48（文件名） | 各组合开始处理时取得的打包目录命名时间 |
| 23:20:24–23:20:38 | GitHub 记录四个附件由 `CairoLee` 上传完成 |
| 23:20:41 | GitHub Release 正式发布，记录的目标分支为 `master`，轻量标签 `v1.2.4` 指向 `2528d02d92` |
| 23:38:15 | `develop` 第一父链的下一提交 `b301350e1f` 将版本升为 `1.2.5.1`，开始 `v1.2.5-dev` 开发 |

从该 Release 的 `Pandas_v1.2.4_20240922_213636_RE_GBK.zip` 读取九个可执行文件后，确认均包含正式提交 `2528d02d925f733eab34ebc789f8f441a1da3a25` 和指向 `D:\Jenkins\.jenkins\workspace\Pandas_master\` 的 PDB 路径；登录服等程序同时保留 `master` 构建分支字符串。各可执行文件的 ZIP CRC32 校验通过。这些制品记录补充确认了 v1.2.4 使用 Jenkins 的 master 工作区构建，再作为 Release 附件上传。

v1.2.2、v1.2.3 也采用相同的版本提交结构：

| 版本 | 正式版提交 | `develop` 上紧接的下一版升版提交 |
| --- | --- | --- |
| [v1.2.2](https://github.com/PandasWS/Pandas/releases/tag/v1.2.2) | `c5130da8b4`，2023-07-22 | `88ba4f4fe4`，升为 `1.2.3.1` |
| [v1.2.3](https://github.com/PandasWS/Pandas/releases/tag/v1.2.3) | `3f97ea37cf`，2023-12-31 | `8a9b46af33`，2024-01-15 升为 `1.2.4.1` |
| v1.2.4 | `2528d02d92`，2024-09-22 | `b301350e1f`，同日升为 `1.2.5.1` |

这三个正式提交都只有一个父提交，并同时位于 `master` 和 `develop` 的第一父链上，标签为轻量标签。历史结构符合将 `master` 快进到正式版提交的方式；未找到对应的发版 PR，现有历史未记录具体使用的 Git 命令、客户端或临时分支名称。Release 作者和附件上传者均为 `CairoLee`，仓库工具中没有自动创建 GitHub Release 的步骤。

## master 与 develop 的关系

2026-11-08 从 GitHub 核对的 `master` 为 `2528d02d92`，`develop` 为 `e6c34462cb`。两者共同祖先就是 `master`；`master` 独有提交为 0，`develop` 独有提交为 621，当前可以沿祖先关系快进。

通常 `master` 停在已发布版本，`develop` 在其后继续开发，发布时将 `master` 推进到经过验证的正式提交。`develop` 内部的功能分支和 rAthena 合并仍然可以形成合并节点；判断这两个长期分支能否快进，看的是 `master` 是否为目标提交的祖先。

历史上有一次明确的分叉：2021-01-29，`develop` 已同步新一轮 rAthena，而 `master` 从 v1.0.8 单独加入 `f074dadac2` 的手推车/邮件修复及 `0f90070be7` 的 v1.0.9 发布提交。当天通过 [`5262176872`](https://github.com/PandasWS/Pandas/commit/5262176872df86d9749b30216912f9e3c26fceec)「Merge branch 'master' into develop」合回，两侧当时分别有 144 和 2 个独有提交。因此快进是常用发布方式，分支关系仍需在每次发布前核对。

## 1. 准备本轮内容和版本号

在最新 `develop` 上整理本轮改动，记录目标版本和提交范围。版本号与更新日志可直接在 `develop` 提交；需要隔离改动或协作审查时，再使用准备分支和 PR。先完成必要的回归、数据与翻译更新，以及 [数据库升级脚本](update-database-upgrades.md)。历史上的 SQL 汇总通常在开发阶段维护：例如 v1.2.3 的汇总脚本在 `c0132788bd` 建立，发布前一晚由 `222c1bc020` 补充。

版本号批量修改工具是 [imple_versions.py](../../../tools/python/imple_versions.py)。在仓库根目录开始：

```sh
cd tools/python
pipenv install
pipenv run python imple_versions.py
```

按提示输入四段式版本。社区版约定第四段为 `1` 表示开发版，为 `0` 表示正式版：

| 目的 | 输入示例 | 程序显示 |
| --- | --- | --- |
| 开发 v1.2.5 | `1.2.5.1` | `v1.2.5-dev` |
| 发布 v1.2.5 | `1.2.5.0` | `v1.2.5` |
| 开始下一版开发 | `1.2.6.1` | `v1.2.6-dev` |

脚本修改 `src/config/pandas.hpp` 的 `Pandas_Version`，以及 `src/` 下九个 Windows `.rc` 的文件/产品数值版本和字符串版本。它直接写入匹配文件，运行后审查 diff；它不处理 composer SQL 的文件名或注释。`CHANGELOG.md`、提交、标签及 Release 由后续步骤完成。

形成正式提交前，核对本轮 `sql-files/composer/community/{main,logs,web}/` 中升级脚本的文件名、顶部目标版本注释，以及 CHANGELOG、发行版 SQL README 对这些文件的引用。社区版 SQL 沿用三段目标版本：程序 `1.2.5.1 → 1.2.5.0` 时，SQL 使用 `upgrade_to_1.2.5_*.sql`，注释写 `1.2.5`。若开发期间使用过临时版本或开发标记，按最终版本同步更名、改注释和引用，清除本轮目标版本中的四段 `.1`、`-dev` 等开发标记，避免旧临时文件一起进入运行包；保留已发布的历史升级文件。历史例子是 `07b13d8190`：计划版本由 `1.1.20` 调整为 `1.2.0` 时，同时修改了两份 composer 文件的名称和顶部注释。维护方式见 [数据库升级脚本](update-database-upgrades.md#2-同步新建库和升级路径)。

## 2. 编写更新日志并形成正式提交

按 [更新日志规范](../standards/changelog.md) 手工更新根 `CHANGELOG.md`，精选上游机制和重要内容，保留必要的升级提示。开发阶段使用“待发布”标记；确认开始执行正式发版后，在形成正式发布提交之前填写本次确认的发布日期，并核对正式版本之间的比较链接。升级提示中的 SQL 路径使用最终运行包布局。

v1.2.2–v1.2.4 的发布提交均同时修改 `CHANGELOG.md`、`pandas.hpp` 和九个 `.rc`；相应 GitHub Release 正文与 CHANGELOG 的该版内容一致。编译脚本对缺少版本日志只发出警告，因此发布准备时应主动检查完整性。

按 [提交规范](../standards/commits.md) 在 `develop` 编写本轮正式提交，例如 `chore(release): 发布 v1.2.5 正式版`，验证后正常推送并固定完整 SHA。检查远端 `master` 是该 SHA 的祖先，再用 `git merge --ff-only <正式提交SHA>` 将本地 `master` 推进到该提交并正常推送；发布步骤无需额外 PR。两条分支、构建产物和正式标签使用同一正式提交身份。若分支已分叉，先处理分叉，不使用强推或临时创建发布 merge commit 代替快进。提交、合并、推送和发布分别遵守用户授权。

## 3. 在 Windows 构建正式提交

构建机应停在上述正式提交，工作区保持干净，并使用命名分支。`Common.get_pandas_branch()` 直接读取 `repo.active_branch`，直接检出 tag 形成的 detached HEAD 会导致当前脚本失败；可在 `master` 或指向相同 SHA 的构建分支操作。

先确认 `git rev-parse HEAD` 与正式提交一致。Dawn 编译需要 Visual Studio、C++ Win32 工具链和 `Devenv`，脚本调用 `vcvarsall.bat x86` 以及 `Release|Win32` 重建。实际入口是 [dawn_compile.py](../../../tools/python/dawn_compile.py)；Jenkins 的 `Build` 阶段也调用它。

构建机的工具配置来源为 [config.yml](../../../tools/python/config.yml) 及本地环境配置。先按仓库提供的 `.secret.env.sample` 准备 `tools/python/.secret.env`，也可恢复已有文件和环境配置，只在本机保存实际值。配置模块要求启动目录中存在 `.secret.env`，或能从 sample 自动复制它。

明确设置 `PUBLISH_OUTPUT_DIRPATH`，启用符号归档时也设置 `SYMBOLS_ARCHIVE_DIRPATH` 等相关变量。路径变量的显式空值可使用脚本默认目录，缺失变量可能返回 `None` 并在调用 `len()` 时失败。

在该构建机的 `tools/python` 中准备依赖，再按顺序执行：

```sh
pipenv install
pipenv run python dawn_compile.py
pipenv run python dawn_symstore.py
pipenv run python dawn_publish.py
```

- `dawn_compile.py` 先编译 PRE 再编译 RE，注入 Git 分支和 SHA，保留两套二进制。手工运行遇到脏工作区会询问，Jenkins 路径直接终止。
- `dawn_symstore.py` 是可选的符号归档步骤。它保存 PE/PDB 并按配置同步符号服务；本地会询问归档和 OSS 同步，Jenkins 自动执行。启用时检查归档与同步日志。
- `dawn_publish.py` 从已提交的 `HEAD` 导出数据，结合本地二进制，完成本地化、SQL 重排和 ZIP 压缩。编译到打包之间保持同一提交。

### 使用已有 Jenkins 任务

[Jenkinsfile](../../../Jenkinsfile) 的步骤是准备环境与 Pipenv → Build → 可选的符号归档和打包 → 归档制品。运行时核对实际 checkout 的 SHA，并按需要打开：

- `publish_package=true`：按 `compile_mode` 和 `publish_lang` 打包，社区版默认四个运行包。
- `archive_symbols=true`：归档符号并按配置同步 OSS。

两个参数默认均为 `false`。任务通过 `JENKINS_ENVIRONMENT_FILE` 加载构建环境配置，包放入 `artifacts/packages`，符号放入 `artifacts/symbols`，最后归档 `artifacts/**`。任务中没有创建 GitHub Release 的调用。

旧发布包已确认使用过 `Pandas_master` 构建工作区。本次调查未连接旧 Jenkins 实例；现存 job、具体构建记录及环境配置需在构建机恢复发布时核对。

## 4. 验收四个运行包

社区版默认组合为 `RE_GBK`、`RE_BIG5`、`PRE_GBK`、`PRE_BIG5`。文件名形如：

```text
Pandas_v1.2.5_YYYYMMDD_HHMMSS_RE_GBK.zip
Pandas_v1.2.5_YYYYMMDD_HHMMSS_RE_BIG5.zip
Pandas_v1.2.5_YYYYMMDD_HHMMSS_PRE_GBK.zip
Pandas_v1.2.5_YYYYMMDD_HHMMSS_PRE_BIG5.zip
```

不同组合单独取时间戳。配置输出路径为空时，默认目录是仓库相邻的 `../Release/Pandas/v1.2.5/`，同时保留用于核验的同名解包目录。语言档名称与文件实际编码的关系见 [发布流程](../architecture/release-pipeline.md)。

检查每个包的版本、模式、语言、二进制和更新日志，确认 SQL 新建/升级路径齐全；逐包复核本轮升级 SQL 的文件名、顶部目标版本和日志引用，确保使用正式三段版本且没有遗留临时文件。在相应测试环境验证启动和关键场景，保留与发布二进制匹配的符号。当前开发文档规则还要求包中排除根 `AGENTS.md` 与 `doc/development/`。

## 5. 创建 GitHub Release 并上传运行包

已核对的 v1.2.2–v1.2.4 均以 `vX.Y.Z` 为 tag 和 Release 标题，以 `master` 为目标，附带上述四个 ZIP。恢复发布时可通过仓库的 Releases → Draft a new release：

1. 选择已经指向正式提交的 tag；若创建新 tag，确认目标 `master` 的 SHA 就是本次构建提交。
2. 标题填写 `vX.Y.Z`，正文使用 `CHANGELOG.md` 的本版内容。
3. 上传四个已验收的 ZIP，核对文件名和版本。
4. 检查正式版的发布属性，完成审阅后执行 Publish release。

网页操作依据 [GitHub Release 文档](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository)。保存草稿、创建 tag 和公开发布均按本次授权执行。仓库生成的四个运行包需要作为附件上传；GitHub 提供的源码归档是单独的下载项。

## 6. 开启下一轮开发

确认正式提交及其制品已固定后，在 `develop` 再执行 `imple_versions.py`，将下一版设为 `X.Y.(Z+1).1`，或采用本轮已经决定的下一目标版本。直接在 `develop` 单独提交并正常推送升版，例如 `chore(release): 开始 v1.2.6-dev 开发`，按用户授权执行，无需专门创建升版 PR。`master` 和正式 tag 继续保留在已发布提交。

v1.2.4 的顺序最直观：`1.2.4.0` 正式提交 → 构包与 Release → `develop` 升为 `1.2.5.1`。v1.2.2 则在 GitHub Release 公开发布前已经提交下一版开发号，因此制品阶段应明确使用正式提交，避免随开发分支继续移动。

## 当前起点

2026-11-08 核对时，远端 `master` 和最新 Release 仍为 `v1.2.4` 的 `2528d02d92`，`develop` 为 `e6c34462cb`，版本字段为 `1.2.5.1`；`CHANGELOG.md` 最新条目仍为 v1.2.4。`composer/community/main/` 已有 v1.2.5 的主库和 `use_sql_db` 升级文件，应继续核对本轮全部 schema 差异后再正式发布。
