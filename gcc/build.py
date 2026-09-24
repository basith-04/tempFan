"""Build the CubeMX/IAR source list with the installed bare-metal ARM GCC."""

import pathlib
import subprocess
import xml.etree.ElementTree as ET

project = pathlib.Path(__file__).resolve().parent.parent
output = project / "firmware-build"
output.mkdir(exist_ok=True)
includes = [
    project / "Core/Inc",
    project / "Drivers/STM32F4xx_HAL_Driver/Inc",
    project / "Drivers/STM32F4xx_HAL_Driver/Inc/Legacy",
    project / "Drivers/CMSIS/Device/ST/STM32F4xx/Include",
    project / "Drivers/CMSIS/Include",
]
flags = [
    "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard",
    "-DUSE_HAL_DRIVER", "-DSTM32F411xE", "-ffreestanding", "-Os", "-g3",
    "-ffunction-sections", "-fdata-sections", "-Wall", "-Wextra",
    *["-I" + str(path) for path in includes],
]
sources = []
for node in ET.parse(project / "EWARM/Fan.ewp").findall(".//file/name"):
    if node.text and node.text.startswith("$PROJ_DIR$/"):
        source = (project / "EWARM" / node.text[len("$PROJ_DIR$/"):]).resolve()
        if source.suffix == ".c":
            sources.append(source)
sources.extend([
    project / "Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s",
    project / "gcc/minimal_runtime.c",
])
objects = []
for index, source in enumerate(sources):
    obj = output / f"source_{index}.o"
    subprocess.run(["arm-none-eabi-gcc", *flags, "-c", str(source), "-o", str(obj)], check=True)
    objects.append(str(obj))
elf = output / "fan.elf"
subprocess.run([
    "arm-none-eabi-gcc", *flags, "-nostdlib", "-Wl,--gc-sections",
    "-Wl,-Map=" + str(output / "fan.map"),
    "-T", str(project / "gcc/stm32f411ceux_flash.ld"), *objects, "-lgcc",
    "-o", str(elf),
], check=True)
subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", str(elf),
                str(output / "fan.bin")], check=True)
subprocess.run(["arm-none-eabi-size", str(elf)], check=True)
