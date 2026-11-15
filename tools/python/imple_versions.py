'''
//===== Pandas Python Script =================================
//= 版本号批量修改辅助脚本
//===== By: ==================================================
//= Sola丶小克
//===== Current Version: =====================================
//= 1.1
//===== Description: =========================================
//= 此脚本用于查看、检查和快速修改当前工程的版本号
//===== Additional Comments: =================================
//= 1.0 首个版本. [Sola丶小克]
//= 1.1 支持命令行查看、检查、预览与批量修改版本号.
//============================================================
'''

# -*- coding: utf-8 -*-

import argparse
import codecs
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import stat
import sys
import tempfile


PROJECT_ROOT = Path(__file__).resolve().parents[2]
COMMUNITY_HEADER = "src/config/pandas.hpp"
PROFESSIONAL_HEADER = "src/config/professional.hpp"
REQUIRED_RESOURCES = (
    "src/char/char-server.rc",
    "src/login/login-server.rc",
    "src/map/map-server.rc",
    "src/map/map-server-generator.rc",
    "src/web/web-server.rc",
    "src/tool/csv2yaml.rc",
    "src/tool/mapcache.rc",
    "src/tool/yaml2sql.rc",
    "src/tool/yamlupgrade.rc",
)
RESOURCE_FIELDS = (
    ("FILEVERSION", r'^[ \t]*FILEVERSION[ \t]+(?P<value>[^\r\n/]+)', ","),
    ("PRODUCTVERSION", r'^[ \t]*PRODUCTVERSION[ \t]+(?P<value>[^\r\n/]+)', ","),
    ("FileVersion", r'^[ \t]*VALUE[ \t]+"FileVersion"[ \t]*,[ \t]*"(?P<value>[^"\r\n]*)"', "."),
    ("ProductVersion", r'^[ \t]*VALUE[ \t]+"ProductVersion"[ \t]*,[ \t]*"(?P<value>[^"\r\n]*)"', "."),
)


class VersionError(Exception):
    """版本数据、项目布局或文件操作有误。"""


class UsageError(Exception):
    """命令行参数有误。"""


class ArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        raise UsageError(message)


def validate_version(value):
    # Windows VERSIONINFO 的四个分量均为 16 位无符号整数。
    if not re.fullmatch(r"[0-9]+(?:\.[0-9]+){3}", value):
        raise ValueError("版本号须为四段 ASCII 整数，例如 1.2.5.0")
    if any(len(part.lstrip("0")) > 5 or int(part) > 65535 for part in value.split(".")):
        raise ValueError("版本号每一段须在 0 到 65535 之间")
    return value


def version_argument(value):
    try:
        return validate_version(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError(str(error)) from error


def display_version(value, edition):
    fields = value.split(".")
    result = "v" + ".".join(fields[:3])
    revision = fields[3]
    if edition == "professional":
        return result + (f" Rev.{revision}" if revision != "0" else "")
    return result + ("-dev" if revision == "1" else "")


def header_field(name):
    return ((name, rf'^[ \t]*#[ \t]*define[ \t]+{name}[ \t]+"(?P<value>[^"\r\n]*)"', "."),)


@dataclass
class VersionFile:
    path: Path
    relative: str
    original: bytes
    text: str
    encoding: str
    bom: bytes
    fields: list
    versions: dict
    active: bool = True

    def render(self, version):
        text = self.text
        for start, end, separator in sorted(self.fields, reverse=True):
            text = text[:start] + version.replace(".", separator) + text[end:]
        return self.bom + text.encode(self.encoding)


def read_version_file(root, relative, rules, active=True):
    path = root / relative
    if path.is_symlink():
        raise VersionError(f"版本文件不能是符号链接: {relative}")
    original = path.read_bytes()
    bom, encoding = b"", "latin-1"
    # 无 BOM 文件按单字节映射处理，版本字段为 ASCII，其他字节可原样往返。
    for marker, codec in ((codecs.BOM_UTF8, "utf-8"),
                          (codecs.BOM_UTF16_LE, "utf-16-le"),
                          (codecs.BOM_UTF16_BE, "utf-16-be")):
        if original.startswith(marker):
            bom, encoding = marker, codec
            break
    text = original[len(bom):].decode(encoding)
    fields, versions = [], {}
    for name, pattern, separator in rules:
        matches = list(re.finditer(pattern, text, re.MULTILINE))
        if len(matches) != 1:
            raise VersionError(f"{relative}: {name} 应出现一次，实际为 {len(matches)} 次")
        match = matches[0]
        raw = match.group("value")
        parts = [part.strip() for part in raw.split(separator)]
        if len(parts) != 4:
            raise VersionError(f"{relative}: {name} 须使用 {separator} 分隔四段整数")
        value = ".".join(parts)
        try:
            validate_version(value)
        except ValueError as error:
            raise VersionError(f"{relative}: {name}: {error}") from error
        # 只替换版本分量，保留字段两侧空白及原始换行。
        start, end = match.span("value")
        fields.append((start + len(raw) - len(raw.lstrip()), end - len(raw) + len(raw.rstrip()), separator))
        versions[name] = value
    return VersionFile(path, relative, original, text, encoding, bom, fields, versions, active)


def inspect_project(root):
    professional = (root / PROFESSIONAL_HEADER).exists()
    edition = "professional" if professional else "community"
    files = [read_version_file(root, COMMUNITY_HEADER, header_field("Pandas_Version"), not professional)]
    community_version = files[0].versions["Pandas_Version"]
    if professional:
        files.append(read_version_file(root, PROFESSIONAL_HEADER, header_field("Pandas_Commercial_Version")))
    version = next(iter(files[-1].versions.values()))
    resources = set(REQUIRED_RESOURCES)
    resources.update(path.relative_to(root).as_posix() for path in (root / "src").rglob("*.rc"))
    files.extend(read_version_file(root, path, RESOURCE_FIELDS) for path in sorted(resources))
    issues = [
        f"{file.relative}: {name} 为 {value}，应与 {version} 一致"
        for file in files if file.active
        for name, value in file.versions.items() if value != version
    ]
    report = {
        "edition": edition,
        "version": version,
        "community_version": community_version,
        "display_version": display_version(version, edition),
        "consistent": not issues,
        "issues": issues,
        "files": [{"path": file.relative, "versions": file.versions, "active": file.active} for file in files],
    }
    return files, report


def temporary_copy(path, content):
    fd, name = tempfile.mkstemp(prefix=f".{path.name}.version-", dir=path.parent)
    temporary = Path(name)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(content)
        os.chmod(temporary, stat.S_IMODE(path.stat().st_mode))
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise
    return temporary


def write_changes(changes):
    staged, written, retained = [], [], set()
    try:
        # 先准备全部新文件和恢复副本，完成后再替换目标。
        for file, content in changes:
            new = temporary_copy(file.path, content)
            staged.append((file, new, None))
            backup = temporary_copy(file.path, file.original)
            staged[-1] = (file, new, backup)
        for file, _, _ in staged:
            if file.path.read_bytes() != file.original:
                raise VersionError(f"检查后文件已变化，请重新运行: {file.relative}")
        for file, new, backup in staged:
            os.replace(new, file.path)
            written.append((file, backup))
    except (OSError, VersionError, KeyboardInterrupt) as error:
        recovery_errors = []
        for file, backup in reversed(written):
            try:
                os.replace(backup, file.path)
            except OSError:
                retained.add(backup)
                recovery_errors.append(f"{file.relative} 的原内容保存在 {backup}")
        if recovery_errors:
            raise VersionError(f"写入中断: {error}; 自动恢复失败: {'; '.join(recovery_errors)}") from error
        raise
    finally:
        for _, new, backup in staged:
            for temporary in (new, backup):
                if temporary is not None and temporary not in retained:
                    temporary.unlink(missing_ok=True)


def set_version(files, report, version, dry_run=False, expect_current=None):
    if expect_current is not None:
        mismatches = [
            f"{file.relative}: {name}={value}"
            for file in files if file.active
            for name, value in file.versions.items() if value != expect_current
        ]
        if mismatches:
            raise VersionError(f"当前版本不符合 --expect-current {expect_current}: " + "; ".join(mismatches))
    changes = []
    for file in files:
        if file.active and any(value != version for value in file.versions.values()):
            content = file.render(version)
            if content != file.original:
                changes.append((file, content))
    result = {
        **report,
        "target_version": version,
        "target_display_version": display_version(version, report["edition"]),
        "dry_run": dry_run,
        "planned_changes": [{"path": file.relative, "from": sorted(set(file.versions.values())), "to": version} for file, _ in changes],
        "changed_files": [],
    }
    if not dry_run:
        write_changes(changes)
        result["changed_files"] = [file.relative for file, _ in changes]
    return result


def emit(result, as_json=False):
    if as_json:
        # ASCII 转义使机器输出也能通过 Windows 的非 UTF-8 重定向流。
        print(json.dumps(result, ensure_ascii=True, indent=2))
        return
    if "error" in result:
        print(f"错误: {result['error']}", file=sys.stderr)
        return
    edition = "专业版" if result["edition"] == "professional" else "社区版"
    print(f"{edition}: {result['version']} ({result['display_version']})")
    if result["edition"] == "professional":
        print(f"社区版基线: {result['community_version']}")
    if result["command"] == "set":
        for change in result["planned_changes"]:
            print(f"  {change['path']}: {', '.join(change['from'])} -> {change['to']}")
        count = len(result["planned_changes"])
        if not count:
            print("版本已一致，无需修改")
        elif result["dry_run"]:
            print(f"预览完成，计划修改 {count} 个文件，未写入")
        else:
            print(f"已将 {count} 个文件更新为 {result['target_version']} ({result['target_display_version']})")
    else:
        for file in result["files"]:
            values = ", ".join(f"{name}={version}" for name, version in file["versions"].items())
            print(f"  {file['path']}: {values}")
        print("版本检查通过" if result["consistent"] else "版本不一致")
        for issue in result["issues"]:
            print(f"  {issue}")


def make_parser():
    parser = ArgumentParser(description="查看、检查或批量修改 Pandas 版本号；无参数时使用交互输入。", allow_abbrev=False)
    parser.add_argument("--json", action="store_true", help="输出 JSON（帮助信息除外）")
    commands = parser.add_subparsers(dest="command", parser_class=ArgumentParser)
    for command, description in (("show", "显示当前版本"), ("check", "检查版本字段是否完整且一致"), ("set", "设置四段式版本")):
        subparser = commands.add_parser(command, help=description, description=description, allow_abbrev=False)
        subparser.add_argument("--json", action="store_true", default=argparse.SUPPRESS, help="输出 JSON")
        if command == "set":
            subparser.add_argument("version", type=version_argument, help="目标版本，例如 1.2.5.0")
            subparser.add_argument("--dry-run", action="store_true", help="预览全部改动，不写入")
            subparser.add_argument("--expect-current", type=version_argument, help="要求全部活动版本字段与此版本一致")
    return parser


def interactive(root):
    files, report = inspect_project(root)
    emit({"ok": True, "command": "show", **report})
    rule = "YYYY.MM.DD.REV，第四段为修订号" if report["edition"] == "professional" else "例如 1.2.5.0，第四段 1 表示开发版，0 表示正式版"
    version = input(f"请输入新的四段式版本号（{rule}）: ").strip()
    try:
        validate_version(version)
    except ValueError as error:
        raise UsageError(str(error)) from error
    emit({"ok": True, "command": "set", **set_version(files, report, version)})
    return 0


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    as_json = "--json" in argv
    command = None
    is_interactive = not argv and sys.stdin.isatty()
    try:
        if is_interactive:
            return interactive(PROJECT_ROOT)
        parser = make_parser()
        args = parser.parse_args(argv)
        command = args.command
        if command is None:
            raise UsageError("无交互终端时须指定 show、check 或 set；使用 --help 查看用法")
        files, report = inspect_project(PROJECT_ROOT)
        if command == "set":
            report = set_version(files, report, args.version, args.dry_run, args.expect_current)
        ok = command != "check" or report["consistent"]
        emit({"ok": ok, "command": command, **report}, args.json)
        return 0 if ok else 1
    except UsageError as error:
        emit({"ok": False, "command": command, "error": str(error)}, as_json)
        return 2
    except (VersionError, OSError, UnicodeError, EOFError) as error:
        emit({"ok": False, "command": command, "error": str(error)}, as_json)
        return 1
    except KeyboardInterrupt:
        emit({"ok": False, "command": command, "error": "操作已取消"}, as_json)
        return 130
    finally:
        if is_interactive and os.name == "nt":
            try:
                input("按 Enter 关闭窗口...")
            except (EOFError, KeyboardInterrupt):
                pass


if __name__ == "__main__":
    sys.exit(main())
