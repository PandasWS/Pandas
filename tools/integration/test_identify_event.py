#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""鉴定事件模式回归测试：提取并编译生产 skill_identify 原函数。

运行：python3 tools/integration/test_identify_event.py
需要支持 C++17 的编译器；默认使用 c++，也可通过 CXX 指定。测试不依赖数据库或完整服务。
"""

from __future__ import annotations

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
MODE_MACRO = "Pandas_BattleConfig_Identify_Event_Mode"
FILTER_MACRO = "Pandas_NpcFilter_IDENTIFY"


def production_function() -> str:
    source = (ROOT / "src/map/skill.cpp").read_text(encoding="utf-8-sig")
    marker = "void skill_identify(map_session_data *sd, int32 idx)"
    start = source.index(marker)
    opening = source.index("{", start)
    end = source.find("\n}\n\n/*", opening)
    if end < 0:
        raise AssertionError("skill_identify 函数边界未找到")
    function = source[start : end + 2]
    if "#ifdef Pandas_BattleConfig_Identify_Event_Mode" not in function or "#else" not in function:
        raise AssertionError("skill_identify 配置条件分支缺失")
    return function


def config_fixture() -> tuple[str, int]:
    header = (ROOT / "src/map/battle.hpp").read_text(encoding="utf-8-sig")
    match = re.search(r"(?m)^\s*int32 identify_event_mode;[^\n]*$", header)
    if not match or "#ifdef Pandas_BattleConfig_Identify_Event_Mode" not in header[: match.start()][-160:]:
        raise AssertionError("battle.hpp 缺少受模式宏保护的 identify_event_mode 字段")
    field_declaration = match.group(0)
    registration = (ROOT / "src/map/battle.cpp").read_text(encoding="utf-8-sig")
    match = re.search(
        r'\{\s*"identify_event_mode"\s*,\s*&battle_config\.identify_event_mode\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,',
        registration,
    )
    if not match:
        raise AssertionError("battle.cpp 缺少 identify_event_mode 配置登记")
    default, minimum, maximum = map(int, match.groups())
    if (default, minimum, maximum) != (0, 0, 1):
        raise AssertionError(f"配置范围应为默认 0、范围 0..1，实际为 {default}, {minimum}..{maximum}")
    conf = (ROOT / "conf/battle/pandas.conf").read_text(encoding="utf-8-sig")
    match = re.search(r"(?m)^identify_event_mode:\s*(\d+)\s*$", conf)
    if not match or int(match.group(1)) != default:
        raise AssertionError("conf/battle/pandas.conf 默认值与 battle.cpp 登记表不一致")
    return field_declaration, default


HARNESS = r'''#include <cstdlib>
#include <iostream>
#include <string>
using int32 = int;
constexpr int MAX_INVENTORY = 16;
constexpr int WIP_DISABLE_NONE = 0;
constexpr int NPCF_IDENTIFY = 1;
constexpr int NPCE_IDENTIFY = 8;
struct item { int nameid; int identify; };
struct map_session_data {
    struct { int workinprogress; } state;
    struct { union { item items_inventory[MAX_INVENTORY]; } u; } inventory;
    item* inventory_data[MAX_INVENTORY];
};
#ifdef Pandas_BattleConfig_Identify_Event_Mode
struct battle_config_t {
    @@FIELD@@
};
battle_config_t battle_config = { DEFAULT_MODE };
#endif
enum { FILTER_NONE, FILTER_HALT, FILTER_IDENTIFY, FILTER_REMOVE, FILTER_CLEAR_DATA };
int action = FILTER_NONE, selected_idx = -1;
int add_str(const char*) { return 1; }
void pc_setreg(map_session_data*, int, int32 idx) {
    selected_idx = idx;
    std::cout << "REG 1 " << idx << '\n';
}
bool npc_script_filter(map_session_data* sd, int) {
    item& it = sd->inventory.u.items_inventory[selected_idx];
    if (action == FILTER_IDENTIFY) it.identify = 1;
    if (action == FILTER_REMOVE) it.nameid = 0;
    if (action == FILTER_CLEAR_DATA) sd->inventory_data[selected_idx] = nullptr;
    return action == FILTER_HALT;
}
void clif_item_identified(map_session_data&, int32 idx, bool failure) {
    std::cout << "ACK " << idx << ' ' << failure << '\n';
}
void npc_script_event(map_session_data&, int event) {
    std::cout << "EVENT " << event << ' ' << selected_idx << '\n';
}
#define nullpo_retv(p) do { if (!(p)) return; } while (false)
''' + "\n" + "@@FUNCTION@@" + r'''
int main(int argc, char** argv) {
    const int mode = argc > 1 ? std::atoi(argv[1]) : 0;
    action = argc > 2 ? std::atoi(argv[2]) : FILTER_NONE;
#ifdef Pandas_BattleConfig_Identify_Event_Mode
    battle_config.identify_event_mode = mode;
#endif
    map_session_data sd{};
    constexpr int idx = 3;
    sd.state.workinprogress = 77;
    sd.inventory.u.items_inventory[idx] = { 501, 0 };
    sd.inventory_data[idx] = &sd.inventory.u.items_inventory[idx];
    skill_identify(&sd, idx);
    const item& it = sd.inventory.u.items_inventory[idx];
    std::cout << "STATE " << sd.state.workinprogress << ' ' << it.nameid << ' '
              << it.identify << ' ' << (sd.inventory_data[idx] != nullptr) << '\n';
}
'''


class IdentifyEventTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        field, default = config_fixture()
        cls.temp = tempfile.TemporaryDirectory(prefix="identify-event-")
        cls.directory = Path(cls.temp.name)
        harness = HARNESS.replace("DEFAULT_MODE", str(default)).replace("@@FIELD@@", field.strip()).replace(
            "@@FUNCTION@@", production_function()
        )
        cls.harness = harness

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temp.cleanup()

    def compile_variant(self, name: str, mode_enabled: bool, filter_enabled: bool) -> Path:
        source = self.directory / f"{name}.cpp"
        binary = self.directory / name
        text = self.harness
        # 保留生产预处理分支，仅通过编译选项控制宏组合。
        source.write_text(text, encoding="utf-8")
        command = shlex.split(os.environ.get("CXX", "c++")) + ["-std=c++17", str(source), "-o", str(binary)]
        if mode_enabled:
            command.insert(-4, f"-D{MODE_MACRO}")
        if filter_enabled:
            command.insert(-4, f"-D{FILTER_MACRO}")
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            self.fail(f"C++ 夹具编译失败：{result.stderr}")
        return binary

    def run_case(self, binary: Path, mode: int, action: int) -> list[str]:
        result = subprocess.run([str(binary), str(mode), str(action)], check=True, capture_output=True, text=True)
        return result.stdout.strip().splitlines()

    def test_configured_modes_and_filter_paths(self) -> None:
        binary = self.compile_variant("configured", True, True)
        for mode in (0, 1):
            for action, expected_identify, expected_failure, event in (
                (0, 1, 0, True),  # 成功
                (2, 1, 1, mode == 0),  # 过滤器先鉴定，核心结果为失败
            ):
                with self.subTest(mode=mode, action=action):
                    calls = self.run_case(binary, mode, action)
                    expected = ["REG 1 3", f"ACK 3 {expected_failure}"]
                    if event:
                        expected += ["REG 1 3", "EVENT 8 3"]
                    expected += [f"STATE 0 501 {expected_identify} 1"]
                    self.assertEqual(calls, expected)

            for action, final_item in ((1, "501 0 1"), (3, "0 0 1"), (4, "501 0 0")):
                with self.subTest(mode=mode, action=action):
                    self.assertEqual(
                        self.run_case(binary, mode, action),
                        ["REG 1 3", f"STATE 0 {final_item}"],
                    )

    def test_upstream_only_success_and_filter_disabled_build(self) -> None:
        upstream = self.compile_variant("upstream", False, True)
        self.assertEqual(self.run_case(upstream, 0, 0), ["REG 1 3", "ACK 3 0", "REG 1 3", "EVENT 8 3", "STATE 0 501 1 1"])
        self.assertEqual(self.run_case(upstream, 0, 2), ["REG 1 3", "ACK 3 1", "STATE 0 501 1 1"])

        configured_no_filter = self.compile_variant("configured-no-filter", True, False)
        for mode in (0, 1):
            self.assertEqual(
                self.run_case(configured_no_filter, mode, 0),
                ["ACK 3 0", "REG 1 3", "EVENT 8 3", "STATE 0 501 1 1"],
            )

        no_filter = self.compile_variant("no-filter", False, False)
        self.assertEqual(self.run_case(no_filter, 0, 0), ["ACK 3 0", "REG 1 3", "EVENT 8 3", "STATE 0 501 1 1"])
        # 过滤器未编译时，测试动作不会修改道具，鉴定正常成功。
        self.assertEqual(self.run_case(no_filter, 0, 1), ["ACK 3 0", "REG 1 3", "EVENT 8 3", "STATE 0 501 1 1"])


if __name__ == "__main__":
    unittest.main()
