#!/usr/bin/env python3
import subprocess
import re
import sys
import os

# File
NAME = "V3 Bootloader"
MODULE_PART = "KEL-CTRL-V3*"
MODULE_TYPE = "0x10"
PERMIT = "KDiag"

# Cfg
KFLASHCLI = r"KFlashCLI"
OUTPUT_DIR = "../build/kmx_v3/"
INCLUDE_DIR = "../inc/main.h"
DEVICE = "ATSAMD51P20A"
TOOLCHAIN = "Atmel atprogram"

def get_version_from_header(header_path):
    with open(header_path, 'r') as f:
        lines = f.readlines()
    
    in_version_block = False
    name = major = minor = patch = None
    
    for line in lines:
        if 'BEGIN VERSION' in line:
            in_version_block = True
            continue
        
        if in_version_block:
            if match := re.search(r'BOOTLOADER_VERSION_NAME\s+"([^"]+)"', line):
                name = match.group(1)
            elif match := re.search(r'BOOTLOADER_VERSION_MAJOR\s+(\d+)', line):
                major = match.group(1)
            elif match := re.search(r'BOOTLOADER_VERSION_MINOR\s+(\d+)', line):
                minor = match.group(1)
            elif match := re.search(r'BOOTLOADER_VERSION_PATCH\s+(\d+)', line):
                patch = match.group(1)
            
            if all(v is not None for v in [name, major, minor, patch]):
                return f"{name}{major}.{minor}.{patch}"
    
    raise ValueError("Could not parse version from header")

version = get_version_from_header(INCLUDE_DIR)
output_file = os.path.join(OUTPUT_DIR.rstrip('/'), f"{version}.kflash")
bin_file = os.path.join(OUTPUT_DIR, "bootloader-kmx_v3-.bin")

result = subprocess.run([
    KFLASHCLI, "make",
    "--output", output_file,
    "--permit", PERMIT,
    "--name", NAME,
    "--version", version,
    "--modulePart", MODULE_PART,
    "--moduleType", MODULE_TYPE,
    "--toolchain", TOOLCHAIN,
    "--segment", f"bootloader|0x0|{bin_file}",
    "--flag", f"device|{DEVICE}"
])

sys.exit(result.returncode)