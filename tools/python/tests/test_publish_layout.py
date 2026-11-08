"""在临时目录验证 Dawn 公共整理规则，不运行发布初始化或访问外部服务。"""

import ast
import glob
import os
from pathlib import Path
import shutil
import tempfile
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
PUBLISH_SCRIPT = REPOSITORY / "tools/python/dawn_publish.py"


def load_arranger(is_commercial):
    # 发布模块顶层会初始化依赖、读取配置并切换目录，故只加载所需函数。
    names = {
        "rmdir", "enum_files", "remove_files", "copyfile", "remove_file",
        "recursive_overwrite", "arrange_common",
    }
    tree = ast.parse(PUBLISH_SCRIPT.read_text(encoding="utf-8-sig"))
    functions = [
        node for node in tree.body
        if isinstance(node, ast.FunctionDef) and node.name in names
    ]
    if {node.name for node in functions} != names:
        raise AssertionError("发布整理函数发生变化，请重新核对测试加载入口")
    module = ast.Module(body=functions, type_ignores=[])
    namespace = {
        "os": os, "glob": glob, "shutil": shutil,
        "is_commercial": is_commercial,
    }
    exec(compile(module, str(PUBLISH_SCRIPT), "exec"), namespace)
    return namespace["arrange_common"]


class PublishLayoutTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="pandas-publish-layout-")
        self.addCleanup(temporary.cleanup)
        self.package = Path(temporary.name)
        for relative in ("sql-files", "tools/python/dist", "tools/batches"):
            shutil.copytree(REPOSITORY / relative, self.package / relative)
        shutil.copyfile(REPOSITORY / "CHANGELOG.md", self.package / "CHANGELOG.md")
        self.write("doc/user-guide.txt", "用户文档应保留\n")
        self.write("tools/python/dist/doc/release-guide.txt", "发行覆盖文档应保留\n")
        # 专业版使用独立哨兵，验证分支选择，而非专业版 SQL 的业务内容。
        for database in ("main", "logs", "web"):
            self.write(
                f"sql-files/composer/professional/{database}/edition-probe.sql",
                f"-- professional {database}\n",
            )

    def write(self, relative, content):
        path = self.package / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def add_development_docs(self, prefix=""):
        self.write(prefix + "AGENTS.md", "开发入口\n")
        self.write(prefix + "doc/development/README.md", "开发导航\n")
        self.write(prefix + "doc/development/how-to/nested/task.md", "任务指南\n")

    def check_layout(self, is_commercial=False):
        edition = "professional" if is_commercial else "community"
        expected_upgrades = {}
        for database in ("main", "logs", "web"):
            source = self.package / f"sql-files/composer/{edition}/{database}"
            expected_upgrades[database] = {
                path.relative_to(source): path.read_bytes()
                for path in source.rglob("*") if path.is_file()
            }

        load_arranger(is_commercial)(str(self.package) + os.sep)

        self.assertFalse((self.package / "AGENTS.md").exists())
        self.assertFalse((self.package / "doc/development").exists())
        self.assertEqual(
            (self.package / "doc/user-guide.txt").read_text(encoding="utf-8"),
            "用户文档应保留\n",
        )
        self.assertTrue((self.package / "doc/release-guide.txt").is_file())
        for database in ("main", "logs", "web"):
            creation = self.package / f"sql-files/{database}/creation/01.{database}.sql"
            self.assertEqual(creation.read_bytes(), (REPOSITORY / f"sql-files/{database}.sql").read_bytes())
            destination = self.package / f"sql-files/{database}/upgrades"
            actual = {
                path.relative_to(destination): path.read_bytes()
                for path in destination.rglob("*") if path.is_file()
            }
            self.assertEqual(actual, expected_upgrades[database])
        for mode in ("pre-renewal", "renewal"):
            self.assertTrue((self.package / f"sql-files/main/creation/use_sql_db/{mode}/optional").is_dir())
        self.assertTrue((self.package / "sql-files/main/creation/optional/premium_storage.sql").is_file())
        for removed in ("composer", "compatibility", "upgrades", "tools"):
            self.assertFalse((self.package / "sql-files" / removed).exists())

    def test_community_package_excludes_development_docs(self):
        self.add_development_docs()
        self.check_layout()

    def test_professional_package_excludes_development_docs(self):
        self.add_development_docs()
        self.check_layout(is_commercial=True)

    def test_dist_overlay_cannot_reintroduce_development_docs(self):
        self.add_development_docs("tools/python/dist/")
        self.check_layout()

    def test_package_without_development_docs_still_arranges(self):
        self.check_layout()


if __name__ == "__main__":
    unittest.main(verbosity=2)
