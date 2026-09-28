#pragma once

#include <Windows.h>
#include <QString>

class PreviewAllRequester
{
public:
	static HWND sendCreateCmd(HWND hwndParent, const QString& filePath);
	// Sends the close notification and waits at most CloseTimeoutMs for the
	// acknowledgement. It reports nothing because the page is invalid on both
	// paths; the tray finishes closing it when this connection goes away.
	static void sendCloseCmd(HWND hwndPreview);
};
