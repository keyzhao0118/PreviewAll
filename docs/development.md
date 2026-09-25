# 本地开发与工程结构

## 工程如何工作

本工程是 Windows x64 / C++17 / Qt 6 程序，CMake 同时构建两个产物：

| 目录 / 产物 | 职责 |
| --- | --- |
| `PreviewAll` / `PreviewAll.exe` | Qt Widgets 托盘程序；管理文件扩展名注册、预览窗口、翻译与用户设置 |
| `PreviewAllHandler` / `PreviewAllHandler.dll` | COM `IPreviewHandler`、`IInitializeWithFile`、`IObjectWithSite`、`IOleWindow` 实现，由 Windows 预览宿主加载 |
| `PreviewAll/previewtitlebar.*` | 三种预览共用的标题栏；显示 PreviewAll 图标、文件名，并用 `QDesktopServices` 调用系统默认关联打开原文件 |
| `PreviewAll/previewimage` | 工作线程使用 `QImageReader` 解码，`QOpenGLWidget` 绘图；支持滚轮缩放和拖动 |
| `PreviewAll/previewarchive` | 动态加载 `7zip.dll` 的 `CreateObject`，使用 7-Zip COM 接口枚举文件树；树形目录按需展开 |
| `PreviewAll/previewmd` | 工作线程读取并解析 Markdown，Qt `QTextBrowser` 显示结果；文件大小上限 8 MiB |
| `installer/setup.iss` | Inno Setup 安装包，递归打包 Release 的 `bin`，写入 COM 注册信息 |
| `test` | 已提交的手工测试样本和 Python / PowerShell 再生成脚本 |
| `docs/index.html` | 静态介绍页面，不参与 C++ 构建 |

创建调用链为：资源管理器 → Windows COM 预览宿主 → Handler DLL → `QLocalSocket` → 托盘程序的 `QLocalServer` → 对应 Qt 预览组件。
通信协议只有 `CREATE` 和 `CLOSE`，文件路径使用 UTF-8 + Base64，窗口句柄使用十进制字符串。
当前实现按先 `SetWindow`、后 `DoPreview` 的流程工作：Handler 先记录宿主窗口和预览区域，托盘程序创建 Qt 预览窗口后将它设为 Win32 子窗口，通过 `SetParent` 嵌入宿主。预览宿主随后调用 `SetRect` 时，Handler 直接按宿主给出的像素坐标同步调整子窗口边界，使拖动窗格分隔线时窗口边缘及时跟随。若父窗口与记录值不一致，则跳过该次尺寸调整。不按子窗口的 DPI 比例换算宿主尺寸。

当前实际注册的扩展名如下：图片 `.png`、`.jpg`、`.jpeg`、`.tif`、`.tiff`、`.bmp`、`.webp`、`.ico`、`.svg`、`.gif`；压缩包 `.zip`、`.rar`、`.7z`；Markdown `.md`、`.markdown`。三个预览共用同一个标题栏，内容区分别只提供图片缩放/拖动、压缩包树形目录和 Markdown 渲染。

图片解码与 Markdown 文件读取/语法解析共用一个最多 2 个线程的后台池；关闭窗格会标记任务取消，排队任务开始前会跳过，完成结果只在窗格仍存在时回到 UI。压缩包解析使用独立的最多 2 个线程的池，因为加密文件头可能等待用户输入密码；关闭预览会唤醒等待中的解析器并停止后续条目处理。底层 7-Zip 正在打开文件时无法被中断，因此该次调用返回前仍占用一个解析槽位。GIF/WebP 动画文件目前显示首帧。

Markdown 使用 Qt 6 `QTextDocument::setMarkdown` 的 GitHub dialect，文件读取和文档生成均放在工作线程，完成后再把文档交给 UI 线程中的 `QTextBrowser`。内嵌图片随窗格宽度缩小，长代码行在窄窗格中折行。运行时 smoke 检查标题、粗体、列表的基础解析、文档线程归属及窄窗格中的横向溢出；这不是完整的 CommonMark/GFM 兼容性验收，暂不宣称支持所有扩展语法。

## 工具链和依赖

本机使用 Visual Studio 2026 Community、MSVC x64、Windows SDK 10.0.26100.0、CMake 和 Git。
预设使用 `Visual Studio 18 2026` 生成器，要求 CMake 4.2 或更高版本；顶层 `cmake_minimum_required` 仅表示项目本身所需版本。
Visual Studio 应安装“使用 C++ 的桌面开发”及 Windows SDK。

所有直接依赖声明在根目录 `vcpkg.json`，由 `builtin-baseline` 固定版本，统一使用动态链接 `x64-windows`：

| 包 | 用途 |
| --- | --- |
| `qtbase` | Core、Gui、Widgets、Network、OpenGL、OpenGLWidgets、JPEG/PNG、windeployqt |
| `qtsvg` | SVG 图像解码和 SVG 图标加载 |
| `qtimageformats[tiff,webp]` | TIFF / WebP 运行时图片插件，不能仅凭 EXE 链接依赖判断是否需要 |
| `qttools[linguist]` | lupdate / lrelease，编译中英文翻译 |
| `7zip` | 7-Zip 头文件、导入库、运行时 DLL |

间接依赖由 vcpkg 自动安装。当前不需要 Qt Creator、QML、WebEngine 或 Python 才能编译主程序。
Python + Pillow 仅用于重新生成测试样本，Inno Setup 6 仅用于打包。

`vcpkg-configuration.json` 指定了 `cmake/vcpkg-ports/opengl`：保留上游从 Windows SDK 安装 OpenGL 头文件和导入库的流程，移除本项目不使用的完整 Khronos 注册表仓库依赖。Qt 使用自身的 OpenGL 扩展头文件。更新 baseline 时应同时检查此覆盖包；若以后直接使用 Khronos XML、GLES 等头文件，应恢复上游包。

## 首次构建与日常迭代

在项目根目录的普通 PowerShell 中执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Debug -UseSystemTools -Verify
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Release -UseSystemTools -Verify
```

脚本会检测工具链，克隆固定版本的官方 vcpkg，安装依赖，配置并编译。首次需联网并编译 Qt，时间明显长于后续增量构建。
脚本检测到原生工具失败即终止；解决错误后可以再次执行同一命令，下载和已编译包会复用。

`-UseSystemTools` 复用本机 CMake、Visual Studio 附带的 Ninja、`C:\Program Files\7-Zip` 和 Git for Windows 附带的 Perl；保留这些工具的 PATH，避免 vcpkg 额外下载构建工具。
新机器若没有这些工具，可省略该参数，由 vcpkg 自动获取其指定版本。
`-Verify` 会构建并运行 `PreviewAllSmoke.exe`：检查 Handler DLL 导出、图片解码插件和样本解码、Markdown 基础语法、异步文档线程归属与窄窗格布局、ZIP/7Z/RAR 与加密 7Z 解析、翻译加载。验证不写注册表；该测试程序不进入安装包。

所有自动生成内容都位于 Git 忽略的 `out`：

```text
out/tools/vcpkg             固定版本依赖管理器及包构建中间文件
out/downloads              下载缓存
out/vcpkg-cache            已编译依赖缓存
out/vcpkg_installed         Debug/Release 共用的依赖安装目录
out/build/x64-debug/bin     Debug EXE、Handler DLL、Qt DLL/插件、翻译
out/build/x64-release/bin   Release 产物，也是安装包输入目录
```

初始化后，只修改 C++ 代码时可以直接增量编译：

```powershell
cmake --build --preset x64-debug --parallel
cmake --build --preset x64-release --parallel
```

修改依赖清单或需要重新配置时重新运行 `build.ps1`。修改依赖版本时应同时更新 `builtin-baseline`，不要单独在 vcpkg 工具目录切换版本。
本机已生成 Git 忽略的 `CMakeUserPresets.json`。使用 Visual Studio“打开文件夹”打开项目根目录，选择 `local-debug` 或 `local-release` 可复用本机工具及同一构建目录。命令行也可以使用 `cmake --preset local-debug`、`cmake --build --preset local-debug`。
公共的 `x64-debug` / `x64-release` 预设不强制使用系统工具，适合通过 `build.ps1` 驱动或让 vcpkg 下载其指定工具的环境。
机器专属设置可以放在已忽略的 `CMakeUserPresets.json`，不要把个人绝对路径写回公共预设。

普通构建只将 `.ts` 编译成 `.qm`，不会改写翻译源。新增或修改 `tr()` 文本后显式执行：

```powershell
cmake --build --preset x64-debug --target PreviewAll_update_translations
# 编辑 PreviewAll/translations/*.ts 后再构建
```

## 调试预览组件

无需注册 COM、启动托盘或安装到系统，直接运行：

```powershell
& .\out\build\x64-debug\bin\PreviewAll.exe --preview .\test\images\png-alpha.png
& .\out\build\x64-debug\bin\PreviewAll.exe --preview .\test\archives\plain.7z
& .\out\build\x64-debug\bin\PreviewAll.exe --preview .\test\markdown\rich-syntax.md
```

该入口显示独立、可关闭的窗口；在 Visual Studio 中可将上述参数设为调试参数。它绕过注册表和托盘初始化，仍使用实际的三个预览组件。
更多测试样本和密码见 `test/README.md`。独立预览通过不代表 Explorer / COM 集成已经验证。

## 资源管理器集成调试

不带参数运行 `PreviewAll.exe` 会检查同目录 Handler DLL 的注册路径；当前实现检查 HKCU 和 HKLM，若 HKLM 已指向当前 DLL，则会尝试直接补写 HKCU，仍缺失时才请求 UAC 并启动 `--register-preview-handler` 子进程。
首次启动时三类文件预览默认关闭：必须在托盘菜单中分别勾选“图片预览”“压缩包预览”“Markdown 预览”，才会将对应扩展名的预览处理程序写入当前交互用户的 HKCU。勾选状态保存在用户设置中，后续启动时自动恢复并重新注册；退出时删除这些扩展名绑定。菜单以蓝色勾选图标表示已启用。

正式验收应先运行 Release 托盘进程并勾选对应类型，再在资源管理器中启用“预览窗格”并选择 `test` 中的样本。
调试不同配置时，注册路径需要指向正在调试的 `bin`。

Windows 预览宿主可能持有 Handler DLL：如果出现 LNK1104 / DLL 被占用，先关闭预览窗格、退出自己的 PreviewAll 进程，再结束持有该 DLL 的 `prevhost.exe` 后重试。
不要在 Debug 和 Release 程序之间同时使用同一个单实例互斥量 / 本地服务名。
Handler 没有导出 `DllRegisterServer`，不能使用 `regsvr32` 注册；请使用主程序注册流程或安装包。

当前注册代码会覆盖已有扩展名绑定，退出时不会恢复旧值；集成调试前请留意已有预览工具。该行为是原实现的一部分，本次环境配置没有改动它。

## 安装包

先完成 Release 构建，再在项目根目录执行：

```powershell
& 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe' .\installer\setup.iss
```

产物位于 `installer/Output/PreviewAll_Setup_1.0.0.exe`。编译安装包不等于执行安装；运行安装包才会修改系统注册信息。
发布前需要另行验证 Explorer 集成、干净机器运行和第三方许可证随包分发。

## 参考

- [vcpkg manifest 模式](https://learn.microsoft.com/vcpkg/consume/manifest-mode)
- [Qt Windows 部署](https://doc.qt.io/qt-6/windows-deployment.html)
