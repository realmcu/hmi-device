#!/usr/bin/env python3
"""Host core + isolated Kconfig + CMake source-graph regression (no SDK required).

Run: python modules/device/tests/run_input_tests.py
Requires Python kconfiglib, gcc and cmake on PATH. All generated files are placed
in a temporary directory, not in the source tree. The CMake mock validates graph
selection only; it does not compile Zephyr/HAL drivers or replace board builds.
"""
# SPDX-License-Identifier: Apache-2.0
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

DEVICE = Path(__file__).resolve().parents[1]
TESTS = Path(__file__).resolve().parent
DRIVERS = ("CHSC6417", "CHSC6413", "CST816D", "CST820")


def run(command):
    subprocess.run([str(x) for x in command], check=True)


def test_core(root, gcc):
    executable = root / ("touch_core.exe" if os.name == "nt" else "touch_core")
    core = DEVICE / "input/touch/core"
    run([gcc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", core,
         TESTS / "touch_core_test.c", core / "rtk_touch.c", "-o", executable])
    run([executable])


def test_kconfig(root):
    import kconfiglib
    mock = root / "Kconfig"
    external = ("REALTEK_DISPLAY", "REALTEK_HONEYGUI", "GPIO", "I2C",
                "GPIO_ENABLE_DISABLE_INTERRUPT", "SOC_SERIES_RTL87X3G", "ZEPHYR")
    mock.write_text("\n".join(f'config {name}\n    bool "{name}"' for name in external)
                    + f'\nsource "{(DEVICE / "Kconfig").as_posix()}"\n', encoding="utf-8")

    def config():
        return kconfiglib.Kconfig(str(mock), warn_to_stderr=False)

    kconf = config()
    priority = kconf.syms["REALTEK_INPUT_INIT_PRIORITY"]
    assert priority.type == kconfiglib.INT and priority.str_value == "60"
    for value in ("51", "99"):
        priority.set_value(value)
        assert priority.str_value == value
    for value in ("50", "100"):
        priority.set_value(value)
        assert priority.str_value == "60", "out-of-range init priority must fall back to default"
    priority.unset_value()
    assert kconf.syms["REALTEK_TOUCH"].str_value == "n"
    kconf.syms["REALTEK_DISPLAY"].set_value(2)
    assert kconf.syms["REALTEK_TOUCH"].str_value == "n", "display must not enable core by default"
    kconf.syms["REALTEK_DISPLAY"].set_value(0)
    kconf.syms["REALTEK_HONEYGUI"].set_value(2)
    assert kconf.syms["REALTEK_TOUCH"].str_value == "y"
    kconf.syms["REALTEK_TOUCH"].set_value(0)
    assert kconf.syms["REALTEK_TOUCH"].str_value == "n", "explicit core disable must win over GUI default"
    for driver in DRIVERS:
        kconf = config()
        fragment = root / f"{driver}.conf"
        fragment.write_text(f"CONFIG_REALTEK_TOUCH_{driver}_ZEPHYR=y\n", encoding="utf-8")
        kconf.load_config(str(fragment))
        assert kconf.syms[f"REALTEK_TOUCH_{driver}_ZEPHYR"].str_value == "y", "existing prj.conf selection must activate optional choice"
        for selected in ("REALTEK_TOUCH", "GPIO", "I2C"):
            assert kconf.syms[selected].str_value == "y", (driver, selected)
        assert kconf.syms["REALTEK_DISPLAY"].str_value == "n"
        assert sum(kconf.syms[f"REALTEK_TOUCH_{x}_ZEPHYR"].tri_value == 2 for x in DRIVERS) == 1
        other = next(x for x in DRIVERS if x != driver)
        kconf.syms[f"REALTEK_TOUCH_{other}_ZEPHYR"].set_value(2)
        assert sum(kconf.syms[f"REALTEK_TOUCH_{x}_ZEPHYR"].tri_value == 2 for x in DRIVERS) == 1
    for legacy in ("CHSC6417", "CHSC6413", "GT911", "CST816D", "CST820"):
        kconf = config()
        kconf.syms[f"REALTEK_TOUCH_{legacy}"].set_value(2)
        assert kconf.syms["REALTEK_TOUCH"].str_value == "y"
    kconf = config()
    kconf.syms["REALTEK_KEY_BUTTON_8773G_ZEPHYR"].set_value(2)
    for selected in ("REALTEK_KEY_BUTTON", "GPIO", "GPIO_ENABLE_DISABLE_INTERRUPT"):
        assert kconf.syms[selected].str_value == "y"
    display = (DEVICE.parent / "display/Kconfig").read_text(encoding="utf-8")
    assert not re.search(r"^config REALTEK_(?:TOUCH|KEY_BUTTON)", display, re.M)
    module = (DEVICE / "zephyr/module.yml").read_text(encoding="utf-8")
    assert re.search(r"^\s+kconfig:\s*Kconfig\s*$", module, re.M)
    print("PASS isolated device Kconfig: GUI default, explicit disable, four controllers, choice, legacy, key, module entry")


HARNESS = '''cmake_minimum_required(VERSION 3.20)
project(input_graph C)
set(ZEPHYR_BASE "mock-zephyr")
function(zephyr_library_named name)
    add_library(${name} STATIC)
endfunction()
# Present even in input-only configurations: catch accidental TARGET-only links.
add_library(realtek_display_driver_library INTERFACE)
target_include_directories(realtek_display_driver_library INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/display-hardware-marker")
add_library(guiLib INTERFACE)
add_subdirectory("@DEVICE@" device)
set(report "target=absent\\n")
if(TARGET realtek_device_driver_library)
    set(report "target=present\\n")
    foreach(property SOURCES LINK_LIBRARIES INTERFACE_LINK_LIBRARIES INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(value realtek_device_driver_library ${property})
        if(value MATCHES "-NOTFOUND$")
            set(value "")
        endif()
        string(APPEND report "${property}=${value}\\n")
    endforeach()
endif()
file(WRITE "${CMAKE_BINARY_DIR}/graph.txt" "${report}")
'''


def test_cmake(root, cmake, gcc):
    harness = root / "harness"
    harness.mkdir()
    (harness / "CMakeLists.txt").write_text(HARNESS.replace("@DEVICE@", DEVICE.as_posix()), encoding="utf-8")
    base = {"CONFIG_SOC_SERIES_RTL87X3G": "y", "CONFIG_REALTEK_TOUCH": "n",
            "CONFIG_REALTEK_DISPLAY": "n", "CONFIG_REALTEK_KEY_BUTTON": "n"}
    cases = [("empty", {}, False, False, False),
             ("core-only", {"CONFIG_REALTEK_TOUCH": "y"}, True, False, False),
             ("display-only", {"CONFIG_REALTEK_DISPLAY": "y"}, False, True, False),
             ("mixed", {"CONFIG_REALTEK_DISPLAY": "y", "CONFIG_REALTEK_TOUCH": "y",
                        "CONFIG_REALTEK_TOUCH_CHSC6417_ZEPHYR": "y"}, True, True, False),
             ("key-only", {"CONFIG_REALTEK_KEY_BUTTON": "y",
                           "CONFIG_REALTEK_KEY_BUTTON_8773G_ZEPHYR": "y"}, False, False, True)]
    for driver in DRIVERS:
        cases.append((f"touch-only-{driver}", {"CONFIG_REALTEK_TOUCH": "y",
                      f"CONFIG_REALTEK_TOUCH_{driver}_ZEPHYR": "y"}, True, False, False))
    for name, overrides, touch, display, key in cases:
        values = dict(base, **overrides)
        build = root / name
        run([cmake, "-S", harness, "-B", build, "-G", "Ninja", f"-DCMAKE_C_COMPILER={gcc}"]
            + [f"-D{k}={v}" for k, v in values.items()])
        report = (build / "graph.txt").read_text(encoding="utf-8").replace("\\", "/")
        if name == "empty":
            assert report == "target=absent\n", report
        else:
            assert "target=present" in report
            assert ("/core/rtk_touch.c" in report) == touch, (name, report)
            assert ("/output/display/" in report) == display, (name, report)
            assert ("realtek_display_driver_library" in report) == display, (name, report)
            assert ("key_button_8773g_zephyr.c" in report) == key, (name, report)
            if not display:
                assert "display-hardware-marker" not in report and "/zephyr/display" not in report, report
            if not touch:
                assert "/input/touch" not in report, (name, report)
            selected = [driver for driver in DRIVERS if values.get(f"CONFIG_REALTEK_TOUCH_{driver}_ZEPHYR") == "y"]
            if selected:
                sources = next(line for line in report.splitlines() if line.startswith("SOURCES="))
                assert "rtk_touch_zephyr.c" in sources, (name, report)
                for driver in DRIVERS:
                    filename = f"touch_{driver if driver.startswith('CHSC') else driver.lower()}_zephyr.c"
                    assert (filename in sources) == (driver in selected), (name, filename, report)
        print(f"PASS CMake graph {name}")
    invalid = dict(base, CONFIG_REALTEK_TOUCH="y",
                   CONFIG_REALTEK_TOUCH_CHSC6417_ZEPHYR="y",
                   CONFIG_REALTEK_TOUCH_CST820_ZEPHYR="y")
    result = subprocess.run([str(cmake), "-S", str(harness), "-B", str(root / "invalid-multiple"),
                             "-G", "Ninja", f"-DCMAKE_C_COMPILER={gcc}"]
                            + [f"-D{k}={v}" for k, v in invalid.items()], check=False)
    assert result.returncode != 0, "CMake must reject multiple touch controllers"
    print("PASS CMake rejects multiple touch controllers (expected configure failure above)")


def test_non_zephyr(root, cmake, gcc):
    harness = root / "non-zephyr-harness"
    harness.mkdir()
    project = HARNESS.replace('set(ZEPHYR_BASE "mock-zephyr")', 'set(ZEPHYR_BASE "")')
    project = project.replace('@DEVICE@', DEVICE.as_posix())
    # Core-only must compile/link with neither display target nor HAL target.
    core_project = project.replace('add_library(realtek_display_driver_library INTERFACE)', '')
    core_project = core_project.replace('target_include_directories(realtek_display_driver_library INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/display-hardware-marker")', '')
    core_project += f'\nadd_executable(core_test "{(TESTS / "touch_core_test.c").as_posix()}")\ntarget_link_libraries(core_test PRIVATE realtek_device_driver_library)\n'
    (harness / "CMakeLists.txt").write_text(core_project, encoding="utf-8")
    build = root / "non-zephyr-core"
    run([cmake, "-S", harness, "-B", build, "-G", "Ninja", f"-DCMAKE_C_COMPILER={gcc}",
         "-DCONFIG_REALTEK_TOUCH=y", "-DCONFIG_REALTEK_DISPLAY=n", "-DCONFIG_REALTEK_KEY_BUTTON=n"])
    report = (build / "graph.txt").read_text(encoding="utf-8")
    assert "halLib" not in report and "realtek_display_driver_library" not in report, report
    run([cmake, "--build", build])
    run([build / ("core_test.exe" if os.name == "nt" else "core_test")])
    print("PASS non-Zephyr core-only CMake compile/link/run without display or HAL")
    # Configure-only graphs: legacy hardware compilation still requires SDK.
    (harness / "CMakeLists.txt").write_text(project, encoding="utf-8")
    cases = {"legacy": {"CONFIG_REALTEK_TOUCH_CHSC6417": "y"},
             "mixed": {"CONFIG_REALTEK_DISPLAY": "y", "CONFIG_REALTEK_TOUCH_CHSC6417": "y"},
             "key": {"CONFIG_REALTEK_KEY_BUTTON": "y", "CONFIG_REALTEK_KEY_BUTTON_8773G": "y"}}
    for name, extra in cases.items():
        config = dict(CONFIG_REALTEK_TOUCH="y", CONFIG_REALTEK_DISPLAY="n",
                      CONFIG_REALTEK_KEY_BUTTON="n", CONFIG_SOC_SERIES_RTL87X3G="y")
        config.update(extra)
        build = root / f"non-zephyr-{name}"
        run([cmake, "-S", harness, "-B", build, "-G", "Ninja", f"-DCMAKE_C_COMPILER={gcc}"]
            + [f"-D{k}={v}" for k, v in config.items()])
        report = (build / "graph.txt").read_text(encoding="utf-8")
        assert "halLib" in report, (name, report)
        assert ("realtek_display_driver_library" in report) == (name == "mixed"), report
        print(f"PASS non-Zephyr {name} graph retains HAL")
    for driver in DRIVERS:
        result = subprocess.run([str(cmake), "-S", str(harness), "-B",
                                 str(root / f"non-zephyr-invalid-{driver}"), "-G", "Ninja",
                                 f"-DCMAKE_C_COMPILER={gcc}", "-DCONFIG_REALTEK_TOUCH=y",
                                 "-DCONFIG_REALTEK_DISPLAY=n", "-DCONFIG_REALTEK_KEY_BUTTON=n",
                                 f"-DCONFIG_REALTEK_TOUCH_{driver}_ZEPHYR=y"], check=False)
        assert result.returncode != 0, f"non-Zephyr must reject {driver} Zephyr controller"
        print(f"PASS non-Zephyr rejects {driver} Zephyr controller (expected configure failure above)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gcc", default=shutil.which("gcc"))
    parser.add_argument("--cmake", default=shutil.which("cmake"))
    args = parser.parse_args()
    if not args.gcc or not args.cmake or not shutil.which("ninja"):
        parser.error("gcc, cmake and ninja must be available (or pass --gcc/--cmake)")
    with tempfile.TemporaryDirectory(prefix="device-input-tests-") as temporary:
        root = Path(temporary)
        test_core(root, args.gcc)
        test_kconfig(root)
        test_cmake(root, args.cmake, args.gcc)
        test_non_zephyr(root, args.cmake, args.gcc)
    print("PASS all input host regressions (Zephyr driver compilation not exercised)")


if __name__ == "__main__":
    main()
