"""使用发布转译的 YAML 解析器检查仓库数据，不运行发布初始化。"""

from pathlib import Path
import subprocess
import unittest

from ruamel.yaml import YAML


REPOSITORY = Path(__file__).resolve().parents[3]


class RepositoryYamlTest(unittest.TestCase):
    def test_tracked_yaml_has_valid_syntax_and_unique_keys(self):
        filenames = subprocess.check_output(
            ["git", "ls-files", "-z", "*.yml", "*.yaml"],
            cwd=REPOSITORY,
        ).decode("utf-8").split("\0")
        paths = sorted(
            Path(name) for name in filenames
            if name and "3rdparty" not in Path(name).parts
        )
        self.assertTrue(paths, "没有找到待检查的 YAML 文件")
        print(f"检查 {len(paths)} 个 YAML 文件", flush=True)
        for path in paths:
            with self.subTest(path=path.as_posix()):
                parser = YAML(typ="rt")
                parser.preserve_quotes = True
                with (REPOSITORY / path).open(encoding="utf-8-sig") as stream:
                    parser.load(stream)


if __name__ == "__main__":
    unittest.main()
