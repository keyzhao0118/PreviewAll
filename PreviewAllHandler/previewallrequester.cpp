#include "previewallrequester.h"
#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QDebug>
#include <optional>

namespace
{

	const QString SocketName = "PreviewAllSocket_{0A869132-411F-41ED-9CD7-47659A55569F}";
	constexpr int CreateTimeoutMs = 30000;
	constexpr int CloseTimeoutMs = 3000;

	std::optional<QByteArray> sendRequest(const QByteArray& command, int timeoutMs)
	{
		QDeadlineTimer deadline(timeoutMs);
		QLocalSocket socket;
		socket.connectToServer(SocketName);
		if (!socket.waitForConnected(deadline.remainingTime()))
		{
			qWarning() << "PreviewAll IPC connect failed:" << socket.errorString();
			return std::nullopt;
		}
		if (socket.write(command) != command.size())
			return std::nullopt;
		while (socket.bytesToWrite() > 0)
		{
			if (!socket.waitForBytesWritten(deadline.remainingTime()) && socket.bytesToWrite() > 0)
				return std::nullopt;
		}
		while (!socket.canReadLine())
		{
			if (!socket.waitForReadyRead(deadline.remainingTime()))
			{
				qWarning() << "PreviewAll IPC response failed:" << socket.errorString();
				return std::nullopt;
			}
		}
		const QByteArray response = socket.readLine().trimmed();
		socket.disconnectFromServer();
		return response;
	}
}

HWND PreviewAllRequester::sendCreateCmd(HWND hwndParent, const QString& filePath)
{
	const QByteArray command = "CREATE " + QByteArray::number(reinterpret_cast<qulonglong>(hwndParent))
		+ ' ' + filePath.toUtf8().toBase64() + '\n';
	const auto response = sendRequest(command, CreateTimeoutMs);
	if (!response)
		return nullptr;
	bool validHwnd = false;
	const quint64 hwndValue = response->toULongLong(&validHwnd);
	return validHwnd ? reinterpret_cast<HWND>(hwndValue) : nullptr;
}

bool PreviewAllRequester::sendCloseCmd(HWND hwndPreview)
{
	if (!hwndPreview)
		return true;
	const QByteArray hwndValue = QByteArray::number(reinterpret_cast<qulonglong>(hwndPreview));
	const auto response = sendRequest("CLOSE " + hwndValue + '\n', CloseTimeoutMs);
	return response && *response == "CLOSED " + hwndValue;
}
