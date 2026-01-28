#include <QApplication>
#include <QMessageBox>

#define NOMINMAX
#include <Windows.h>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
	const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
	{
		QMessageBox::critical(nullptr, "PreviewDemo",
			QString("COM 初始化失败 (CoInitializeEx): 0x%1").arg(QString::number(static_cast<qulonglong>(hr), 16)));
		return -1;
	}

	QApplication app(argc, argv);

	MainWindow w;
	w.resize(1000, 700);
	w.show();

	const int rc = app.exec();
	CoUninitialize();
	return rc;
}


