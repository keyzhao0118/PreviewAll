#pragma once

#include <Windows.h>
#include <QString>

class PreviewAllRequester
{
public:
	static HWND sendCreateCmd(HWND hwndParent, const QString& filePath);
	static void postResizeCmd(HWND hwndPreview, int width, int height);
	static void postCloseCmd(HWND hwnd);
};

