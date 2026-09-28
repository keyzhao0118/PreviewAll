#include "previewallrequester.h"
#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QDebug>
#include <optional>

namespace
{

	const QString SocketName = "PreviewAllSocket_{0A869132-411F-41ED-9CD7-47659A55569F}";
	constexpr int CreateTimeoutMs = 30000;
	// The close budget only covers the tray finding the page, cancelling its
	// task and acknowledging. Destroying the native child is deliberately not
	// part of it, so waiting longer buys nothing while the preview host thread
	// stays blocked and the next selection waits behind it.
	constexpr int CloseTimeoutMs = 250;

	std::optional<QByteArray> sendRequest(const QByteArray& command, int timeoutMs)
	{
		const QDeadlineTimer startedAt = QDeadlineTimer::current();
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
				qWarning() << "PreviewAll IPC response failed after"
					<< (startedAt.remainingTime() - deadline.remainingTime()) << "ms:"
					<< socket.errorString();
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

// Sends CLOSE and waits at most CloseTimeoutMs for CLOSED. The page is invalid
// whether or not the acknowledgement arrives: the notification has been sent, so
// the tray cancels the page's task as soon as it reads the request and finishes
// releasing the native child once this connection goes away. Waiting past the
// budget cannot make the page more invalid, so the caller never branches on it.
void PreviewAllRequester::sendCloseCmd(HWND hwndPreview)
{
	if (!hwndPreview)
		return;
	const QByteArray hwndValue = QByteArray::number(reinterpret_cast<qulonglong>(hwndPreview));
	const auto response = sendRequest("CLOSE " + hwndValue + '\n', CloseTimeoutMs);
	if (!response)
	{
		// Connection or write failure: the tray may never have seen the request.
		qWarning() << "PreviewAll close notification failed for preview" << hwndPreview;
		return;
	}
	if (*response != "CLOSED " + hwndValue)
	{
		qWarning() << "PreviewAll did not acknowledge closing preview" << hwndPreview
			<< "within" << CloseTimeoutMs << "ms";
	}
}
