# 2026-09-28 Handler 调用链证据

结论与适用边界见 [调用链实测报告](../../preview-handler-lifecycle.md)。这些文件记录真实资源管理器预览窗格的操作，不是模拟宿主程序输出。

- `events.tsv`：2382 条事件。第 1–1494 行为普通轮（39 次 `DoPreview`、38 次外部 `Unload`，用户确认页面均正常），第 1495–1496 行为空闲引用计数维护，第 1497–2346 行为快速切换与关闭重开补采，第 2347–2382 行为其余尺寸事件及最后一个测试窗口关闭。
- `metadata.json`：系统版本、代码版本、运行文件校验值、阶段范围和 QPC 频率。`handler_sha256` 对应临时插桩 DLL，`restored_handler_sha256` 对应收尾时重新编译的正式 DLL；PreviewAll.exe 未改动。
- `summary.json`：按对象生命周期汇总的调用数量、线程、引用归零和析构时刻。
- `analyze.py`：只解析上述事件并重建 `summary.json`；不启动进程、不操作 UI、不模拟预览宿主。在仓库根目录执行 `python docs/diagnostics/2026-09-28-handler-trace/analyze.py` 可复核。

TSV 无表头，每行九列：

| 列 | 含义 |
| --- | --- |
| `qpc` | 事件发生时的 `QueryPerformanceCounter` 值 |
| `pid` / `tid` | 执行该事件的进程、线程 ID，十进制 |
| `instance` | 构造时记录的 QPC 值，仅供诊断区分对象生命周期 |
| `object` | `this` 地址，十六进制，无 `0x` 前缀 |
| `event` | 构造、引用计数操作或接口的 `.begin` / `.end` |
| `preview_hwnd` | 该时刻 Handler 持有的预览 HWND，十进制 |
| `result` | 接口返回 HRESULT（十六进制）；构造时初始计数或 AddRef/Release 后引用计数（十进制）；无值时为 `-` |
| `detail` | 文件路径；SetWindow 为父 HWND；SetSite 为 `site` / `null`；QueryInterface 仅记录 IID 的 Data1（不是完整 IID） |

两条事件相差的毫秒数为 `(qpc2 - qpc1) * 1000 / 10000000`。对象由 `(pid, instance)` 区分，并结合构造/析构核对，不能只根据地址是否相同判断复用。这个诊断字段不进入正式 IPC。

归档只做路径规范化（移除本机仓库前缀、统一 `/`）和将空 detail 写为 `-`；其他字段和事件顺序保留。日志写入会影响耗时，不能把这里的时间当成无插桩性能基准。`QueryInterface` 返回 `80004002` 表示不支持该接口，不能统计为预览失败。

全量共 55 次成功 `DoPreview`、55 次成功外部 `Unload`，另有 3 次析构内部的空 `Unload`，因此事件表中的 `Unload.begin` / `.end` 各为 58 次。三个实例全部正常析构，52 次实例内切换全部同线程；旧 `Unload` 都先返回，随后才进入新 `Initialize` / `DoPreview`。

本轮只记录 Handler 内实际执行的调用，不记录 Explorer 发起 COM 调用的线程，也不记录 PreviewAll 的工作线程退出或原生窗口析构。析构函数内部还会调用一次 `Unload`；分析时将它与宿主调用的 `Unload` 分开。关闭/重开阶段没有逐个 UI 动作的时间标记，不能据此推断某次析构对应 Alt+P 切换；最后一次窗口关闭有用户单独确认。
