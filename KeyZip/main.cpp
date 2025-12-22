#include <QApplication>
#include "keyzipwindow.h"
#include "archivetree.h"

int main(int argc, char* argv[])
{
	qRegisterMetaType<const ArchiveTreeNode*>("const ArchiveTreeNode*");
	QApplication app(argc, argv);

	KeyZipWindow w;
	w.show();
	
	return app.exec();
}
