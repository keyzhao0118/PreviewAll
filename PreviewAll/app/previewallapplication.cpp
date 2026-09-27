#include "previewallapplication.h"
#include "previewallregister.h"
#include "preview/previewwidget.h"
#include "preview/common/previewtaskqueue.h"
#include <QLocalSocket>
#include <QGuiApplication>
#include <QLocale>
#include <QScreen>
#include <QTranslator>
#include <Windows.h>
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

namespace
{

	const QString s_previewAllSocketName = "PreviewAllSocket_{0A869132-411F-41ED-9CD7-47659A55569F}";
	constexpr UINT_PTR s_previewSubclassId = 1;

	QScreen* screenForWindow(HWND hwnd)
	{
		const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info{sizeof(info)};
		if (!GetMonitorInfoW(monitor, &info))
			return nullptr;

		// Qt preserves native monitor origins even when it scales screen sizes.
		const QPoint origin(info.rcMonitor.left, info.rcMonitor.top);
		for (QScreen* screen : QGuiApplication::screens())
		{
			if (screen->geometry().topLeft() == origin)
				return screen;
		}
		return nullptr;
	}

	LRESULT CALLBACK previewWindowProc(HWND hwnd, UINT message, WPARAM wParam,
		LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData)
	{
		const LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
		if (message == WM_DPICHANGED_AFTERPARENT)
		{
			auto* hostWindow = reinterpret_cast<QWindow*>(refData);
			HWND hwndParent = reinterpret_cast<HWND>(hostWindow->winId());
			RECT rect{};
			if (GetParent(hwnd) == hwndParent)
			{
				// The native child DPI changed, but Qt children inherit the
				// foreign parent's QScreen, which otherwise stays at its old DPI.
				// During a drag the child can still mostly occupy the old monitor;
				// the top-level host is the window whose DPI Windows switched.
				HWND hwndRoot = GetAncestor(hwndParent, GA_ROOT);
				if (QScreen* screen = screenForWindow(hwndRoot); screen && hostWindow->screen() != screen)
					hostWindow->setScreen(screen);

				// The preview host does not call SetRect for a monitor change.
				if (GetClientRect(hwndParent, &rect))
					SetWindowPos(hwnd, nullptr, 0, 0, rect.right, rect.bottom,
						SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
		else if (message == WM_NCDESTROY)
		{
			RemoveWindowSubclass(hwnd, previewWindowProc, subclassId);
		}
		return result;
	}

}

PreviewAllApplication::PreviewAllApplication(int& argc, char** argv)
	: QApplication(argc, argv)
{
}

PreviewAllApplication::~PreviewAllApplication()
{
	m_previews.clear();
	PreviewTaskQueue::instance().waitForDone();
}

void PreviewAllApplication::initTranslations()
{
	const QLocale locale = QLocale::system();
	const QString translationsDir = QCoreApplication::applicationDirPath() + "/translations";

	// Qt Widgets creates its own context menus (for example, QTextBrowser's).
	auto* qtTranslator = new QTranslator(this);
	if (qtTranslator->load(locale, "qt", "_", translationsDir))
		installTranslator(qtTranslator);

	auto* appTranslator = new QTranslator(this);
	if (appTranslator->load(locale, "previewall", "_", translationsDir))
		installTranslator(appTranslator);
}

void PreviewAllApplication::startWindowManageService()
{
	QLocalServer::removeServer(s_previewAllSocketName);
	m_previewAllServer = new QLocalServer(this);
	m_previewAllServer->setSocketOptions(QLocalServer::WorldAccessOption);
	connect(m_previewAllServer, &QLocalServer::newConnection, this, &PreviewAllApplication::onNewConnection);
	if (!m_previewAllServer->listen(s_previewAllSocketName))
	{
		const QString err = m_previewAllServer->errorString();
		const QString msg = QString("PreviewAll: QLocalServer listen failed on '%1': %2")
			.arg(s_previewAllSocketName)
			.arg(err);
		qDebug() << msg;
	}
	
}

HWND PreviewAllApplication::handleCreateCmd(HWND hwndParent, const QString& filePath)
{
	QSharedPointer<QWindow> hostWindow;
	QSharedPointer<PreviewWidget> previewWidget = createPreviewWidget(filePath);
	if (!previewWidget)
		return nullptr;

	HWND hwndPreview = reinterpret_cast<HWND>(previewWidget->winId());
	hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(hwndParent)));
	if (!hostWindow)
		return nullptr;
	if (!previewWidget->windowHandle())
		return nullptr;
	previewWidget->windowHandle()->setParent(hostWindow.data());
	if (GetParent(hwndPreview) != hwndParent)
		return nullptr;
	// Initialize Qt's layout from the current host client size. The Handler
	// applies the authoritative SetWindow/SetRect position and size after CREATE.
	// Without this initial Qt resize, the first embedded page can retain a small
	// layout even though the Handler has resized its native HWND.
	RECT hostClient{};
	if (GetClientRect(hwndParent, &hostClient) &&
		hostClient.right > hostClient.left && hostClient.bottom > hostClient.top)
	{
		const qreal scale = previewWidget->devicePixelRatioF();
		previewWidget->resize(qRound((hostClient.right - hostClient.left) / scale),
			qRound((hostClient.bottom - hostClient.top) / scale));
	}
	previewWidget->show();
	if (!SetWindowSubclass(hwndPreview, previewWindowProc, s_previewSubclassId,
		reinterpret_cast<DWORD_PTR>(hostWindow.data())))
		return nullptr;
	m_previews.insert(hwndPreview, {hostWindow, previewWidget});
	return hwndPreview;
}

void PreviewAllApplication::handleCloseCmd(HWND hwndPreview, QLocalSocket* clientSocket)
{
	EmbeddedPreview preview = m_previews.take(hwndPreview);
	if (preview.widget)
	{
		// Invalidate queued work and results immediately; running decoders may
		// finish later and are never joined on the IPC/UI thread.
		preview.widget->cancelPreview();
		RemoveWindowSubclass(hwndPreview, previewWindowProc, s_previewSubclassId);
		preview.widget->close();

		// close() hides the page. Retain its native child until the waiting
		// preview host has read the acknowledgement and disconnected, so native
		// destruction cannot synchronously notify the blocked host thread.
		connect(clientSocket, &QLocalSocket::disconnected, this,
			[preview = std::move(preview)]() mutable {
				preview.widget.clear();
				preview.hostWindow.clear();
			});
	}
	clientSocket->write("CLOSED " + QByteArray::number(reinterpret_cast<qulonglong>(hwndPreview)) + '\n');
	clientSocket->flush();
}

QSharedPointer<PreviewWidget> PreviewAllApplication::createPreviewWidget(const QString& filePath)
{
	return QSharedPointer<PreviewWidget>::create(filePath);
}

void PreviewAllApplication::onNewConnection()
{
	QLocalSocket* clientSocket = m_previewAllServer->nextPendingConnection();
	if (!clientSocket)
		return;

	connect(clientSocket, &QLocalSocket::readyRead, this, &PreviewAllApplication::onReadyRead);
	connect(clientSocket, &QLocalSocket::disconnected, clientSocket, &QLocalSocket::deleteLater);
}

void PreviewAllApplication::onReadyRead()
{
	QLocalSocket *clientSocket = qobject_cast<QLocalSocket*>(sender());
	if (!clientSocket)
		return;

	while (clientSocket->canReadLine())
	{
		const QStringList parts = QString::fromUtf8(clientSocket->readLine().trimmed()).split(' ');
		if (parts.size() == 3 && parts[0] == "CREATE")
		{
			bool validParent = false;
			const quint64 parentId = parts[1].toULongLong(&validParent);
			const QString filePath = QByteArray::fromBase64(parts[2].toUtf8());
			const HWND created = validParent && parentId
				? handleCreateCmd(reinterpret_cast<HWND>(parentId), filePath)
				: nullptr;
			clientSocket->write(QByteArray::number(reinterpret_cast<qulonglong>(created)) + '\n');
			clientSocket->flush();
		}
		else if (parts.size() == 2 && parts[0] == "CLOSE")
		{
			bool validHwnd = false;
			const quint64 hwndValue = parts[1].toULongLong(&validHwnd);
			if (validHwnd && hwndValue)
				handleCloseCmd(reinterpret_cast<HWND>(hwndValue), clientSocket);
			else
				clientSocket->write("ERROR\n");
		}
		else
		{
			clientSocket->write("ERROR\n");
		}
	}

}
