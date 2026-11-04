# embed.py - converts 34Cheat.dll into C++ array for single-exe standalone build
import sys
import os
import hashlib

dll_path = os.path.join(os.path.dirname(__file__), "34Cheat.dll")
out_path = os.path.join(os.path.dirname(__file__), "EmbeddedDll.h")

if not os.path.exists(dll_path):
    print(f"Error: {dll_path} not found")
    sys.exit(1)

with open(dll_path, "rb") as f:
    data = f.read()

print(f"Embedding {len(data)} bytes from {dll_path}...")

with open(out_path, "w") as f:
    f.write("// Auto-generated Embedded DLL data for 34Cheat standalone executable\n")
    f.write("#pragma once\n#include <cstddef>\n\n")
    f.write(f"const size_t g_EmbeddedDllSize = {len(data)};\n")
    f.write(f'const char g_EmbeddedDllSha256[] = "{hashlib.sha256(data).hexdigest()}";\n')
    f.write("const unsigned char g_EmbeddedDllData[] = {\n")
    
    # Write in chunks of 16 bytes for fast MSVC compilation
    chunk_size = 16
    lines = []
    for i in range(0, len(data), chunk_size):
        chunk = data[i:i + chunk_size]
        hex_bytes = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append("    " + hex_bytes + ",")
    f.write("\n".join(lines))
    f.write("\n};\n")

print(f"Successfully generated {out_path} ({len(data)} bytes embedded).")
