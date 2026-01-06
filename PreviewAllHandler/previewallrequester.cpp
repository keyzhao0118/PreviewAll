#include "previewallrequester.h"
#include <QLocalSocket>

namespace
{

	const QString s_previewAllSocketName = "PreviewAllSocket_{0A869132-411F-41ED-9CD7-47659A55569F}";

}

HWND PreviewAllRequester::sendCreateCmd(HWND hwndParent, const QString& filePath)
{
	HWND hwndPreview = nullptr;
	QLocalSocket socket;
	socket.connectToServer(s_previewAllSocketName);
	if (!socket.waitForConnected())
		return hwndPreview;

	QString encodedPath = filePath.toUtf8().toBase64();
	QString command = QString("CREATE %1 %2\n").arg((qulonglong)hwndParent).arg(encodedPath);
	socket.write(command.toUtf8());
	socket.flush();

	if (socket.waitForReadyRead())
	{
		QByteArray response = socket.readLine().trimmed();
		hwndPreview = HWND(response.toULongLong());
	}
	socket.disconnectFromServer();

	return hwndPreview;
}

void PreviewAllRequester::postResizeCmd(HWND hwndPreview, int width, int height)
{
	QLocalSocket socket;
	socket.connectToServer(s_previewAllSocketName);
	if (socket.waitForConnected())
	{
		socket.write(QString("RESIZE %1 %2 %3\n").arg((qulonglong)hwndPreview).arg(width).arg(height).toUtf8());
		socket.flush();
		socket.waitForBytesWritten();
		socket.disconnectFromServer();
	}
}

void PreviewAllRequester::postCloseCmd(HWND hwnd)
{
	QLocalSocket socket;
	socket.connectToServer(s_previewAllSocketName);
	if (socket.waitForConnected())
	{
		socket.write(QString("CLOSE %1\n").arg((qulonglong)hwnd).toUtf8());
		socket.flush();
		socket.waitForBytesWritten();
		socket.disconnectFromServer();
	}
}
