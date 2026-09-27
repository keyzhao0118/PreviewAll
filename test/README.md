# 预览样张目录

这里保存可直接在资源管理器预览窗格中选择的文件。格式范围以 `PreviewAll/preview/previewpagefactory.cpp` 为准；`manifest.json` 记录每份样张的用途、预期、大小和 SHA-256。测试前启动正式 `PreviewAll.exe`，在托盘启用三类预览，并打开资源管理器预览窗格。独立窗口不能代替宿主环境验收。

## 目录用途

| 目录 | 内容与重点 |
| --- | --- |
| `images/` | PNG、JPG、JPEG、TIF、TIFF、BMP、WebP、ICO、SVG、GIF；透明度、EXIF、动画首帧、尺寸/内存边界、损坏与扩展名不符 |
| `archives/` | ZIP、RAR、7Z；普通、空、Unicode/嵌套、内容加密、文件头加密、损坏和大量条目 |
| `markdown/` | MD、MARKDOWN；基础与扩展语法、内嵌图片、BOM、长行、接近及超过 8 MiB 上限 |
| `switching/` | `01`–`15` 排序的轻量样张，每个已注册扩展名恰好一份，格式交错排列，供 COM 调用链和快速切换观察 |

`switching/` 中按名称排序依次为 PNG → ZIP → MD → JPG → 7Z → MARKDOWN → JPEG → RAR → TIF → TIFF → BMP → WebP → ICO → SVG → GIF。先用方向键正向、反向逐个切换，再快速跨多个文件跳选，可同时观察同类与跨类切换。该目录的样张是从其他目录复制而来，特意保持较小，以便将切换时序与大文件加载耗时分开观察。样张是实际对应格式，不靠改扩展名冒充；专门测试扩展名不符的文件只放在 `images/`。

从仓库根目录直接打开切换目录：`& explorer.exe (Resolve-Path .\test\switching).Path`。在资源管理器中按 `Alt + P` 显示预览窗格。

## 慢加载与异常场景

| 文件 | 目的 | 预期 |
| --- | --- | --- |
| `images/slow-decode-6144x4096.jpg` | 25 MP 高细节渐进式 JPEG，给解码施压 | 显示统一加载态后出现图片 |
| `archives/slow-many-entries.zip` | 25 万条目，给枚举和建树施压 | 显示统一加载态后可展开目录树 |
| `markdown/slow-syntax.md` | 约 4 MiB 的重复表格/富语法，给 Markdown 解析施压 | 显示统一加载态后出现渲染文档 |
| `markdown/large-near-limit.md` | 接近当前 8 MiB 上限 | 可以预览 |
| `markdown/large-over-limit.md` | 超过当前 8 MiB 上限 | 明确显示无法预览 |
| `archives/password-header.7z`、`archives/password-header.rar` | 文件头加密 | 直接提示无法预览，不要求输入密码 |
| `archives/password-content.*`、`archives/password-aes.zip`、`archives/password-zipcrypto.zip` | 仅内容加密、目录可见 | 显示目录树，不要求输入密码 |
| `images/corrupt-truncated.png`、`archives/corrupt-truncated.*` | 损坏文件 | 显示失败状态，常驻进程继续正常工作 |

慢样张制造真实解码或解析工作，不人为睡眠。是否超过加载提示的约 150 毫秒阈值取决于机器、缓存和调度；验收时应同时观察加载中切换/关闭是否迅速，以及旧结果是否误入新页面。大尺寸图片和接近上限的 Markdown 用于观察内存、布局和关闭后的残留任务。GIF/WebP 动画目前显示首帧；空 ZIP 应显示空状态。

## 生成与校验

在仓库根目录运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\test\generate.ps1
```

需要 Python 与 Pillow。ZIP/图片/Markdown 由 Python 生成；7Z 和加密 ZIP 使用 7-Zip，RAR 使用 WinRAR。默认保留已有压缩包以维持提交样张的字节稳定；加 `-RegenerateArchives` 才重建它们，此时必须有 7-Zip 与 WinRAR。缺少压缩工具时，脚本会保留并校验仓库已提交的对应样张；若样张也不存在则失败，不会写出不完整清单。可传入 `-Python <路径>`，或用 `-DownloadTools` 在本机下载命令行工具。生成中间文件位于忽略的 `out/test-fixture-work/`，工具下载位于忽略的 `test/.tools/`，都不属于样张。

只校验现有文件而不重新生成大样张：

```powershell
python .\test\generate.py --verify
```

校验会逐项比对大小与 SHA-256、检测清单外样张，并检查 `switching/` 是否与当前注册扩展名一一对应。样张生成规则见 [AGENTS.md](AGENTS.md)。加密样张的统一测试密码为 `PreviewAll-Test-123!`；预览流程不会要求输入密码。
