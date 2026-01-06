#include "previewallapplication.h"
#include <QLocalSocket>
#include <QFileInfo>
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
{}


void PreviewAllApplication::startWindowManageService()
{
	QLocalServer::removeServer(s_previewAllSocketName);
	m_previewAllServer = new QLocalServer(this);
	m_previewAllServer->setSocketOptions(QLocalServer::WorldAccessOption);
	connect(m_previewAllServer, &QLocalServer::newConnection, this, &PreviewAllApplication::onNewConnection);
	if (!m_previewAllServer->listen(s_previewAllSocketName))
	{
		// Failed to start the server
	}
	
}

HWND PreviewAllApplication::handleCreateCmd(HWND hwndParent, const QString& filePath)
{
	QSharedPointer<QWidget> previewWidget = createPreviewWidget(filePath);
	HWND hwndPreview = reinterpret_cast<HWND>(previewWidget->winId());
	SetParent(hwndPreview, hwndParent);
	previewWidget->show();
	m_widgetHash[hwndPreview] = previewWidget;
	return hwndPreview;
}

void PreviewAllApplication::handleResizeCmd(HWND hwndPreview, int width, int height)
{
	if (m_widgetHash.contains(hwndPreview))
	{
		auto previewWidget = m_widgetHash.value(hwndPreview);
		qreal ratio = previewWidget->devicePixelRatioF();
		previewWidget->setGeometry(0, 0, width / ratio, height / ratio);
	}
}

void PreviewAllApplication::handleCloseCmd(HWND hwndPreview)
{
	if (m_widgetHash.contains(hwndPreview))
	{
		m_widgetHash[hwndPreview]->close();
		m_widgetHash.remove(hwndPreview);
	}
}

void PreviewAllApplication::handleQuitCmd()
{
	for (auto w : m_widgetHash)
		w->close();

	m_widgetHash.clear();
	quit();
}

QSharedPointer<QWidget> PreviewAllApplication::createPreviewWidget(const QString& filePath)
{
	QSharedPointer<QWidget> previewWidget;

	QFileInfo fileInfo(filePath);
	QString suffix = fileInfo.suffix().toLower();

	previewWidget.reset(new QWidget());
	previewWidget->setStyleSheet("background-color: red;");
	previewWidget->setWindowFlags(Qt::FramelessWindowHint);

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
	else if (command == "RESIZE" && parts.size() == 4)
	{
		HWND hwndPreview = reinterpret_cast<HWND>(parts[1].toULongLong());
		int width = parts[2].toInt();
		int height = parts[3].toInt();
		handleResizeCmd(hwndPreview, width, height);
	}
	else if (command == "CLOSE" && parts.size() == 2)
	{
		HWND hwndPreview = reinterpret_cast<HWND>(parts[1].toULongLong());
		handleCloseCmd(hwndPreview);
	}
	else if (command == "QUIT" && parts.size() == 1)
	{
		handleQuitCmd();
	}
		
}

