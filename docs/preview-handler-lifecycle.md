# 资源管理器预览切换的 COM 调用链实测

本文记录 2026-09-27 在真实 Windows 资源管理器预览窗格中的观察，供后续修改 Handler、IPC 和页面生命周期时参考。这里的次数和线程分配是本机样本，不是 Windows 对所有版本和场景的保证。

## 测试方式

- 使用 Release 版 `PreviewAll.exe` 和已注册的 `PreviewAllHandler.dll`；托盘已启用图片、压缩包和 Markdown 扩展名。通过资源管理器预览窗格连续选择测试文件，包含同类型和跨类型切换。
- 将六个样本放在同一目录，顺序是 PNG、JPEG、ZIP、7Z、Markdown、较慢的 Markdown，并多轮正向、反向选择。另有一组在 `test/archives` 内切换的记录。
- 可从仓库样本重建同一目录：把 `test/images/png-alpha.png`、`test/images/jpeg-baseline.jpg`、`test/archives/plain.zip`、`test/archives/plain.7z`、`test/markdown/rich-syntax.md`、`test/markdown/slow-syntax.md` 依次复制为 `out/com-trace-fixtures/01-image.png` 至 `06-markdown-slow.md`。`out` 是本地构建目录，不作为仓库资产提交。
- 临时插桩记录 Handler 的构造、`Initialize`、`SetWindow`、`DoPreview`、`Unload`、析构，以及 PreviewAll 接收 `CREATE` / `CLOSE` 的起止。每行记录 `GetTickCount64` 毫秒值、PID、线程 ID、Handler 对象地址和 HWND；两进程向同一文件追加完整行。时间戳只有毫秒精度，同一毫秒内以日志行序观察先后。
- 基线使用原有异步 `CLOSE`：Handler 等命令写入本地套接字，但不等 PreviewAll 执行 `close()`。临时日志和测试样本保存在忽略构建产物的 `out/` 下；本页保留必要的汇总和代表性事件。

## 基线观察

日志共 632 行，其中有 63 次 `DoPreview`、62 次 `Unload`，来自两个 `prevhost` 进程：

| prevhost PID / 线程 ID | Handler 地址 | 预览次数 | 覆盖类型 |
| --- | --- | ---: | --- |
| 30372 / 22040 | `0x28b100b7cf0` | 14 | 7Z、RAR、ZIP、Markdown |
| 28028 / 26232 | `0x1e360bd9840` | 49 | PNG、JPEG、ZIP、7Z、Markdown |

每个进程里的多次文件切换均复用同一个 Handler 对象和同一个线程。记录到的同类型转换包括 PNG→JPEG 7 次、ZIP→7Z 6 次、Markdown→Markdown 12 次；跨类型转换包括 JPEG→ZIP 4 次、7Z→Markdown 4 次、ZIP→Markdown 1 次等。没有观察到因三类文件间切换而更换 Handler 线程，也没有观察到格式专属的生命周期顺序。

典型顺序为：

```text
Handler: Initialize(file A) → SetWindow(parent) → DoPreview.begin
PreviewAll: CREATE.begin → CREATE.end
Handler: DoPreview.end
Handler: Unload.begin → Unload.end
PreviewAll: CLOSE.begin → CLOSE.end
Handler: Initialize(file B) → SetWindow(parent) → DoPreview.begin
```

这 62 次 `Unload` 中，有 61 次是 `Unload.end` 先于对应的 `CLOSE.end` 写入日志；例如旧页的 `Unload.end` 在毫秒值 263227031，PreviewAll 的 `CLOSE.end` 在 263227046，下一文件的 `Initialize` 在 263227062。样本中没有一次下一文件的 `DoPreview.begin` 早于旧页 `CLOSE.end`，但这是实际操作节奏下的观察，不是异步协议提供的顺序保证。异步 `Unload` 返回时，不能据此认定旧页面已关闭或旧任务已取消。

## 对同步关闭的设计约束

`Unload` 应在 PreviewAll 对对应 HWND 的页面调用 `cancelPreview()` 和 `close()` 之后收到 `CLOSED <HWND>`，再返回给预览宿主。`cancelPreview()` 立即使排队任务和结果失效；正在第三方解码或 7-Zip 调用中的工作可能稍后退出。确认不等待工作线程结束，也不等待原生子窗口销毁。

该实测没有理由给每次预览再引入独立 ID。现有 HWND 可定位在用页面；关闭确认前保留其原生窗口，避免旧 HWND 提早释放后复用。多预览窗格可以在不同 Handler/宿主线程中并存，因此 PreviewAll 仍按 HWND 分别管理页面，不应把单个 Handler 的实测串行性扩展成全局串行假设。

同步版在真实资源管理器中再次按同一组 PNG、JPEG、ZIP、7Z、Markdown 样本往返切换，三类页面均正常显示。对照日志中的代表性顺序是：

```text
264455500  Handler Unload.begin(723892)
264455500  PreviewAll CLOSE.begin(723892)
264455500  PreviewAll CLOSE.end(723892)
264455500  Handler Unload.end
264455515  Handler Initialize(01-image.png)
```

同一毫秒内以日志行序为准。同步确认使 `Unload.end` 落在 PreviewAll 的取消任务与 `close()` 调用之后；它不表示原生窗口已经销毁。页面在收到关闭请求后先隐藏，原生子窗口及 Qt 外部父窗口保留到 IPC 客户端断开连接后释放，避免在宿主线程等待回复期间销毁跨进程子窗口。

中途有一轮预览页未出现：日志显示 `CREATE.begin` 后创建流程提前返回，Handler 的 `DoPreview` 失败，未进入 `CLOSE`。该轮混用了反复重启的进程和窗口；之后重新启动 PreviewAll 和预览宿主，再在同一目录多轮切换未复现。现有记录不足以把这次创建失败归因于同步关闭。如果以后再次出现，应先在 `handleCreateCmd` 中分别记录 `QWindow::fromWinId`、`windowHandle()`、`GetParent` 和 `SetWindowSubclass` 的结果，再判断原因；不能添加定时重试来掩盖它。

本次插桩没有记录 `SetRect`、焦点和站点相关接口，不能从这些数据推断它们是否在切换中调用。需要研究尺寸或焦点问题时应针对对应接口另行采样。
