# 资源管理器预览切换的 COM 调用链实测

本报告更新于 2026-09-28，使用当前 `test/switching/` 样张和真实资源管理器预览窗格，完成普通切换、快速切换、关闭及重新打开资源管理器的取证。结论是：**连续切换复用同一 Handler，并在其线程中先卸载旧预览再初始化新文件；引用计数归零才析构，窗口重开后本次观察到新实例和新宿主线程。** 下文区分实测、代码行为和 COM 契约，不把本机宿主的复用策略当作所有 Windows 环境的保证。

## 当前能够确认的结论

| 问题 | 本轮证据与结论 |
| --- | --- |
| 上一文件的 Unload 与下一文件的 DoPreview 是否在同一线程串行执行？ | 52 次实例内切换全部如此；旧 `Unload.end` 还早于新 `Initialize.begin`。这里记录的是 prevhost 内的执行线程。跨关闭/重开边界，下一实例的线程可以变化。 |
| 每次选文件是否新建 Handler？ | 没有。普通轮 39 次成功预览仅一次构造；快速切换也复用。全部 55 次成功预览共涉及 3 个实例，新构造发生在关闭/重开阶段。 |
| Unload 后是否立即析构 Handler？ | 普通和快速文件切换不会。三个实例都在最终释放引用时才析构；距各自最后一次外部 Unload 返回约 77.372、35.299、23.311 ms。 |
| 同类型与跨类型切换是否不同？ | 本轮没有区别。图片、压缩包、Markdown 的全部 9 种类型转换方向均有样本，都复用同一实例和线程。 |
| 析构由谁触发、在哪个线程执行？ | 三次均为 `Release()` 减至零后直接进入析构，线程均与该实例之前的 DoPreview/Unload 相同。 |
| 关闭再打开资源管理器是否重建实例？ | 本次观察到旧实例正常析构，随后在新的 prevhost PID/TID 中构造新实例。不能据此承诺每次重开都会更换进程，也未独立验证仅用 Alt+P 切换窗格的规则。 |

## 环境与取证方法

- 系统：Windows x64，DisplayVersion `25H2`，build `26200.9457`。
- 源代码：`d5427ae6279c6e33d41f4e21989bcf6943a57ed3`，已使用同步 `CLOSE` 确认。
- 运行程序：`out/build/x64-release/bin/PreviewAll.exe`，PID 3236；取证没有修改这个程序。
- 已注册的 Handler：`out/build/x64-release/bin/PreviewAllHandler.dll`，Release 构建，临时加入调用日志。三类托盘选项已启用。
- 15 个扩展名均绑定同一 CLSID `{A26D5A00-AF3F-47B7-B075-A3282DE904E6}`，注册的 `ThreadingModel=Apartment`、`DisableLowILProcessIsolation=1`。文件类型在 Qt 内容页工厂中分派，并不是不同的 COM Handler 类。
- 所有操作都在真实资源管理器中完成，没有使用独立 Smoke 或 `--preview` 窗口。当前自动操作工具无法列出用户打开的资源管理器窗口，因此由用户按指定步骤切换，日志从实际 `prevhost.exe` 采集；用户确认普通轮页面均正常。

临时插桩覆盖构造、析构、AddRef/Release、Initialize、站点、窗口、预览、尺寸和焦点相关接口。记录接口入口/出口、HRESULT、PID/TID、对象地址、当前 HWND、文件路径，使用 QPC 时间戳（频率 10,000,000 Hz）。另记录构造时的 QPC 值区分对象生命期，避免把内存地址重用误判为对象复用；这个字段只用于本次诊断，不是生产协议中的预览 ID。

[逐条事件、元数据及离线分析脚本](diagnostics/2026-09-28-handler-trace/README.md)已归档，共 2382 条事件；普通轮为前 1494 条。全量有 55 次成功 `DoPreview`、55 次成功外部 `Unload`，另有 3 次析构内部的空 `Unload`，不能将日志中的 58 次 Unload 全部计为文件切换。文件路径被规范化为仓库相对路径，其余字段和顺序保留。

## 普通轮：同实例、同线程、先卸载再初始化

操作顺序为按名称 `01→15→01` 往返，再做 `01→04→07→01`（图片）、`02→05→08→02`（压缩包）、`03→06→03`（Markdown）。每次稍停至页面出现，最后停在 `03-markdown.md`。以日志中实际执行的调用为统计依据，不根据按键次数推算预览次数。

| PID / TID | 对象地址 | 构造时 QPC | 构造 / 析构 | DoPreview 成功 / 外部 Unload 成功 |
| --- | --- | --- | --- | --- |
| 9040 / 2564 | `0x19f86848af0` | `2701072351123` | 1 / 0 | 39 / 38 |

三类之间的转换次数如下。每格对应同一实例上相邻的两次 `DoPreview`，中间都有完成的 `Unload`。

| 从 / 到 | 图片 | 压缩包 | Markdown |
| --- | ---: | ---: | ---: |
| 图片 | 15 | 5 | 2 |
| 压缩包 | 4 | 3 | 3 |
| Markdown | 2 | 2 | 2 |

省略引用计数和接口查询后的实际顺序：

```text
construct Handler H                         // 普通轮仅一次
Initialize(file A)
SetSite(site)
SetWindow(parent)
DoPreview.begin(A) → DoPreview.end(A)
    [可能有 SetRect]
Unload.begin(A) → Unload.end(A)              // 关闭确认成功，m_hwndPreview 变为 0
SetSite(nullptr)
    [QueryInterface / AddRef / Release，未归零]
Initialize(file B)                          // 仍然是 H、仍然是同一 TID
SetSite(site)
SetWindow(parent)
DoPreview.begin(B) → DoPreview.end(B)
```

例如 `01-image.png → 02-archive.zip` 的关键事件，均来自上述 PID/TID/实例：

| QPC | 事件 | 结果 |
| ---: | --- | --- |
| 2702003950053 | `Unload.begin(01-image.png)` | 当前 HWND 69408 |
| 2702003970824 | `Unload.end` | S_OK，当前 HWND 0 |
| 2702003972272 | `SetSite.begin(nullptr)` | 清除旧站点 |
| 2702004137122 | `Initialize.begin(02-archive.zip)` | 同一实例接收新文件 |
| 2702004141934 | `SetSite.begin(site)` | 设置站点 |
| 2702004144266 | `SetWindow.begin` | 父 HWND 462580 |
| 2702004150133 | `DoPreview.begin` | 开始新预览 |
| 2702004590321 | `DoPreview.end` | S_OK，新 HWND 1707824 |

这次 `Unload` 约耗时 2.077 ms，返回到下一次 `DoPreview` 入口间隔约 17.931 ms，仅用来展示先后关系。日志 I/O 会影响调度，不能把这些数值当作正式性能基准。

普通轮所有调用的入口/出口配对完整，没有失败的 `DoPreview` 或 `Unload`，没有旧 `Unload` 尚未返回就进入新 `Initialize`/`DoPreview` 的记录。也记录到一次 `SetRect`；没有记录到焦点接口调用，不表示其他操作场景不会调用它们。`QueryInterface` 探测未实现接口时返回 `E_NOINTERFACE` 属于正常协商，不能算作预览失败。

## 快速切换与关闭重开：复用和新建的边界

用户补做快速往返切换、关闭并重新打开资源管理器，最后选中 `01-image.png`；归档前又单独确认关闭最后一个测试窗口。补采增加 16 次成功预览。快速选择的中间文件没有全部进入 `DoPreview`，例如出现 PNG→RAR→GIF；这与宿主合并快速选择相符，但未记录每次选择事件，不能测算具体合并策略或把按键次数算作预览次数。

全量三个对象的生命周期为：

| 实例 | prevhost PID / TID | 对象地址 / 构造时 QPC | DoPreview / 外部 Unload | 最后 Unload 返回到析构 | 析构线程 |
| --- | --- | --- | ---: | ---: | ---: |
| H1 | 9040 / 2564 | `0x19f86848af0` / `2701072351123` | 48 / 48 | 77.3717 ms | 2564 |
| H2 | 70792 / 70772 | `0x2085340e8c0` / `2997009766594` | 6 / 6 | 35.2986 ms | 70772 |
| H3 | 68124 / 62664 | `0x2b533d83410` / `2997214770862` | 1 / 1 | 23.3112 ms | 62664 |

H1 内有 47 次切换，H2 内有 5 次切换，52 次均在各自的原线程执行，旧 `Unload` 先返回，才进入新 `Initialize` 和 `DoPreview`。H3 只预览最后选中的 PNG。三个实例都记录到一次构造、一次 `Release=0` 和一次完整析构；全部接口都在各自表中的单个 TID 执行，没有观察到同一对象跨线程执行这些接口。

H1 的最终释放及后续重建可直接对照事件表：

| 日志行 | QPC | PID / TID | 事件 |
| ---: | ---: | --- | --- |
| 1820 | 2996641735003 | 9040 / 2564 | 外部 `Unload.end`，S_OK，HWND 已清空 |
| 1838 | 2996642504653 | 9040 / 2564 | `Release` 后引用计数为 0 |
| 1839 | 2996642508720 | 9040 / 2564 | `destruct.begin` |
| 1840–1841 | 2996642509006–2996642509188 | 9040 / 2564 | 析构内部调用空 `Unload`，直接成功 |
| 1842 | 2996642509364 | 9040 / 2564 | `destruct.end` |
| 1843 | 2997009770662 | 70792 / 70772 | 构造 H2，新的进程、线程和对象生命期 |

这说明本次连续选择文件时没有“上一 Handler 析构与下一 Handler 创建并行”的过程；关闭/重开阶段则确实出现了新对象和新线程。**不能把连续切换与关闭后重新打开混为一谈。** 上表的几十毫秒是三次观测值，不是 COM 保证的释放延迟。

补采阶段没有给每一个 UI 操作单独打时间标记，所以只将它作为关闭/重开阶段的证据，不声称某次析构由 Alt+P 单独触发。H3 的最终释放对应用户另外确认的测试窗口关闭。本轮没有通过强制终止进程制造析构记录。

## Unload、Release 与析构是不同的边界

`Unload` 的接口含义是停止当前项目的预览并释放与这次初始化关联的资源。微软文档明确描述：再次 `DoPreview` 前，Handler 会重新接受初始化和 `SetWindow`。因此一次 `Unload` 后复用同一 Handler 符合接口模型，并不要求每个文件都分配新对象。[IPreviewHandler::Unload](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ipreviewhandler-unload)

普通轮每次切换都出现 `SetSite(nullptr)` 和多次 `Release`，但引用计数未归零，也未进入析构；不能把其中任意一次 `Release` 当成对象销毁。COM 对象在引用计数归零时释放。[IUnknown::Release](https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release)

当前实现位于 `PreviewAllHandler/previewallhandler.cpp`：

```cpp
ULONG cRef = InterlockedDecrement(&m_cRef);
if (!cRef)
    delete this;
```

所以真正的析构入口紧随最后一次 `Release`，执行在线程调用栈内，三次实测均印证了这一点。析构函数又会调用 `Unload()` 做清理：如果宿主之前已成功卸载，`m_hwndPreview` 为空，这次内部调用直接返回，不发送额外 `CLOSE`。三次析构中的内部 `Unload` 都属于这种情况。

普通切换没有走到析构，关闭阶段则在外部 `Unload` 返回后继续释放接口引用，最后归零。引用计数日志只能确认增减和对象销毁，不能识别每个引用具体由 Explorer、COM 代理还是宿主缓存持有；本次未追踪这些内部持有者，也不能承诺关闭窗口到释放最后引用的固定时间。

## 线程结论的适用边界

本轮 TID 是 **prevhost 内执行 Handler 接口的线程**，没有对 Explorer 端的 COM 代理调用点插桩，不能据此断言 Explorer 发起调用也只有一个线程。

注册为 `Apartment` 的对象通过正常 COM 封送被调用时，接口在其所属 STA 线程执行；一个 STA 可以容纳多个对象，多个对象也可以分别属于不同 STA。STA 在分派消息或调用其他进程的 COM 对象时仍可能发生同线程重入。因此“本轮同线程且无交叠”不能扩大成“所有 Handler 全局串行”或“任何接口都绝不会重入”。[Single-Threaded Apartments](https://learn.microsoft.com/en-us/windows/win32/com/single-threaded-apartments)

三类格式目前共用 CLSID，为同实例复用提供了条件；真正决定是否保留实例的是宿主。本轮只覆盖本机版本、当前注册配置和这批扩展名，没有覆盖其他预览提供程序、不同 CLSID、多窗口并发或宿主崩溃恢复。

## 对 PreviewAll 实现的约束

1. 保持 `Initialize`、`SetWindow` 和 `DoPreview` 中的 Handler 工作轻量。一次切换中，旧预览的卸载会直接挡在新预览初始化之前；不能在此等待慢解析或解码结束。
2. 当前同步 `CLOSE` 只确认 Qt 页面已调用 `cancelPreview()` 和 `close()`，使旧任务及结果失效并隐藏页面。正在执行的第三方调用可以随后退出；确认不等待后台线程或原生子窗口销毁。
3. `PreviewAllApplication::handleCloseCmd` 保留原生子窗口及外部父窗口到 IPC 客户端断开后释放，避免销毁跨进程子窗口时反向等待正在等回复的宿主线程。`Unload` 成功后清空 Handler 保存的 HWND。
4. 不以 Handler 析构作为正常切换时取消任务的时机，因为 Handler 会复用；取消必须发生在关闭当前页面时。也不需要为了跟踪每次文件切换新增生产预览 ID。
5. PreviewAll 仍按各个 HWND 独立管理页面，并保持后台队列有界。单窗格的串行切换不排除其他窗格的并发请求；旧任务的后台清理也可以与下一预览的加载重叠。

第 2、3 点是当前代码与既有同步关闭验收确认的协议边界；本轮只插桩 Handler，不能用本轮 `Unload.end` 推断具体工作线程何时结束。

## 后续复测方法

先确认托盘程序路径、注册的 DLL 路径及托盘启用项。仅修改日志的构建也要确认已有 prevhost 是否仍加载旧 DLL，不能用磁盘文件更新时间替代运行版本核验。

取证时至少记录构造/析构、AddRef/Release、Initialize、SetSite、SetWindow、DoPreview、Unload 的入口/出口、PID/TID、对象生命期标记、HWND、路径和返回值。复用判断必须同时检查构造次数、析构事件和引用计数，不能仅比较对象地址。

在真实资源管理器中按以下边界分组，分别保存日志行范围：

1. `test/switching` 的 `01→15→01` 普通往返，以及三类内部切换。
2. 不等内容就绪的快速往返；只统计实际进入 Handler 的预览，宿主可能合并快速选择。
3. 关闭、重开预览窗格；关闭、重开资源管理器窗口，观察最后一次 Release、析构及下一次构造。
4. 两个窗口同时预览，判断不同实例的线程分配与交叠，避免将单窗格观察推广到全局。

结束后移除产品代码中的临时插桩，重新构建正式 Release DLL；证据只归档于文档目录，不把诊断开关或日志 ID 引入生产协议。

本次收尾已核对三次正常析构，移除 Handler 插桩，并重新编译 `PreviewAllHandler` 的 Release 目标。Handler 源码与取证前提交一致，托盘进程未更换；元数据同时保留插桩 DLL 和恢复后 DLL 的 SHA-256。源码恢复后显式触发了重新编译，避免复制备份保留旧时间戳导致增量构建误用插桩产物。

## 与 2026-09-27 调查的关系

上一轮使用 `out/com-trace-fixtures` 的六个样本。异步关闭基线有 63 次 `DoPreview`、62 次 `Unload`，两个 prevhost 各自复用一个实例/线程；其中 61 次 `Unload.end` 早于 PreviewAll 的 `CLOSE.end`。这证明当时的异步协议不能在 Unload 返回时保证旧页面已经处理关闭，随后实现了当前的同步确认。

同步版之后在真实预览窗格多轮验收，用户确认三类页面正常。中途反复重启进程时曾出现 CREATE 失败，重启并重测后未复现，既有证据不足以将其归因于同步关闭。旧轮的毫秒时间戳、样本和插桩范围与本轮不同，不能合并计数或把旧结果套给本轮未测的生命周期边界。
