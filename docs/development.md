# 本地开发与工程结构

## 工程如何工作

本工程是 Windows x64 / C++17 / Qt 6 程序，CMake 同时构建两个产物：

| 目录 / 产物 | 职责 |
| --- | --- |
| `PreviewAll` / `PreviewAll.exe` | Qt Widgets 托盘程序；管理文件扩展名注册、预览窗口、翻译与用户设置 |
| `PreviewAllHandler` / `PreviewAllHandler.dll` | COM `IPreviewHandler`、`IInitializeWithFile`、`IObjectWithSite`、`IOleWindow` 实现，由 Windows 预览宿主加载 |
| `PreviewAll/main.cpp`、`PreviewAll/app` | 程序入口、Qt 进程生命周期、托盘菜单、扩展名注册与预览窗口嵌入；不分派格式 |
| `PreviewAll/preview/common` | 格式内容页基类 `PreviewPage`、统一标题栏与状态栈、`PreviewTask` 和 `PreviewTaskQueue` |
| `PreviewAll/preview/previewwidget.*` | 唯一宿主窗口 `PreviewWidget`，组合公共控件和格式内容页 |
| `PreviewAll/preview/previewpagefactory.*` | 后缀到格式内容页的分派，以及托盘注册共用的扩展名目录 |
| `PreviewAll/preview/image` | 工作线程使用 `QImageReader` 解码，`QOpenGLWidget` 绘图；支持滚轮缩放和拖动 |
| `PreviewAll/preview/archive` | 动态加载 `7zip.dll` 的 `CreateObject`，使用 7-Zip COM 接口枚举文件树；树形目录按需展开 |
| `PreviewAll/preview/markdown` | 工作线程读取并解析 Markdown，Qt `QTextBrowser` 显示结果；文件大小上限 8 MiB |
| `PreviewAll/resources`、`PreviewAll/translations` | Qt 资源、Windows 程序图标和中英文翻译 |
| `installer/setup.iss` | Inno Setup 安装包，递归打包 Release 的 `bin`，写入 COM 注册信息 |
| `test` | 已提交的手工测试样本和 Python / PowerShell 再生成脚本 |
| `docs/index.html` | 静态介绍页面，不参与 C++ 构建 |

预览代码的依赖方向为 `app → preview/PreviewWidget → preview/previewpagefactory → preview/{image,archive,markdown} → preview/common`。`PreviewWidget` 只通过工厂获得内容页，不判断具体格式；`PreviewAllHandler` 是独立的 COM 目标，只通过 IPC 与主程序交互。新格式在 `PreviewAll/preview` 下新增同级目录，并加入工厂和 CMake 的 `PREVIEW_FORMAT_SOURCES`。

创建调用链为：资源管理器 → Windows COM 预览宿主 → Handler DLL → `QLocalSocket` → 托盘程序的 `QLocalServer` → 对应 Qt 预览组件。
通信协议只有 `CREATE` 和 `CLOSE`，文件路径使用 UTF-8 + Base64，窗口句柄使用十进制字符串。`CREATE` 返回预览页 HWND；Handler 在 `Unload` 时发送 `CLOSE <HWND>`，整个连接、发送和回复过程最多等待 250 ms。收到 `CLOSED <HWND>` 才表示该页面已调用 `close()` 且后台任务已取消；此确认不等待工作线程或原生子窗口销毁。无论确认成功、超时或通信失败，Handler 都清空本地 HWND 并返回；超时和失败会记录警告，不能据此认定托盘已处理关闭。真实资源管理器的切换调用链和线程观测见 [preview-handler-lifecycle.md](preview-handler-lifecycle.md)。

本机实测中，同一窗格连续切换这三类格式会复用同一 Handler，并在其原线程先完成 `Unload` 再初始化下一文件；Handler 在引用计数归零时才析构，关闭并重开资源管理器后可以创建新实例和新线程。页面取消必须在 `Unload` 对应的关闭流程中完成，不能依赖 Handler 析构。此观察不能推广成多窗格的全局串行保证。

当前实现按先 `SetWindow`、后 `DoPreview` 的流程工作：Handler 记录宿主窗口和预览区域，托盘程序通过 `QWindow::fromWinId` 包装宿主 HWND，再用 `QWindow::setParent` 建立 Qt 与 Win32 一致的父子窗口关系。宿主调用 `SetRect` 时，Handler 在临时的 Per-Monitor v2 DPI 上下文中把宿主提供的物理像素矩形同步给子窗口，然后恢复原线程上下文；拖动分隔线只走这一条尺寸更新路径。跨不同缩放屏幕时，Qt 会处理子窗口的 `WM_DPICHANGED_AFTERPARENT` 并更新原生 DPI；托盘程序随后按资源管理器顶层宿主窗口所在显示器同步外部父 `QWindow` 的 `QScreen`，让 Qt 子窗口的字体和布局使用新屏幕的缩放比例，最后按宿主客户区尺寸对齐一次。不能用预览子窗格自身的显示器归属判断，因为向左拖动时，资源管理器 DPI 已经切换，而子窗格的大部分区域可能仍留在原屏幕。普通移动、Qt 的 Resize/Move 事件和分隔线拖动均不触发额外校正，也不增加 IPC。

当前实际注册的扩展名如下：图片 `.png`、`.jpg`、`.jpeg`、`.tif`、`.tiff`、`.bmp`、`.webp`、`.ico`、`.svg`、`.gif`；压缩包 `.zip`、`.rar`、`.7z`；Markdown `.md`、`.markdown`。三个预览共用同一个标题栏，内容区分别只提供图片缩放/拖动、压缩包树形目录和 Markdown 渲染。

`PreviewTask` 管理每个页面的一次加载：关闭窗格先调用页面的 `cancelPreview()`，排队任务不会启动，结果只在页面仍存活且任务未取消时回到 UI。`PreviewTaskQueue` 内部使用 Qt 线程池，但页面不直接接触线程池；普通图片解码和 Markdown 解析走 `Responsive` 通道，可能被 7-Zip 内部调用阻塞的压缩包解析走 `Blocking` 通道。每条通道最多同时执行 2 项、等待 16 项；已取消的排队任务会被清走，队列满时页面显示明确失败状态。预览页面关闭不等待后台调用返回，整个程序退出时才等待残余工作结束。解析压缩包遇到加密文件头会直接结束并显示无法预览，不再等待密码输入。底层 7-Zip 正在打开文件时无法被中断，因此该次调用返回前仍占用一个解析槽位。GIF/WebP 动画文件目前显示首帧。`PreviewWidget` 使用 `PreviewContentStack` 统一加载态，短时间内完成时不显示加载动画。

Markdown 使用 Qt 6 `QTextDocument::setMarkdown` 的 GitHub dialect，文件读取和文档生成均放在工作线程，完成后再把文档交给 UI 线程中的 `QTextBrowser`。内嵌图片随窗格宽度缩小，长代码行在窄窗格中折行。最终验收在资源管理器预览窗格中进行；这不是完整的 CommonMark/GFM 兼容性实现，暂不宣称支持所有扩展语法。

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
| `qttranslations` | Qt Widgets 自带菜单等界面文字的多语言翻译，由 windeployqt 部署 |
| `7zip` | 7-Zip 头文件、导入库、运行时 DLL |

间接依赖由 vcpkg 自动安装。当前不需要 Qt Creator、QML、WebEngine 或 Python 才能编译主程序。
Python + Pillow 仅用于重新生成测试样本，Inno Setup 6 仅用于打包。

`vcpkg-configuration.json` 指定了 `cmake/vcpkg-ports/opengl`：保留上游从 Windows SDK 安装 OpenGL 头文件和导入库的流程，移除本项目不使用的完整 Khronos 注册表仓库依赖。Qt 使用自身的 OpenGL 扩展头文件。更新 baseline 时应同时检查此覆盖包；若以后直接使用 Khronos XML、GLES 等头文件，应恢复上游包。

## 首次构建与日常迭代

在项目根目录的普通 PowerShell 中执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Debug -UseSystemTools
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Release -UseSystemTools
```

脚本会检测工具链，克隆固定版本的官方 vcpkg，安装依赖，配置并编译。首次需联网并编译 Qt，时间明显长于后续增量构建。
脚本检测到原生工具失败即终止；解决错误后可以再次执行同一命令，下载和已编译包会复用。

`-UseSystemTools` 复用本机 CMake、Visual Studio 附带的 Ninja、`C:\Program Files\7-Zip` 和 Git for Windows 附带的 Perl；保留这些工具的 PATH，避免 vcpkg 额外下载构建工具。
新机器若没有这些工具，可省略该参数，由 vcpkg 自动获取其指定版本。
构建完成后启动正式 `PreviewAll.exe`，在托盘启用对应类型，再直接在资源管理器预览窗格中验收。快速切换时观察旧页关闭确认、下一页加载、同类与跨类型切换、多窗格并存及慢文件加载中关闭；不要用独立窗口替代最终验收。

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

## 新预览类型的共用流程

新预览类型继承 `PreviewPage`，只创建自己的内容控件并实现 `startPreview()` 与 `cancelPreview()`。`PreviewWidget` 是唯一无边框宿主窗口，统一创建 `PreviewTitleBar` 与 `PreviewContentStack`。工厂根据后缀创建内容页，`PreviewWidget` 先注册页面、连接状态信号，再调用 `startPreview()`。把只含文件路径、配置等值类型数据的工作函数交给类型页持有的 `PreviewTask::start(receiver, worker, complete)`；默认进入 `Responsive` 通道，不易中断的第三方调用显式选择 `PreviewTaskQueue::Lane::Blocking`。`worker` 在有界后台线程中执行，`complete` 只在页面仍存活且任务未取消时回到 UI 线程。页面销毁时 `PreviewTask` 自动取消；同一页面重新加载前调用 `start` 会先取消旧任务。若 `start` 返回 `false`，页面必须发出 `failed`，确保加载态结束。解析器若有可唤醒的等待操作，可传入不会阻塞 UI 的 `unblock` 回调，它只在任务运行中取消时调用；取消不会强行终止已经进入第三方库的调用。

格式页解释解析结果，通过 `loading`、`ready`、`empty`、`failed` 信号报告面向用户的预览状态；`PreviewWidget` 将信号转给 `PreviewContentStack`，任务队列不判断文件语义。完成或失败都会停止加载动画，约 150 毫秒内完成的任务不会出现动画。不支持的后缀仍创建公共窗口并显示说明，系统只注册已支持的扩展名。扩展名列表与页面分派统一维护在 `previewpagefactory.cpp`。工作线程不能访问 Qt 页面控件，解析大型目录后的 UI 填充也需要分批或按需进行。压缩包树目前仅建立根目录条目，展开文件夹时才填充其直接子项。新增类型的资源预算、样本和资源管理器验收要求见根目录 `AGENTS.md`。

## 调试预览组件

预览内容页不提供脱离资源管理器宿主环境的独立入口，因此这里没有无需注册即可打开单个文件的调试命令。调试配置时让同目录 Handler DLL 的注册指向正在调试的 `bin`，再在资源管理器中打开预览窗格；注册路径的检查与写入见下一节。
更多测试样本和密码见 `test/README.md`。任何绕过 Explorer / COM 集成的预览都不构成验收。

## 资源管理器集成调试

不带参数运行 `PreviewAll.exe` 会检查同目录 Handler DLL 的注册路径；当前实现检查 HKCU 和 HKLM，若 HKLM 已指向当前 DLL，则会尝试直接补写 HKCU，仍缺失时才请求 UAC 并启动 `--register-preview-handler` 子进程。
首次启动时三类文件预览默认关闭：必须在托盘菜单中分别勾选“图片预览”“压缩包预览”“Markdown 预览”，才会将对应扩展名的预览处理程序写入当前交互用户的 HKCU。勾选状态保存在用户设置中，后续启动时自动恢复并重新注册；退出时删除这些扩展名绑定。菜单以蓝色勾选图标表示已启用。

托盘“退出”在 UI 线程调用 `QCoreApplication::exit(0)`，结束 Qt 事件循环。随后 `main` 注销扩展名绑定，`PreviewAllApplication` 析构时释放预览页、取消页面任务，并等待残余工作结束。这里不能直接连接 `quit()`：嵌入后的预览页在 `QWidget` 层仍是顶层窗口，但对应的 `QWindow` 已是宿主的子窗口；Qt 的默认退出流程尝试调用 `QWindow::close()`，子窗口拒绝此调用，仍可见的页面使退出请求被忽略。`exit(0)` 仍保留正常的析构和任务收尾，不强行终止线程或进程。

退出回归应在真实资源管理器中分别覆盖：无预览页、三类样本已显示、慢样本正在加载，以及多个预览窗格同时打开。点击托盘“退出”后检查页面释放、进程结束、重新启动后仍能预览。若线程正在执行不可中断的第三方解码或打开调用，程序仍需等该次调用返回；这与 Qt 在退出入口忽略请求是不同问题。

本次退出修复已有包含活动嵌入页的正常收尾日志，证据及未覆盖的验收范围见 [托盘退出诊断记录](diagnostics/2026-10-01-tray-exit/README.md)。

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
