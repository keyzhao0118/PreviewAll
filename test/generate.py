from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


PASSWORD = "PreviewAll-Test-123!"
MIB = 1024 * 1024


def canvas(size: tuple[int, int], mode: str = "RGBA") -> Image.Image:
    image = Image.new(mode, size, (245, 247, 250, 255) if mode == "RGBA" else (245, 247, 250))
    draw = ImageDraw.Draw(image)
    width, height = size
    colors = ("#1565c0", "#00897b", "#f9a825", "#c62828")
    for index, color in enumerate(colors):
        left = index * width // len(colors)
        right = (index + 1) * width // len(colors)
        draw.rectangle((left, 0, right, height // 3), fill=color)
    draw.rectangle((20, height // 3 + 20, width - 20, height - 20), outline="#202124", width=3)
    draw.text((40, height // 2), f"PreviewAll {width} x {height}", fill="#202124", font=ImageFont.load_default())
    return image


def generate_images(root: Path) -> list[dict]:
    output = root / "images"
    output.mkdir(parents=True, exist_ok=True)
    records: list[dict] = []

    rgb = canvas((1280, 720), "RGB")
    rgba = canvas((1024, 768), "RGBA")
    alpha = Image.new("RGBA", (640, 480), (0, 0, 0, 0))
    alpha_draw = ImageDraw.Draw(alpha)
    alpha_draw.ellipse((80, 40, 560, 440), fill=(21, 101, 192, 150), outline=(0, 0, 0, 255), width=4)

    cases = [
        ("png-rgb.png", lambda p: rgb.save(p, optimize=True), "RGB PNG"),
        ("png-alpha.png", lambda p: alpha.save(p, optimize=True), "PNG transparency"),
        ("jpeg-baseline.jpg", lambda p: rgb.save(p, quality=88), "Baseline JPEG"),
        ("jpeg-progressive.jpeg", lambda p: rgb.save(p, quality=88, progressive=True), "Progressive JPEG"),
        ("jpeg-cmyk.jpg", lambda p: rgb.convert("CMYK").save(p, quality=88), "CMYK JPEG"),
        ("bitmap.bmp", lambda p: rgb.resize((640, 480)).save(p), "BMP"),
        ("webp-lossy.webp", lambda p: rgb.save(p, quality=80), "Lossy WebP"),
        ("webp-lossless.webp", lambda p: rgba.save(p, lossless=True), "Lossless WebP"),
        ("tiff-deflate.tif", lambda p: rgba.save(p, compression="tiff_adobe_deflate"), "Compressed TIFF"),
        ("tiff-uncompressed.tiff", lambda p: rgb.resize((640, 480)).save(p, compression="raw"), "Uncompressed TIFF"),
        ("icon-multisize.ico", lambda p: rgba.resize((256, 256)).save(p, sizes=[(16, 16), (32, 32), (48, 48), (128, 128), (256, 256)]), "Multi-size ICO"),
    ]
    for name, writer, scenario in cases:
        writer(output / name)
        records.append(record(output / name, "image", scenario, "preview"))

    exif = Image.Exif()
    exif[274] = 6
    rgb.resize((900, 600)).save(output / "jpeg-exif-rotate.jpg", quality=88, exif=exif)
    records.append(record(output / "jpeg-exif-rotate.jpg", "image", "EXIF orientation 6", "rotated preview"))

    gray16 = Image.new("I;16", (1024, 512))
    gray16.putdata([(x * 65535) // 1023 for _y in range(512) for x in range(1024)])
    gray16.save(output / "png-16bit-gray.png")
    records.append(record(output / "png-16bit-gray.png", "image", "16-bit grayscale PNG", "preview"))

    frames = []
    for index, color in enumerate(("#1565c0", "#00897b", "#f9a825", "#c62828")):
        frame = Image.new("RGBA", (480, 320), color)
        ImageDraw.Draw(frame).text((30, 140), f"Animated frame {index + 1}", fill="white")
        frames.append(frame)
    frames[0].save(output / "animated.gif", save_all=True, append_images=frames[1:], duration=350, loop=0)
    records.append(record(output / "animated.gif", "image", "Animated GIF", "animated preview"))
    frames[0].save(output / "animated.webp", save_all=True, append_images=frames[1:], duration=350, loop=0, lossless=True)
    records.append(record(output / "animated.webp", "image", "Animated WebP", "preview first/animated frame depending on Qt plugin"))

    rgba.save(output / "multipage.tiff", save_all=True, append_images=[rgba.transpose(Image.Transpose.FLIP_LEFT_RIGHT)], compression="raw")
    records.append(record(output / "multipage.tiff", "image", "Multipage TIFF", "first page preview"))

    svg = """<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="800" viewBox="0 0 1200 800">
<rect width="1200" height="800" fill="#f5f7fa"/><circle cx="300" cy="400" r="220" fill="#1565c0" fill-opacity=".75"/>
<path d="M500 650 L750 120 L1050 650 Z" fill="#00897b" stroke="#202124" stroke-width="12"/>
<text x="600" y="740" text-anchor="middle" font-family="Segoe UI" font-size="64">PreviewAll SVG 中文</text></svg>"""
    (output / "vector.svg").write_text(svg, encoding="utf-8")
    records.append(record(output / "vector.svg", "image", "SVG vector and Unicode text", "preview"))

    large = Image.new("RGBA", (9000, 9000), (40, 120, 200, 255))
    large_draw = ImageDraw.Draw(large)
    for position in range(0, 9000, 500):
        large_draw.line((position, 0, position, 8999), fill=(255, 255, 255, 100), width=4)
        large_draw.line((0, position, 8999, position), fill=(255, 255, 255, 100), width=4)
    large.save(output / "large-memory-9000x9000.png", optimize=True)
    records.append(record(output / "large-memory-9000x9000.png", "image", "324 MB decoded RGBA image", "scaled preview without allocation failure"))
    del large

    panorama = Image.new("RGB", (20000, 2000), (15, 80, 140))
    panorama_draw = ImageDraw.Draw(panorama)
    for position in range(0, 20000, 1000):
        panorama_draw.rectangle((position, 0, position + 499, 1999), fill=(15 + (position // 1000) * 5, 100, 160))
    panorama.save(output / "large-panorama-20000x2000.jpg", quality=82, progressive=True)
    records.append(record(output / "large-panorama-20000x2000.jpg", "image", "Very wide 160 MB decoded image", "dimension-constrained preview"))
    del panorama

    good = (output / "png-rgb.png").read_bytes()
    (output / "corrupt-truncated.png").write_bytes(good[: len(good) // 3])
    records.append(record(output / "corrupt-truncated.png", "image", "Truncated PNG", "load failure"))
    shutil.copyfile(output / "jpeg-baseline.jpg", output / "mismatched-extension.png")
    records.append(record(output / "mismatched-extension.png", "image", "JPEG data with PNG extension", "content-detected preview"))
    return records


def generate_archive_payload(root: Path) -> None:
    payload = root / "archive-payload"
    (payload / "nested" / "deep").mkdir(parents=True, exist_ok=True)
    (payload / "empty-file.txt").write_bytes(b"")
    (payload / "hello.txt").write_text("PreviewAll archive fixture\n", encoding="utf-8")
    (payload / "unicode-中文-é.txt").write_text("Unicode path and content: 中文, café, Ελληνικά\n", encoding="utf-8")
    (payload / "nested" / "deep" / "data.json").write_text('{"preview": true, "count": 3}\n', encoding="utf-8")
    many = payload / "many-files"
    many.mkdir(exist_ok=True)
    for index in range(1000):
        (many / f"entry-{index:04d}.txt").write_text(f"entry {index}\n", encoding="ascii")


def generate_base_archives(root: Path) -> list[dict]:
    output = root / "archives"
    payload = root / "archive-payload"
    output.mkdir(parents=True, exist_ok=True)
    records: list[dict] = []

    def write_zip(name: str, members: list[Path]) -> None:
        with zipfile.ZipFile(output / name, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
            for item in members:
                if item.is_file():
                    archive.write(item, item.relative_to(payload).as_posix())

    all_files = sorted(path for path in payload.rglob("*") if path.is_file())
    write_zip("plain.zip", all_files)
    records.append(record(output / "plain.zip", "archive", "ZIP with nested, Unicode, empty, and 1000 files", "tree preview"))
    write_zip("unicode-nested.zip", [path for path in all_files if "many-files" not in path.parts])
    records.append(record(output / "unicode-nested.zip", "archive", "Small Unicode and nested ZIP", "tree preview"))
    with zipfile.ZipFile(output / "empty.zip", "w"):
        pass
    records.append(record(output / "empty.zip", "archive", "Empty ZIP", "empty tree preview"))
    return records


RICH_MARKDOWN = r"""# PreviewAll Markdown fixture

Paragraph with **bold**, *italic*, ***combined***, ~~strikethrough~~, `inline code`, an escaped \*asterisk\*, and Unicode: 中文 café Ελληνικά 🚀.

## Links and image

[Qt](https://www.qt.io/) and an automatic link: <https://commonmark.org/>.

![Local fixture](../images/png-rgb.png "Local image")

> Blockquote level one
>
> > Nested blockquote with **formatting**.

## Lists

1. Ordered item
2. Ordered item
   1. Nested ordered item

- Unordered item
  - Nested item
- [x] Completed task
- [ ] Pending task

## Table

| Format | Supported | Notes |
|:--|:--:|--:|
| PNG | yes | alpha |
| GIF | yes | animation |
| SVG | yes | vector |

## Code

```cpp
#include <QString>
QString greeting = QStringLiteral("Hello, PreviewAll");
```

```json
{"name": "PreviewAll", "enabled": true, "items": [1, 2, 3]}
```

    indented code block

---

## HTML

<details><summary>Raw HTML details</summary><p>HTML content.</p></details>

## Extended syntax samples

Footnote reference[^1], definition list-like text, math `$E = mc^2$`, and Mermaid as a fenced code block.

[^1]: Footnote text. Rendering depends on the Markdown implementation.

```mermaid
flowchart LR
    A[Explorer] --> B[PreviewAll]
```

Hard line break here.  
Next line.
"""


def write_sized_markdown(path: Path, target_size: int) -> None:
    header = "# Large Markdown fixture\n\n"
    block = "## Repeated section\n\n- item one\n- item two\n\n```text\n0123456789abcdef\n```\n\n"
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(header)
        while stream.tell() + len(block.encode("utf-8")) < target_size:
            stream.write(block)
        stream.write("End of fixture.\n")


def generate_markdown(root: Path) -> list[dict]:
    output = root / "markdown"
    output.mkdir(parents=True, exist_ok=True)
    records: list[dict] = []
    (output / "rich-syntax.md").write_text(RICH_MARKDOWN, encoding="utf-8", newline="\n")
    records.append(record(output / "rich-syntax.md", "markdown", "CommonMark and extended rich syntax", "render and source modes"))
    (output / "utf8-bom.markdown").write_text("# UTF-8 BOM\n\n中文 café 🚀\n", encoding="utf-8-sig")
    records.append(record(output / "utf8-bom.markdown", "markdown", "UTF-8 BOM and Unicode", "render without BOM artifact"))
    (output / "long-line.md").write_text("# Long line\n\n" + ("0123456789abcdef" * 8192) + "\n", encoding="utf-8")
    records.append(record(output / "long-line.md", "markdown", "Single 128 KiB line", "render and source modes"))
    write_sized_markdown(output / "large-near-limit.md", 7 * MIB + 512 * 1024)
    records.append(record(output / "large-near-limit.md", "markdown", "Approximately 7.5 MiB", "successful load"))
    write_sized_markdown(output / "large-over-limit.md", 9 * MIB)
    records.append(record(output / "large-over-limit.md", "markdown", "Over current 8 MiB limit", "intentional load failure"))
    return records


def record(path: Path, category: str, scenario: str, expected: str) -> dict:
    return {
        "path": path.as_posix(),
        "category": category,
        "scenario": scenario,
        "expected": expected,
    }


def finalize_manifest(root: Path, records: list[dict]) -> None:
    for item in records:
        path = Path(item["path"])
        item["path"] = path.relative_to(root).as_posix()
        item["bytes"] = path.stat().st_size
        item["sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
    manifest = {"archivePassword": PASSWORD, "fixtures": sorted(records, key=lambda item: item["path"])}
    (root / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--manifest-only", action="store_true")
    args = parser.parse_args()
    root = args.root.resolve()

    if not args.manifest_only:
        generate_archive_payload(root)
        records = generate_images(root)
        records += generate_base_archives(root)
        records += generate_markdown(root)
        (root / ".base-records.json").write_text(json.dumps(records, ensure_ascii=False), encoding="utf-8")
    else:
        records = json.loads((root / ".base-records.json").read_text(encoding="utf-8"))

    extra_records_path = root / ".archive-records.json"
    if extra_records_path.exists() and extra_records_path.stat().st_size:
        records += json.loads(extra_records_path.read_text(encoding="utf-8-sig"))
    finalize_manifest(root, records)


if __name__ == "__main__":
    main()

