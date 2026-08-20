#!/usr/bin/env python3
"""Build the Simplified Chinese translation/font header."""

from pathlib import Path
import argparse
import struct

def read_translations(path: Path) -> dict[str, str]:
    translations = {}
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw or raw.startswith("#"):
            continue
        if "|" not in raw:
            raise ValueError(f"{path}:{number}: missing '|'")
        english, chinese = raw.split("|", 1)
        english = english.replace(r"\n", "\n")
        chinese = chinese.replace(r"\n", "\n")
        if not english or not chinese:
            raise ValueError(f"{path}:{number}: empty translation")
        if english in translations:
            raise ValueError(f"{path}:{number}: duplicate key {english!r}")
        translations[english] = chinese
    return translations


def read_bdf(path: Path) -> dict[int, tuple[int, int, int, int, list[int]]]:
    glyphs = {}
    encoding = None
    advance = None
    box = None
    bitmap = None
    for raw in path.read_text(encoding="ascii").splitlines():
        if raw.startswith("ENCODING "):
            encoding = int(raw.split()[1])
        elif raw.startswith("DWIDTH "):
            advance = int(raw.split()[1])
        elif raw.startswith("BBX "):
            box = tuple(map(int, raw.split()[1:]))
        elif raw == "BITMAP":
            bitmap = []
        elif raw == "ENDCHAR":
            if encoding is not None and encoding >= 0 and advance and box and bitmap is not None:
                glyphs[encoding] = (advance, *box, bitmap)
            encoding = advance = box = bitmap = None
        elif bitmap is not None:
            bitmap.append(int(raw, 16))
    return glyphs


def normalize_glyph(codepoint: int, glyph) -> tuple[int, list[int]]:
    advance, width, height, x_offset, y_offset, bitmap = glyph
    if advance > 12 or width > 12 or height > 18 or x_offset < 0:
        raise ValueError(f"U+{codepoint:04X}: unsupported metrics {advance}, BBX {width} {height} {x_offset} {y_offset}")
    rows = [0] * 16
    top = 13 - (y_offset + height)
    source_bits = (width + 7) // 8 * 8
    for row, value in enumerate(bitmap):
        target = top + row
        if 0 <= target < 16:
            rows[target] |= (value >> (source_bits - width)) << (12 - width - x_offset)
    return advance, rows


def generate(translations: dict[str, str], glyphs, output: Path) -> None:
    codepoints = sorted(
        {ord(ch) for text in translations.values() for ch in text if ord(ch) >= 128}
        | set(range(32, 127))
    )
    missing = [cp for cp in codepoints if cp not in glyphs]
    if missing:
        raise ValueError("missing glyphs: " + ", ".join(f"U+{cp:04X}" for cp in missing))

    header_size = struct.calcsize("<7I")
    translation_offset = header_size
    glyph_offset = translation_offset + len(translations) * struct.calcsize("<2I")
    string_offset = glyph_offset + len(codepoints) * struct.calcsize("<18H")

    strings = bytearray()
    records = bytearray()
    for english, chinese in sorted(translations.items()):
        english_offset = string_offset + len(strings)
        strings.extend(english.encode("utf-8") + b"\0")
        chinese_offset = string_offset + len(strings)
        strings.extend(chinese.encode("utf-8") + b"\0")
        records.extend(struct.pack("<2I", english_offset, chinese_offset))

    glyph_data = bytearray()
    for codepoint in codepoints:
        advance, rows = normalize_glyph(codepoint, glyphs[codepoint])
        glyph_data.extend(struct.pack("<18H", codepoint, advance, *rows))

    size = string_offset + len(strings)
    header = struct.pack(
        "<7I", 0x31485A4D, 2, size, len(translations), len(codepoints),
        translation_offset, glyph_offset,
    )
    data = header + records + glyph_data + strings
    assert len(data) == size
    output.write_bytes(data)
    print(f"{len(translations)} translations, {len(codepoints)} glyphs -> {output}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("translations", type=Path)
    parser.add_argument("bdf", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    generate(read_translations(args.translations), read_bdf(args.bdf), args.output)


if __name__ == "__main__":
    main()
