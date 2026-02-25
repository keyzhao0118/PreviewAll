# Preview All

让 Windows 资源管理器的「预览窗格」支持更多文件类型：压缩包、图片、代码/文本等。

> 使用提示：Preview All 需要在后台托盘运行，资源管理器预览窗格才能正常显示。

## 效果预览（待补图）

> 你后续把截图/动图按文件名放到 `docs/media/` 目录即可，README 会自动显示。

**资源管理器预览窗格（代码）**

![资源管理器预览窗格 - 代码](docs/media/preview-pane-code.png)

**资源管理器预览窗格（压缩包）**

![资源管理器预览窗格 - 压缩包](docs/media/preview-pane-archive.png)

**资源管理器预览窗格（图片）**

![资源管理器预览窗格 - 图片](docs/media/preview-pane-image.png)

**一分钟演示动图（推荐）**

![一分钟演示动图](docs/media/previewall-demo.gif)

## 技术栈 / Tech Stack

| 类别 | 详情 |
|------|------|
| **编程语言** | C++17 |
| **构建系统** | CMake（≥ 3.20） |
| **UI 框架** | Qt 5（Widgets / Network / OpenGL） |
| **语法高亮** | KDE KF5SyntaxHighlighting |
| **压缩包解析** | 7-zip（通过 vcpkg 提供的 `7zip` 库） |
| **平台** | Windows（Win32 API、Shell Preview Handler COM 接口） |

> **一句话概括**：PreviewAll 是一个用 **C++17 + Qt 5** 编写的 Windows 系统托盘程序，通过注册 COM Preview Handler 接口，让 Windows 资源管理器的「预览窗格」原生支持图片（PNG/JPG/WebP/SVG/GIF 等）、压缩包（ZIP/RAR/7Z）和代码/文本（50+ 扩展名）的预览。

## 适用环境

- Windows 10/11（x64，建议）
- 使用 Windows 资源管理器的「预览窗格」（快捷键 `Alt + P`）

## 安装与首次使用

1. 下载发行版（Release）并解压到任意目录
2. 确保以下文件在同一目录（非常重要）
   - `PreviewAll.exe`
   - `PreviewAllHandler.dll`
3. 运行 `PreviewAll.exe`
4. 首次运行可能会弹出 UAC/管理员权限请求（用于注册预览处理器），请选择“是”
5. 在任务栏托盘找到 Preview All 图标，右键打开菜单，勾选你需要的预览类型：
   - Preview Image（图片）
   - Preview Archive（压缩包）
   - Preview Code（代码/文本）

完成后，在资源管理器中：

1. 按 `Alt + P` 打开「预览窗格」
2. 单击选中一个受支持的文件
3. 右侧预览窗格将显示内容

（待补图）

![托盘菜单（开关预览类型）](docs/media/tray-menu.png)

## 日常使用

- Preview All 会常驻托盘；**只要托盘程序在运行**，预览窗格就能正常工作。
- 你可以随时在托盘菜单中开关不同类别的预览。开关会被记住，下次启动会自动恢复。

## 支持的文件类型

以下扩展名来自当前版本的默认配置（大小写不敏感）：

### 图片

`.png` `.jpg` `.jpeg` `.tif` `.tiff` `.bmp` `.webp` `.ico` `.svg` `.gif`

### 压缩包

`.zip` `.rar` `.7z`

### 代码/文本

`.c` `.h` `.hpp` `.cpp` `.cxx` `.cc` `.mm` `.m` `.swift`

`.java` `.kt` `.kts` `.cs` `.fs`

`.py` `.rb` `.pl` `.lua` `.php`

`.js` `.ts` `.jsx` `.tsx` `.css` `.scss` `.html`

`.json` `.jsonc` `.yaml` `.yml` `.xml` `.toml` `.ini` `.conf`

`.cmake` `.mk` `.sh` `.ps1` `.bat` `.cmd`

`.md` `.rst` `.adoc` `.tex`

`.sql` `.diff` `.patch`

## 常见问题（FAQ）

### 1) 预览窗格没有任何变化 / 一直空白

请按顺序检查：

1. 资源管理器是否已打开「预览窗格」（`Alt + P`）
2. Preview All 是否仍在托盘运行
3. 托盘菜单里对应的预览类型是否已勾选
4. 关闭并重新打开资源管理器窗口（必要时可重启资源管理器进程）

### 2) 首次运行弹出管理员权限请求是做什么的？

Preview All 需要在系统中注册为“预览处理器（Preview Handler）”，资源管理器才能把文件交给它来预览。这个注册过程通常需要管理员权限。

如果你点了“否”，预览可能无法工作；你可以稍后重新注册：

```powershell
# 在“以管理员身份运行”的终端中执行
PreviewAll.exe --register-preview-handler
```

### 3) 为什么退出后预览又不生效了？

Preview All 采用托盘常驻模式：关闭/退出托盘程序后，会停止提供预览服务；再次启动后即可恢复。

## 卸载 / 停用

- 停用某类预览：在托盘菜单取消勾选对应项。
- 退出程序：托盘菜单选择 Exit。
- 删除程序：退出后直接删除解压目录即可。

## 反馈与支持

- 发现 bug 或想提需求：请在 GitHub Issues 中提交（建议附上 Windows 版本、复现步骤、相关文件类型/扩展名）。

## 截图/动图清单（你需要提供的素材）

请把素材放到 `docs/media/`，并使用以下文件名（不区分大小写但建议保持一致）：

- `preview-pane-code.png`：资源管理器预览窗格预览一份代码文件（例如 `.cpp` / `.ts` / `.json`）
- `preview-pane-archive.png`：预览窗格展示压缩包内容树（例如 `.zip`）
- `preview-pane-image.png`：预览窗格展示一张图片（例如 `.png`）
- `tray-menu.png`：托盘右键菜单，能看到三个开关项（Preview Image / Preview Archive / Preview Code）
- `previewall-demo.gif`（可选但强烈建议）：从“打开预览窗格 → 选中文件 → 展示预览”的一段 5~10 秒动图

建议：截图尽量包含资源管理器右侧的预览窗格与被选中的文件名；动图推荐 1080p 或更小的清晰录制，控制在 5~10MB。


