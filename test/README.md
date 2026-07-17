# PreviewAll 测试文件说明

本目录用于保存 PreviewAll 各类预览功能的手工测试文件。测试文件可重复生成，生成结果记录在 `manifest.json` 中，包括相对路径、文件大小、SHA-256、测试场景和预期结果。

## 重新生成

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\test\generate.ps1
```

脚本会使用本机 Python 生成基础图片、Markdown 和 ZIP 文件；如果本机可找到 7-Zip 或 WinRAR 命令行工具，还会生成 7Z、加密 ZIP 和 RAR5 文件。

如果缺少 7-Zip 或 WinRAR，可允许脚本下载官方命令行工具到 `test/.tools`：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\test\generate.ps1 -DownloadTools
```

`test/.tools` 不纳入 Git。

## 加密压缩包密码

所有加密压缩包使用同一个测试密码：

```text
PreviewAll-Test-123!
```

## 文件分类

- `images/`：图片预览测试文件，覆盖 PNG、JPEG/JPG、TIFF/TIF、BMP、WebP、ICO、SVG、GIF、透明通道、动画、EXIF 旋转、渐进式 JPEG、多页 TIFF、损坏文件、9000x9000 大内存图片和 20000x2000 超宽图片。
- `archives/`：压缩包预览测试文件，覆盖 ZIP、7Z、RAR5、内容加密、文件头加密、Unicode/嵌套路径、空文件、大量条目、空压缩包和损坏压缩包。
- `markdown/`：Markdown 预览测试文件，覆盖常见 Markdown 语法、本地图片引用、原始 HTML、表格、任务列表、代码块、Unicode、超长行、UTF-8 BOM、接近 8 MiB 限制的大文件和超过 8 MiB 限制的大文件。

## 预期行为

- 普通图片应能正常加载、缩放并适应预览区域。
- 大尺寸图片用于验证图片预览的内存占用和加载稳定性。
- 损坏图片和损坏压缩包应显示加载失败状态。
- 普通压缩包应显示文件树。
- 加密内容压缩包应能看到文件列表，但读取内容时需要密码。
- 文件头加密压缩包在列出文件前就应要求密码。
- `markdown/large-near-limit.md` 应能加载。
- `markdown/large-over-limit.md` 按当前 8 MiB 限制应显示加载失败。