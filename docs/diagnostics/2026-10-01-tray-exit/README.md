# 托盘退出修复的审核记录

本记录于 2026-10-01 整理，针对预览仍显示时点击托盘“退出”无效的问题。修复仅将托盘菜单的退出槽改为 UI 线程中的 `QCoreApplication::exit(0)`；应用仍由原有析构流程释放页面、取消任务并等待剩余工作。

## 原因与代码依据

`PreviewWidget` 没有 QWidget 父对象，因此仍属于 `QApplication::topLevelWidgets()`；嵌入时只为其 `QWindow` 设置了外部宿主父窗口。两层对顶层窗口的判断不同。

本机 Qt 6.11.1 源码显示：

- `qapplication.cpp:1594` 的 `tryCloseAllWidgetWindows` 对可见顶层 QWidget 调用 `windowHandle()->close()`。
- `qwindow.cpp:2440` 的 `QWindow::close()` 对非顶层 QWindow 返回 `false`。
- `qapplication.cpp:1682` 的 Quit 事件处理在发现仍可见的顶层 QWidget 后忽略退出请求。
- `qcoreapplication.cpp:1510` 的 `QCoreApplication::exit()` 发出 `aboutToQuit` 并结束当前线程的事件循环，随后正常返回 `main`，执行注销及析构。

上述源码属于本地 vcpkg 的 Qt 构建目录，行号仅对应该版本。没有给生产程序添加关闭计时器、新 IPC 或诊断开关。

## 已取得的运行证据

程序路径为 `out/build/x64-release/bin/PreviewAll.exe`，带临时日志的 Release 构建。归档的 [events.tsv](events.tsv) 是既有日志中 PID 8108 的完整段落，所有事件均来自 UI 线程 TID 17192。列依次为 `GetTickCount64` 毫秒值、PID、TID、事件、详情；未添加表头，详情为空时省略末列，其余字段原样保留。

该段记录了 15 次 `CREATE.ready`，此事件只在设置宿主父窗口、校验原生父窗口、显示页面并注册窗口子类均成功后写入。最后一次退出依次记录：

```text
28586703  aboutToQuit
28586703  destructor.begin  1
28586734  previews.cleared
28586734  tasks.finished
```

退出进入析构时仍管理一个嵌入预览页；页面释放与剩余任务等待均返回。本次审核读取进程列表时 PreviewAll 已不在运行。日志中两个收尾事件距析构入口约 31 ms，这只是带文件日志的单次观察，不作为性能保证。

## 证据边界与收尾

本段日志没有记录文件路径、页面类型或任务是否仍在加载，不能据此认定三类预览、加载中退出及多个窗格并存已逐项验收；也没有捕获有活动预览时旧 `quit()` 被忽略的运行日志，原因依据来自上面的 Qt 源码调用链。

临时插桩已从生产源码移除；最终源码仅保留托盘退出修复。后续回归直接在真实资源管理器中覆盖无预览、三类已就绪预览、慢样本加载中退出、多窗格退出及重新启动，不能用独立窗口替代宿主验收。
