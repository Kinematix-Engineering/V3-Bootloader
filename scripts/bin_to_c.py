#!/usr/bin/env python3
import pathlib
import sys

source = pathlib.Path(sys.argv[1]).read_bytes()
target = pathlib.Path(sys.argv[2])
with target.open("w") as output:
    output.write("#include <stdint.h>\n")
    output.write("const uint8_t boot_image[] __attribute__((aligned(4))) = {\n")
    for offset in range(0, len(source), 16):
        output.write("    " + ", ".join(f"0x{byte:02x}" for byte in source[offset:offset + 16]) + ",\n")
    output.write("};\n")
    output.write(f"const uint32_t boot_image_len = {len(source)}u;\n")
