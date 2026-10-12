#!/usr/bin/env python3
"""生成 Q15 正弦表 firmware/foc/sin_table.h

256 点覆盖整圈（0..2π），运行时线性插值。
用法：python3 tools/gen_sin_table.py
"""
import math
import pathlib

N = 256
out = pathlib.Path(__file__).resolve().parent.parent / "firmware" / "foc" / "sin_table.h"

vals = [int(round(math.sin(2 * math.pi * i / N) * 32767)) for i in range(N)]

lines = [
    "/* 由 tools/gen_sin_table.py 生成，请勿手改 */",
    "#ifndef SIN_TABLE_H",
    "#define SIN_TABLE_H",
    "#include <stdint.h>",
    "",
    f"#define FOC_SIN_TABLE_SIZE {N}",
    "",
    f"static const int16_t foc_sin_table[{N}] = {{",
]
for i in range(0, N, 8):
    lines.append("    " + ", ".join(f"{v}" for v in vals[i:i + 8]) + ",")
lines += ["};", "", "#endif /* SIN_TABLE_H */", ""]

out.write_text("\n".join(lines))
print(f"wrote {out}")
