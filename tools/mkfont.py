import sys

FIRST = 32
LAST = 126
HEIGHT = 16


def load_glyphs(path):
    glyphs = {}
    for line in open(path).read().splitlines():
        if ":" not in line:
            continue
        code, bitmap = line.split(":")
        glyphs[int(code, 16)] = bitmap
    return glyphs


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: mkfont.py unscii-16.hex font.c")

    glyphs = load_glyphs(sys.argv[1])
    lines = [
        '#include "font.h"',
        "",
        "const unsigned char font_glyphs[FONT_COUNT][FONT_HEIGHT] = {",
    ]

    for code in range(FIRST, LAST + 1):
        bitmap = glyphs.get(code, "")
        if len(bitmap) != HEIGHT * 2:
            sys.exit("glyph %d is not 8x16" % code)
        rows = ", ".join("0x" + bitmap[i:i + 2].upper() for i in range(0, len(bitmap), 2))
        lines.append("    {%s}," % rows)

    lines.append("};")
    lines.append("")
    open(sys.argv[2], "w").write("\n".join(lines))


main()
