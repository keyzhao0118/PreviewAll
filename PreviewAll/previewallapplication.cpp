#include "previewallapplication.h"
#include "previewallregister.h"
#include "previewarchive/archivepreviewwidget.h"
#include "previewimage/imageviewerwidget.h"
#include "previewmd/markdownpreviewwidget.h"
#include "previewloadpool.h"
#include <QLocalSocket>
#include <QFileInfo>
#include <QLocale>
#include <QTranslator>
#include <Windows.h>

namespace
{

	const QString s_previewAllSocketName = "PreviewAllSocket_{0A869132-411F-41ED-9CD7-47659A55569F}";

}

PreviewAllApplication::PreviewAllApplication(int& argc, char** argv)
	: QApplication(argc, argv)
{
}

PreviewAllApplication::~PreviewAllApplication()
{
	m_widgetHash.clear();
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
	QSharedPointer<QWidget> previewWidget = createPreviewWidget(filePath);
	if (!previewWidget)
		return nullptr;

	HWND hwndPreview = reinterpret_cast<HWND>(previewWidget->winId());
	const LONG_PTR style = GetWindowLongPtrW(hwndPreview, GWL_STYLE);
	SetWindowLongPtrW(hwndPreview, GWL_STYLE, (style & ~WS_POPUP) | WS_CHILD);
	SetLastError(ERROR_SUCCESS);
	if (!SetParent(hwndPreview, hwndParent) && GetLastError() != ERROR_SUCCESS)
		return nullptr;
	SetWindowPos(hwndPreview, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	previewWidget->show();
	m_widgetHash[hwndPreview] = previewWidget;
	return hwndPreview;
}

void PreviewAllApplication::handleCloseCmd(HWND hwndPreview)
{
	if (m_widgetHash.contains(hwndPreview))
	{
		m_widgetHash[hwndPreview]->close();
		m_widgetHash.remove(hwndPreview);
	}
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
