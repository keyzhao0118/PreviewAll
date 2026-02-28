# Preview All

让 Windows 资源管理器的「预览窗格」支持更多文件类型：图片、压缩包、代码/文本等。

> Preview All 需要在后台托盘运行，资源管理器预览窗格才能正常显示。

## 预览能力

### 图片预览

在资源管理器预览窗格中直接查看图片内容，支持 PNG、JPEG、BMP、WebP、SVG、GIF、TIFF、ICO 等常见格式。预览窗格会根据窗口大小自适应缩放，完整展示图片。

### 压缩包预览

无需解压即可查看压缩包内部的文件结构。以树形目录的方式展示压缩包中的文件和文件夹层级，并显示每个条目的文件名、大小等信息，支持 ZIP、RAR、7z 格式。

### 代码/文本预览

在预览窗格中以语法高亮的方式展示源代码和文本文件内容，覆盖 50+ 种编程语言和配置文件格式（C/C++、Python、Java、JavaScript/TypeScript、Markdown、JSON、YAML 等），方便快速浏览代码而无需打开编辑器。

## 工作原理

Preview All 采用 **进程外（out-of-process）** 架构，由两个核心组件协同工作：

```
资源管理器
  │  选中文件
  ▼
PreviewAllHandler.dll（COM 组件，被加载到 prevhost.exe）
  │  通过 QLocalSocket 命名管道发送 CREATE / RESIZE / CLOSE 命令
  ▼
PreviewAll.exe（Qt 托盘程序，常驻后台）
  │  根据文件扩展名创建对应的 Qt 预览控件
  │  通过 Win32 SetParent() 嵌入资源管理器窗口
  ▼
预览窗格中显示内容
```

- **PreviewAllHandler.dll** 实现了 Windows 标准的 `IPreviewHandler` / `IInitializeWithFile` COM 接口，资源管理器通过它获取预览窗口。DLL 本身不做渲染，仅作为通信桥梁。
- **PreviewAll.exe** 监听命名管道，收到请求后创建对应的 Qt Widget（图片查看器 / 压缩包树 / 代码高亮编辑器），并将窗口句柄嵌入资源管理器提供的父窗口。
- 新增文件类型只需在 `PreviewAll.exe` 中添加渲染模块，**COM DLL 无需任何改动**，扩展成本极低。

## 技术栈

| 类别 | 详情 |
|------|------|
| **编程语言** | C++17 |
| **构建系统** | CMake ≥ 3.20 + Ninja |
| **包管理** | vcpkg |
| **UI 框架** | Qt 5（Widgets / Network / OpenGL） |
| **语法高亮** | KDE KF5SyntaxHighlighting |
| **压缩包解析** | 7-zip（vcpkg 提供的 `7zip` 库） |
| **进程间通信** | QLocalSocket（命名管道） |
| **平台接口** | Win32 API、Shell Preview Handler COM 接口 |

## 适用环境

- Windows 10 / 11（x64）
- 资源管理器「预览窗格」（快捷键 `Alt + P`）

## 安装与首次使用

1. 从 [Releases](../../releases) 下载最新版并解压到任意目录
2. 确保以下文件在同一目录：
   - `PreviewAll.exe`
   - `PreviewAllHandler.dll`
   - `7zip.dll`
3. 运行 `PreviewAll.exe`
4. 首次运行会弹出 UAC 管理员权限请求（用于注册预览处理器），请选择"是"
5. 在任务栏托盘找到 Preview All 图标，右键打开菜单，勾选你需要的预览类型：
   - **Preview Image** — 图片预览
   - **Preview Archive** — 压缩包预览
   - **Preview Code** — 代码/文本预览

完成后，在资源管理器中按 `Alt + P` 打开预览窗格，单击选中一个受支持的文件，右侧即可显示预览内容。

## 日常使用

- Preview All 常驻托盘；**托盘程序运行期间**，预览窗格即可正常工作。
- 托盘菜单中可随时开关不同类别的预览，开关状态会自动持久化，下次启动自动恢复。
- 程序使用 Windows 全局互斥量保证单实例运行，不会重复启动。

## 支持的文件类型

以下扩展名均大小写不敏感：

### 图片（10 种）

`.png` `.jpg` `.jpeg` `.tif` `.tiff` `.bmp` `.webp` `.ico` `.svg` `.gif`

### 压缩包（3 种）

`.zip` `.rar` `.7z`

### 代码/文本（50+ 种）

| 分类 | 扩展名 |
|------|--------|
| C/C++/ObjC | `.c` `.h` `.hpp` `.cpp` `.cxx` `.cc` `.mm` `.m` |
| Swift | `.swift` |
| Java/Kotlin | `.java` `.kt` `.kts` |
| C#/F# | `.cs` `.fs` |
| 脚本语言 | `.py` `.rb` `.pl` `.lua` `.php` |
| Web 前端 | `.js` `.ts` `.jsx` `.tsx` `.css` `.scss` `.html` |
| 配置/数据 | `.json` `.jsonc` `.yaml` `.yml` `.xml` `.toml` `.ini` `.conf` |
| 构建/Shell | `.cmake` `.mk` `.sh` `.ps1` `.bat` `.cmd` |
| 文档/标记 | `.md` `.rst` `.adoc` `.tex` |
| 其他 | `.sql` `.diff` `.patch` |

## 从源码构建

### 前置条件

- Visual Studio 2019+ 或 MSVC Build Tools（需要 C++17 支持）
- [CMake](https://cmake.org/) ≥ 3.20
- [vcpkg](https://github.com/microsoft/vcpkg)（设置 `VCPKG_ROOT` 环境变量）
- 通过 vcpkg 安装以下依赖：

```powershell
vcpkg install qt5-base qt5-network qt5-opengl 7zip kf5syntaxhighlighting --triplet x64-windows
```

### 构建步骤

```powershell
cmake --preset x64-debug      # 或 x64-release
cmake --build out/build/x64-debug
```

构建产物输出到 `out/build/<preset>/bin/` 目录，包含 `PreviewAll.exe`、`PreviewAllHandler.dll` 和 `7zip.dll`。

### 手动注册预览处理器

如果首次运行时跳过了 UAC 授权，可以手动注册：

```powershell
# 以管理员身份运行
.\PreviewAll.exe --register-preview-handler
```

## 常见问题

### 预览窗格没有任何变化 / 一直空白

请按顺序检查：

1. 资源管理器是否已打开预览窗格（`Alt + P`）
2. Preview All 是否仍在托盘运行
3. 托盘菜单里对应的预览类型是否已勾选
4. 关闭并重新打开资源管理器窗口（必要时可在任务管理器中重启 `explorer.exe`）

### 首次运行弹出管理员权限请求是做什么的？

Preview All 需要将 `PreviewAllHandler.dll` 注册为系统的 Preview Handler COM 组件（写入 `HKEY_LOCAL_MACHINE` 和 `HKEY_CURRENT_USER` 下的注册表项），资源管理器才能识别并加载它。

### 为什么退出后预览又不生效了？

Preview All 的渲染由托盘程序 `PreviewAll.exe` 提供。退出托盘程序后，COM DLL 无法通过命名管道获得预览窗口，因此预览会失效。重新启动即可恢复。

## 卸载 / 停用

- **停用某类预览**：在托盘菜单取消勾选对应项。
- **退出程序**：托盘菜单选择 Exit（退出时会自动反注册所有文件扩展名关联）。
- **删除程序**：退出后直接删除整个目录即可。

## Roadmap / 未来规划

### 第一阶段：补齐核心体验

- [ ] **Markdown 渲染预览**：当前 `.md` 以语法高亮的源码形式展示，应增加渲染模式（基于 Qt WebEngine 或自研 Markdown→HTML），支持源码/渲染双模式切换。
- [ ] **代码预览增强**：添加行号显示、文字搜索（Ctrl+F）、亮色/暗色主题切换。
- [ ] **图片预览增强**：显示图片基础信息（分辨率、文件大小、色彩空间），支持 EXIF 元数据展示，增加实际像素 1:1 与适应窗口的快捷切换。
- [ ] **压缩包预览增强**：支持在树形视图中搜索/过滤文件名；支持预览压缩包内的单个文件（点击条目后就地预览）。

### 第二阶段：扩展更多文件类型

得益于进程外渲染架构，新增预览类型只需在 PreviewAll.exe 中添加对应的 Qt Widget 模块，COM Handler DLL 无需任何改动。

- [ ] **PDF 预览**：利用 Qt PDF 模块或 MuPDF / Poppler 实现翻页预览。
- [ ] **Office 文档预览**：轻量级 `.docx` / `.xlsx` / `.pptx` 预览（可借助 LibreOffice 无头模式或解析 OOXML）。
- [ ] **字体文件预览**：`.ttf` / `.otf` / `.woff` / `.woff2`，展示字体样张、字符集和元信息。
- [ ] **3D 模型预览**：`.stl` / `.obj` / `.gltf`，利用 Qt 3D 或 OpenGL 做基础的旋转/缩放查看。
- [ ] **音视频缩略预览**：`.mp4` / `.mp3` / `.wav` 等，展示视频关键帧缩略图或音频波形图。

### 第三阶段：架构升级与插件化

- [ ] **插件系统**：将每种预览类型抽象为独立插件（动态库），定义标准 `IPreviewPlugin` 接口，支持第三方开发者编写自定义预览插件并热加载。
- [ ] **设置界面**：独立的设置窗口，提供文件类型管理（自定义扩展名绑定）、主题选择（亮色/暗色/跟随系统）、字体大小、快捷键配置等。
- [ ] **性能优化**：大文件异步加载与流式渲染；频繁预览文件的缩略图缓存，提升文件切换响应速度。
- [ ] **多语言完善**：补齐英文及其他语言的翻译（当前已有 i18n 框架，支持中文和英文）。

### 第四阶段：分发与生态

- [ ] **安装包**：提供 MSI / MSIX 安装包，支持静默安装和开机自启。
- [ ] **包管理器分发**：发布到 `winget` / `scoop` / `chocolatey`，一行命令安装。
- [ ] **自动更新**：内置版本检查与自动更新（基于 GitHub Releases API）。
- [ ] **开机自启**：托盘菜单选项或安装时选项，注册到 Windows 启动项。

## 反馈与支持

发现 bug 或想提需求，请在 [GitHub Issues](../../issues) 中提交（建议附上 Windows 版本、复现步骤、相关文件扩展名）。

## License

MIT
