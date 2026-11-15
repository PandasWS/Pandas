"""验证发布转译的实际 YAML 读取、替换和保存路径，不运行发布初始化。"""

import ast
from functools import partial
import os
from pathlib import Path
import re
import subprocess
import tempfile
from types import SimpleNamespace
import unittest

import chardet
from ruamel.yaml import YAML, scalarstring
from ruamel.yaml.constructor import DuplicateKeyError
from ruamel.yaml.reader import ReaderError


REPOSITORY = Path(__file__).resolve().parents[3]
SCRIPTS = REPOSITORY / "tools/python"


def load_definitions(path, names, namespace):
    # 模块顶层会安装依赖、读取发布配置并切换目录，只加载实际函数与类。
    tree = ast.parse(path.read_text(encoding="utf-8-sig"))
    definitions = [
        node for node in tree.body
        if isinstance(node, (ast.FunctionDef, ast.ClassDef)) and node.name in names
    ]
    if {node.name for node in definitions} != set(names):
        raise AssertionError("转译入口发生变化，请重新核对测试加载范围")
    exec(compile(ast.Module(body=definitions, type_ignores=[]), str(path), "exec"), namespace)
    return namespace


def fail_translation(message):
    raise AssertionError(message)


def load_translation():
    common = load_definitions(
        SCRIPTS / "libs/Common.py",
        {"is_file_exists", "encoding_maps", "get_file_encoding"},
        {"os": os, "chardet": chardet, "partial": partial},
    )
    return load_definitions(
        SCRIPTS / "imple_translate.py",
        {"TranslateDatabase", "YamlReplaceController", "YamlDoubleQuotedHandling"},
        {
            "os": os, "YAML": YAML, "scalarstring": scalarstring,
            "Common": SimpleNamespace(**common),
            "Message": SimpleNamespace(ShowError=fail_translation),
        },
    )


class TranslationTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.translation = load_translation()
        tree = ast.parse((SCRIPTS / "imple_translate.py").read_text(encoding="utf-8-sig"))
        configuration = next(
            node for node in tree.body
            if isinstance(node, ast.Assign)
            and any(isinstance(target, ast.Name) and target.id == "configures" for target in node.targets)
        )
        namespace = {"re": re}
        exec(compile(ast.Module(body=[configuration], type_ignores=[]), "configures", "exec"), namespace)
        cls.configures = namespace["configures"]
        cls.yaml_configures = [v for v in cls.configures if v["operate"] == "YamlReplaceController"]
        if not cls.yaml_configures:
            raise AssertionError("没有找到 YAML 转译配置")

    def controller(self, config=None, lang="zh-cn"):
        config = config or self.yaml_configures[0]
        previous = Path.cwd()
        try:
            # 实际 TranslateDatabase 按辅助脚本工作目录读取仓库中的对照表。
            os.chdir(SCRIPTS)
            return self.translation[config["operate"]](
                **config["operate_params"], lang=lang, silent=True, project_dir=str(REPOSITORY),
            )
        finally:
            os.chdir(previous)

    def load(self, path):
        return self.controller()._YamlReplaceController__load(path)


class YamlEncodingTest(TranslationTestCase):
    def test_release_item_names_fit_runtime_encodings(self):
        contents = self.load(REPOSITORY / "db/re/item_db_etc.yml")
        item = next(item for item in contents["Body"] if item["Id"] == 311366)
        self.assertEqual(item["Name"], "Movement Speed Increase")
        item = next(item for item in contents["Body"] if item["Id"] == 300598)
        self.assertEqual(item["Name"], "Gunter Card")

    def test_unicode_without_bom_survives_read_translate_write(self):
        # 大片 ASCII 后出现 Unicode，覆盖分块猜编码漏掉后部文本的场景。
        content = "# ASCII padding\n" * 6000 + (
            "Body:\n  - Id: 501\n    Name: Red Potion\n"
            "  - Id: 99999999\n    Name: 'Movement Speed Increase 中文 ’ → Ⅱ'\n"
        )
        with tempfile.TemporaryDirectory(prefix="pandas-yaml-") as directory:
            source = Path(directory) / "source.yml"
            output = Path(directory) / "translated.yml"
            source.write_text(content, encoding="utf-8")
            for lang, expected in (("zh-cn", "红色药水"), ("zh-tw", "紅色藥水")):
                with self.subTest(lang=lang):
                    self.assertTrue(self.controller(lang=lang).execute(source, output))
                    self.assertTrue(output.read_bytes().startswith(b"\xef\xbb\xbf"))
                    actual = self.load(output)["Body"]
                    self.assertEqual(actual[0]["Name"], expected)
                    self.assertEqual(actual[1]["Name"], "Movement Speed Increase 中文 ’ → Ⅱ")
            self.assertEqual(source.read_text(encoding="utf-8"), content)

    def test_unrepresentable_data_fails_before_saving(self):
        with tempfile.TemporaryDirectory(prefix="pandas-yaml-") as directory:
            source = Path(directory) / "source.yml"
            output = Path(directory) / "translated.yml"
            for lang in ("zh-cn", "zh-tw"):
                for field in ("Name", "Script"):
                    with self.subTest(lang=lang, field=field):
                        text = "Body:\n  - Id: 99999999\n    %s: 'Bad\u200bdata'\n" % field
                        source.write_text(text, encoding="utf-8")
                        with self.assertRaisesRegex(ValueError, r"source.yml: Body\[Id=99999999\]\." + field + r".*U\+200B"):
                            self.controller(lang=lang).execute(source, output)
                        self.assertFalse(output.exists())
                        self.assertEqual(source.read_text(encoding="utf-8"), text)

    def test_yaml_unicode_boms(self):
        with tempfile.TemporaryDirectory(prefix="pandas-yaml-") as directory:
            source = Path(directory) / "source.yml"
            for encoding in ("utf-8-sig", "utf-16"):
                with self.subTest(encoding=encoding):
                    source.write_text("Name: 中文\n", encoding=encoding)
                    self.assertEqual(self.load(source)["Name"], "中文")

    def test_target_codepage_is_selected_by_language(self):
        with tempfile.TemporaryDirectory(prefix="pandas-yaml-") as directory:
            source = Path(directory) / "source.yml"
            output = Path(directory) / "translated.yml"
            source.write_text("Body:\n  - Id: 99999999\n    Name: Günter\n", encoding="utf-8")
            self.assertTrue(self.controller(lang="zh-cn").execute(source, output))
            previous = output.read_bytes()
            with self.assertRaisesRegex(ValueError, r"BIG5.*U\+00FC"):
                self.controller(lang="zh-tw").execute(source, output)
            self.assertEqual(output.read_bytes(), previous)

    def test_invalid_bytes_are_rejected(self):
        with tempfile.TemporaryDirectory(prefix="pandas-yaml-") as directory:
            source = Path(directory) / "invalid.yml"
            source.write_bytes(b"Name: \xff\n")
            with self.assertRaises(ReaderError):
                self.load(source)

    def test_duplicate_keys_are_rejected(self):
        with tempfile.TemporaryDirectory(prefix="pandas-yaml-") as directory:
            source = Path(directory) / "duplicate.yml"
            source.write_text("Trade:\n  NoDrop: true\nTrade:\n  NoSell: true\n", encoding="utf-8")
            with self.assertRaises(DuplicateKeyError):
                self.load(source)


class RepositoryYamlTest(TranslationTestCase):
    def test_tracked_yaml_uses_release_loader(self):
        filenames = subprocess.check_output(
            ["git", "ls-files", "-z", "*.yml", "*.yaml"], cwd=REPOSITORY,
        ).decode("utf-8").split("\0")
        paths = sorted(Path(name) for name in filenames if name and "3rdparty" not in Path(name).parts)
        self.assertTrue(paths, "没有找到待检查的 YAML 文件")
        controller = self.controller()
        print(f"通过实际发布入口检查 {len(paths)} 个 YAML 文件", flush=True)
        for path in paths:
            with self.subTest(path=path.as_posix()):
                controller._YamlReplaceController__load(REPOSITORY / path)

    def test_configured_yaml_translations(self):
        # 按真实配置覆盖 PRE/RE 数据与两种语言，实际写入临时输出再解析核对。
        with tempfile.TemporaryDirectory(prefix="pandas-yaml-release-") as directory:
            output = Path(directory) / "translated.yml"
            for config in self.yaml_configures:
                params = config["operate_params"]
                paths = set(REPOSITORY / name for name in config.get("filepath", []))
                for pattern in config.get("globpath", []):
                    paths.update(REPOSITORY.glob(pattern))
                paths = sorted(path for path in paths if path.is_file() and not any(
                    exclude in path.relative_to(REPOSITORY).as_posix()
                    for exclude in config.get("globexclude", [])
                ))
                self.assertTrue(paths, config)
                controllers = {lang: self.controller(config, lang) for lang in ("zh-cn", "zh-tw")}
                for path in paths:
                    # 独立读取原始字节作为内容基线，避免读写两端同样误解码却互相通过。
                    original = YAML(typ="safe").load(path.read_bytes())
                    for lang, controller in controllers.items():
                        with self.subTest(path=path.relative_to(REPOSITORY).as_posix(), lang=lang):
                            print(f"转译 {lang}: {path.relative_to(REPOSITORY)}", flush=True)
                            self.assertTrue(controller.execute(path, output))
                            self.assertTrue(output.read_bytes().startswith(b"\xef\xbb\xbf"))
                            actual = YAML(typ="safe").load(output.read_bytes())
                            self.assertEqual(len(original["Body"]), len(actual["Body"]))
                            translated = 0
                            for before, after in zip(original["Body"], actual["Body"]):
                                identifier = before[params["id_field"]]
                                name = controller._YamlReplaceController__trans.trans(identifier)
                                target = params["target_field"]
                                expected = dict(before)
                                if name:
                                    expected[target] = name
                                    translated += 1
                                self.assertEqual(dict(after), expected, f"{path}: {identifier}")
                            self.assertGreater(translated, 0, "没有替换任何已知条目")


if __name__ == "__main__":
    unittest.main()
