#include "commonhelper.h"
#include <QDebug>

void CommonHelper::LogKeyZipDebugMsg(const QString& msg)
{

	QString logMsg = "[KeyZip]: " + msg;
	qDebug() << logMsg;
}
