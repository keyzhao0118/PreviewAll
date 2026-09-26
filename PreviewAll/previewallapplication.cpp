#include "previewallapplication.h"
#include "previewallregister.h"
#include "previewarchive/archivepreviewwidget.h"
#include "previewimage/imageviewerwidget.h"
#include "previewmd/markdownpreviewwidget.h"
#include "previewloadpool.h"
#include <QLocalSocket>
#include <QFileInfo>
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
	previewLoadPool().waitForDone();
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
	QSharedPointer<QWidget> previewWidget = createPreviewWidget(filePath);
	if (!previewWidget)
		return nullptr;

	HWND hwndPreview = reinterpret_cast<HWND>(previewWidget->winId());
	hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(hwndParent)));
	if (!hostWindow || !previewWidget->windowHandle())
		return nullptr;
	previewWidget->windowHandle()->setParent(hostWindow.data());
	if (GetParent(hwndPreview) != hwndParent)
		return nullptr;
	previewWidget->show();
	if (!SetWindowSubclass(hwndPreview, previewWindowProc, s_previewSubclassId,
		reinterpret_cast<DWORD_PTR>(hostWindow.data())))
		return nullptr;
	m_previews.insert(hwndPreview, {hostWindow, previewWidget});
	return hwndPreview;
}

void PreviewAllApplication::handleCloseCmd(HWND hwndPreview)
{
	EmbeddedPreview preview = m_previews.take(hwndPreview);
	if (!preview.widget)
		return;
	RemoveWindowSubclass(hwndPreview, previewWindowProc, s_previewSubclassId);
	preview.widget->close();
}

QSharedPointer<QWidget> PreviewAllApplication::createPreviewWidget(const QString& filePath)
{
	QSharedPointer<QWidget> previewWidget;

	QFileInfo fileInfo(filePath);
	QString suffix = "." + fileInfo.suffix();

	if (PreviewAllRegister::archiveExtList.contains(suffix, Qt::CaseInsensitive))
	{
		previewWidget.reset(new ArchivePreviewWidget(filePath));
	}
	else if (PreviewAllRegister::imageExtList.contains(suffix, Qt::CaseInsensitive))
	{
		previewWidget.reset(new ImageViewerWidget(filePath));
	}
	else if (PreviewAllRegister::markdownExtList.contains(suffix, Qt::CaseInsensitive))
	{
		previewWidget.reset(new MarkdownPreviewWidget(filePath));
	}
	else
	{
		return previewWidget;
	}

	return previewWidget;
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

	QByteArray line = clientSocket->readLine().trimmed();
	QStringList parts = QString::fromUtf8(line).split(' ');
	if (parts.isEmpty())
		return;

	QString command = parts[0];
	if (command == "CREATE" && parts.size() == 3)
	{
		HWND hwndParent = reinterpret_cast<HWND>(parts[1].toULongLong());
		QString filePath = QByteArray::fromBase64(parts[2].toUtf8());

		HWND hwndPreview = handleCreateCmd(hwndParent, filePath);

		QByteArray response = QByteArray::number((qulonglong)hwndPreview) + "\n";
		clientSocket->write(response);
		clientSocket->flush();
	}
	else if (command == "CLOSE" && parts.size() == 2)
	{
		HWND hwndPreview = reinterpret_cast<HWND>(parts[1].toULongLong());
		handleCloseCmd(hwndPreview);
	}

}
