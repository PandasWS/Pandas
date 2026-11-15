"""通过命令行验证临时仓库中的版本读取、校验与写入。"""

import codecs
from contextlib import redirect_stdout
import io
import json
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


REPOSITORY = Path(__file__).resolve().parents[3]
SCRIPT = Path("tools/python/imple_versions.py")
COMMUNITY_HEADER = Path("src/config/pandas.hpp")
PROFESSIONAL_HEADER = Path("src/config/professional.hpp")
RESOURCE_FILES = tuple(sorted(
    path.relative_to(REPOSITORY) for path in (REPOSITORY / "src").rglob("*.rc")
))


def resource_versions(text, old, new):
    # 只修改真实样本中的四处字段，保留注释与其他出现旧版本的字节。
    for field in ("FILEVERSION", "PRODUCTVERSION"):
        text = text.replace(
            f" {field} {old.replace('.', ',')}\r\n",
            f" {field} {new.replace('.', ',')}\r\n",
        )
    for field in ("FileVersion", "ProductVersion"):
        text = text.replace(f'VALUE "{field}", "{old}"', f'VALUE "{field}", "{new}"')
    return text


class VersionsCLITest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="pandas-versions-")
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name).resolve()
        self.repository = root / "project"
        self.cwd = root / "cwd"
        self.cwd.mkdir()
        for relative in (SCRIPT, COMMUNITY_HEADER, *RESOURCE_FILES):
            destination = self.repository / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(REPOSITORY / relative, destination)

        header = (self.repository / COMMUNITY_HEADER).read_text(encoding="utf-8-sig")
        self.current = header.split('#define Pandas_Version "', 1)[1].split('"', 1)[0]
        self.target = "65535.0.7.2" if self.current != "65535.0.7.2" else "1.2.5.1"

    def snapshot(self):
        result = {}
        for path in sorted(self.repository.rglob("*")):
            if path.is_file():
                stat = path.stat()
                result[path.relative_to(self.repository)] = (
                    path.read_bytes(), stat.st_mtime_ns, stat.st_ino,
                )
        return result

    def invoke(self, *arguments, exit_code=0, stdin=subprocess.DEVNULL, io_encoding="utf-8"):
        result = subprocess.run(
            [sys.executable, "-S", str(self.repository / SCRIPT), *arguments],
            cwd=self.cwd,
            stdin=stdin,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding=io_encoding,
            env=dict(os.environ, PYTHONIOENCODING=io_encoding),
            timeout=10,
        )
        self.assertEqual(result.returncode, exit_code, (result.stdout, result.stderr))
        if "--json" in arguments and "--help" not in arguments:
            self.assertEqual(result.stderr, "")
            payload = json.loads(result.stdout)
            self.assertIsInstance(payload, dict)
            self.assertIs(payload["ok"], exit_code == 0)
            return payload
        return result

    def test_help_show_and_check_do_not_write(self):
        before = self.snapshot()
        help_result = self.invoke("--help")
        for command in ("show", "check", "set"):
            self.assertIn(command, help_result.stdout)

        parts = self.current.split(".")
        display_version = "v" + ".".join(parts[:3]) + ("-dev" if parts[3] == "1" else "")
        for command, arguments in (("show", ("--json", "show")), ("check", ("check", "--json"))):
            with self.subTest(command=command):
                payload = self.invoke(*arguments)
                self.assertEqual(payload["command"], command)
                self.assertEqual(payload["edition"], "community")
                self.assertEqual(payload["version"], self.current)
                self.assertEqual(payload["display_version"], display_version)
                self.assertIs(payload["consistent"], True)
                self.assertIsInstance(payload["files"], list)
                self.assertEqual(len(payload["files"]), 1 + len(RESOURCE_FILES))
        self.assertEqual(self.snapshot(), before)

    def test_dry_run_reports_changes_without_writing(self):
        before = self.snapshot()
        payload = self.invoke("set", self.target, "--expect-current", self.current, "--dry-run", "--json")
        self.assertEqual(payload["command"], "set")
        self.assertEqual(payload["edition"], "community")
        self.assertEqual(payload["target_version"], self.target)
        self.assertIs(payload["dry_run"], True)
        self.assertEqual(payload["changed_files"], [])
        paths = {Path(change["path"]) for change in payload["planned_changes"]}
        self.assertEqual(paths, {COMMUNITY_HEADER, *RESOURCE_FILES})
        for change in payload["planned_changes"]:
            self.assertIsInstance(change["from"], list)
            self.assertEqual(set(change["from"]), {self.current})
            self.assertEqual(change["to"], self.target)
        self.assertEqual(self.snapshot(), before)

    def test_set_preserves_unrelated_bytes_and_file_encodings(self):
        original = (self.repository / RESOURCE_FILES[0]).read_bytes().decode("utf-16")
        comment = (
            f'\r\n// FILEVERSION {self.current.replace(".", ",")} 保留原说明\r\n'
            f'// VALUE "FileVersion", "{self.current}"\r\n'
        )
        expected = resource_versions(original, self.current, self.target) + comment
        extra_resources = (
            (Path("src/extra-utf16be.rc"), codecs.BOM_UTF16_BE, "utf-16-be"),
            (Path("src/extra-utf8-bom.rc"), codecs.BOM_UTF8, "utf-8"),
            (Path("src/extra-utf8.rc"), b"", "utf-8"),
        )
        for path, bom, encoding in extra_resources:
            (self.repository / path).write_bytes(bom + (original + comment).encode(encoding))

        before = self.snapshot()
        payload = self.invoke("--json", "set", self.target)
        expected_paths = {COMMUNITY_HEADER, *RESOURCE_FILES, *(path for path, _, _ in extra_resources)}
        self.assertEqual({Path(path) for path in payload["changed_files"]}, expected_paths)
        self.assertIs(payload["dry_run"], False)
        self.assertEqual(self.snapshot().keys(), before.keys())
        self.assertEqual(
            (self.repository / COMMUNITY_HEADER).read_bytes(),
            before[COMMUNITY_HEADER][0].replace(
                f'#define Pandas_Version "{self.current}"'.encode("ascii"),
                f'#define Pandas_Version "{self.target}"'.encode("ascii"),
            ),
        )
        for path in RESOURCE_FILES:
            with self.subTest(path=path):
                text = before[path][0].decode("utf-16")
                updated = resource_versions(text, self.current, self.target)
                self.assertNotEqual(updated, text)
                self.assertEqual(
                    (self.repository / path).read_bytes(),
                    codecs.BOM_UTF16_LE + updated.encode("utf-16-le"),
                )
        for path, bom, encoding in extra_resources:
            with self.subTest(path=path):
                self.assertEqual((self.repository / path).read_bytes(), bom + expected.encode(encoding))
        self.assertEqual(self.invoke("check", "--json")["version"], self.target)

    def test_set_with_current_version_does_not_rewrite(self):
        before = self.snapshot()
        payload = self.invoke("set", self.current, "--json")
        self.assertEqual(payload["changed_files"], [])
        self.assertEqual(payload["planned_changes"], [])
        self.assertEqual(self.snapshot(), before)

    def test_display_preserves_existing_version_string_rules(self):
        # 与发布工具 Common.get_community_ver/get_commercial_ver 的展示约定一致。
        header = self.repository / COMMUNITY_HEADER
        header.write_bytes(header.read_bytes().replace(
            f'#define Pandas_Version "{self.current}"'.encode("ascii"),
            b'#define Pandas_Version "1.02.3.01"',
        ))
        self.assertEqual(self.invoke("show", "--json")["display_version"], "v1.02.3")
        professional = self.repository / PROFESSIONAL_HEADER
        professional.write_bytes(b'#define Pandas_Commercial_Version "2026.01.02.00"\n')
        self.assertEqual(self.invoke("show", "--json")["display_version"], "v2026.01.02 Rev.00")

    def test_invalid_arguments_do_not_write(self):
        before = self.snapshot()
        self.invoke(exit_code=2)
        for arguments in (
            ("unknown",), ("show", "--unknown"), ("set",),
            ("set", "1.2.3"), ("set", "1.2.3.65536"), ("set", "1.2.3.１"),
            ("set", self.target, "--expect-current", "1.2.3.-1"),
        ):
            with self.subTest(arguments=arguments):
                self.assertTrue(self.invoke("--json", *arguments, exit_code=2)["error"])
        self.assertEqual(self.snapshot(), before)

    def test_json_errors_support_ascii_output(self):
        payload = self.invoke("set", "1.2.3", "--json", exit_code=2, io_encoding="ascii")
        self.assertIn("四段", payload["error"])

    @unittest.skipUnless(hasattr(os, "openpty"), "系统未提供伪终端")
    def test_no_arguments_accepts_version_from_terminal(self):
        master, terminal = os.openpty()
        self.addCleanup(os.close, master)
        self.addCleanup(os.close, terminal)
        os.write(master, (self.target + "\n").encode("ascii"))
        self.invoke(stdin=terminal)
        self.assertEqual(self.invoke("check", "--json")["version"], self.target)

    def test_expect_current_mismatch_does_not_write(self):
        before = self.snapshot()
        payload = self.invoke("set", self.target, "--expect-current", self.target, "--json", exit_code=1)
        self.assertTrue(payload["error"])
        self.assertEqual(self.snapshot(), before)

    def test_missing_or_duplicate_fields_fail_before_writing(self):
        path = self.repository / RESOURCE_FILES[-1]
        original = path.read_bytes()
        field = f'VALUE "ProductVersion", "{self.current}"'.encode("utf-16-le")
        self.assertEqual(original.count(field), 1)
        duplicate_field = field + "\r\n            ".encode("utf-16-le") + field
        for malformed in (original.replace(field, b""), original.replace(field, duplicate_field)):
            with self.subTest(field_count=malformed.count(field)):
                path.write_bytes(malformed)
                before = self.snapshot()
                self.assertTrue(self.invoke("set", self.target, "--json", exit_code=1)["error"])
                self.assertEqual(self.snapshot(), before)

    def test_missing_required_resource_fails_before_writing(self):
        (self.repository / RESOURCE_FILES[-1]).unlink()
        before = self.snapshot()
        self.assertTrue(self.invoke("set", self.target, "--json", exit_code=1)["error"])
        self.assertEqual(self.snapshot(), before)

    def test_numeric_resource_version_requires_commas(self):
        path = self.repository / RESOURCE_FILES[-1]
        original_field = f" FILEVERSION {self.current.replace('.', ',')}".encode("utf-16-le")
        malformed_field = f" FILEVERSION {self.current}".encode("utf-16-le")
        original = path.read_bytes()
        self.assertEqual(original.count(original_field), 1)
        path.write_bytes(original.replace(original_field, malformed_field))
        before = self.snapshot()
        for arguments in (("check",), ("set", self.target)):
            with self.subTest(arguments=arguments):
                self.assertTrue(self.invoke(*arguments, "--json", exit_code=1)["error"])
        self.assertEqual(self.snapshot(), before)

    def test_replace_failure_restores_original_files(self):
        before = {path: data[0] for path, data in self.snapshot().items()}
        main = runpy.run_path(str(self.repository / SCRIPT))["main"]
        replace = os.replace
        destinations = []

        def fail_second_replace(source, destination):
            destinations.append(Path(destination))
            if len(destinations) == 2:
                first = destinations[0]
                self.assertNotEqual(first.read_bytes(), before[first.relative_to(self.repository)])
                raise OSError("模拟写入失败")
            return replace(source, destination)

        stdout = io.StringIO()
        with patch("os.replace", side_effect=fail_second_replace), redirect_stdout(stdout):
            self.assertEqual(main(["set", self.target, "--json"]), 1)
        payload = json.loads(stdout.getvalue())
        self.assertIs(payload["ok"], False)
        self.assertIn("模拟写入失败", payload["error"])
        self.assertEqual({path: data[0] for path, data in self.snapshot().items()}, before)

    def test_check_detects_single_field_mismatch_and_set_repairs_it(self):
        path = self.repository / RESOURCE_FILES[-1]
        original = path.read_bytes()
        path.write_bytes(original.replace(
            f'VALUE "ProductVersion", "{self.current}"'.encode("utf-16-le"),
            f'VALUE "ProductVersion", "{self.target}"'.encode("utf-16-le"),
        ))
        before = self.snapshot()
        self.assertIs(self.invoke("show", "--json")["consistent"], False)
        payload = self.invoke("check", "--json", exit_code=1, io_encoding="ascii")
        self.assertIs(payload["consistent"], False)
        self.assertEqual(payload["version"], self.current)
        self.assertTrue(self.invoke(
            "set", self.current, "--expect-current", self.current, "--json", exit_code=1,
        )["error"])
        self.assertEqual(self.snapshot(), before)

        payload = self.invoke("set", self.current, "--json")
        self.assertEqual([Path(item) for item in payload["changed_files"]], [RESOURCE_FILES[-1]])
        self.assertEqual(path.read_bytes(), original)
        after = self.snapshot()
        for relative in before.keys() - {RESOURCE_FILES[-1]}:
            self.assertEqual(after[relative], before[relative])
        self.assertIs(self.invoke("check", "--json")["consistent"], True)

    def test_professional_revision_updates_resources_and_leaves_community_header(self):
        old_version = "2026.11.15.2"
        target_version = "2026.11.15.3"
        header = self.repository / PROFESSIONAL_HEADER
        definition = f'#define Pandas_Commercial_Version "{old_version}"\r\n'
        header.write_bytes(codecs.BOM_UTF8 + definition.encode("ascii"))
        for path in RESOURCE_FILES:
            resource = self.repository / path
            text = resource.read_bytes().decode("utf-16")
            text = resource_versions(text, self.current, old_version)
            resource.write_bytes(codecs.BOM_UTF16_LE + text.encode("utf-16-le"))

        before = self.snapshot()
        payload = self.invoke("show", "--json")
        self.assertEqual(payload["edition"], "professional")
        self.assertEqual(payload["display_version"], "v2026.11.15 Rev.2")
        self.assertEqual(payload["version"], old_version)
        self.assertIs(payload["consistent"], True)

        result = self.invoke("set", target_version, "--expect-current", old_version, "--json")
        changed_paths = {Path(path) for path in result["changed_files"]}
        self.assertEqual(changed_paths, {PROFESSIONAL_HEADER, *RESOURCE_FILES})
        after = self.snapshot()
        self.assertEqual(after[COMMUNITY_HEADER], before[COMMUNITY_HEADER])
        expected = definition.replace(old_version, target_version).encode("ascii")
        self.assertEqual(after[PROFESSIONAL_HEADER][0], codecs.BOM_UTF8 + expected)
        self.assertEqual(self.invoke("check", "--json")["display_version"], "v2026.11.15 Rev.3")


if __name__ == "__main__":
    unittest.main(verbosity=2)
