#!/usr/bin/env python3
import subprocess
import re
import sys
import os

# --- Configuration ---
NAME = "V3 Bootloader"
MODULE_PART = "KEL-CTRL-V3*"
MODULE_TYPE = "0x10"
PERMIT = "KDiag"

# Cfg
KFLASHCLI = r"kdc"
DEVICE = "ATSAMD51P20A"
TOOLCHAIN = "Atmel atprogram"

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR_REL = "build/kmx_v3/"
INCLUDE_DIR_REL = "inc/main.h"

OUTPUT_DIR = os.path.join(PROJECT_ROOT, OUTPUT_DIR_REL)
INCLUDE_DIR = os.path.join(PROJECT_ROOT, INCLUDE_DIR_REL)
# ----------------------------------------

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
    
    raise ValueError(f"Could not parse version from header: {header_path}")

version = get_version_from_header(INCLUDE_DIR)
os.makedirs(OUTPUT_DIR, exist_ok=True) 

output_file = os.path.join(OUTPUT_DIR, f"{version}.kflash")
bin_file = os.path.join(OUTPUT_DIR, "bootloader-kmx_v3.bin")

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
    "--flag", f"device|{DEVICE}",
    "--mapping", "v3bl_factorydata|0x3EF0"
])

sys.exit(result.returncode)