#include "archiveparser.h"
#include "commonhelper.h"
#include <archive.h>
#include <archive_entry.h>
#include <QSharedPointer>
#include <QMessageBox>

ArchiveParser::ArchiveParser(QObject* parent /*= nullptr*/)
	: QThread(parent)
{
}

ArchiveParser::~ArchiveParser()
{
	requestInterruption();
	if (!(this->wait(1000)))
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to stop thread within 1 second.");
}

void ArchiveParser::parseArchive(const QString& archivePath)
{
	m_archivePath = archivePath;
	this->start();
}

void ArchiveParser::run()
{
	QSharedPointer<struct archive> archivePtr(archive_read_new(), archive_read_free);
	archive_read_support_format_all(archivePtr.data());
	archive_read_support_filter_all(archivePtr.data());

	if (archive_read_open_filename(archivePtr.data(), m_archivePath.toUtf8().constData(), 10240) != ARCHIVE_OK)
	{
		CommonHelper::LogKeyZipDebugMsg(archive_error_string(archivePtr.data()));
		emit parsingFailed(tr("ArchiveParser: Could not open archive: %1"));
		return;
	}

	QSharedPointer<struct archive_entry> entryPtr(archive_entry_new(), archive_entry_free);
	do
	{
		if (isInterruptionRequested())
			break;

		int ret = archive_read_next_header2(archivePtr.data(), entryPtr.data());
		if (ret == ARCHIVE_OK)
		{
			QString entryPath = QString::fromStdWString(archive_entry_pathname_w(entryPtr.data()));
			bool bIsDir = archive_entry_filetype(entryPtr.data()) == AE_IFDIR;
			quint64 entrySize = archive_entry_size(entryPtr.data());
			emit entryFound(entryPath, bIsDir, entrySize);
		}
		else if (ret == ARCHIVE_EOF)
		{
			break;
		}
		else
		{
			CommonHelper::LogKeyZipDebugMsg(archive_error_string(archivePtr.data()));
			emit parsingFailed(tr("ArchiveParser: Error while reading archive: %1"));
			return;
		}
	} while (true);



}
