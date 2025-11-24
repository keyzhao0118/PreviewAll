#pragma once

#include <QThread>

class ArchiveParser : public QThread
{
	Q_OBJECT

public:
	explicit ArchiveParser(QObject* parent = nullptr);
	~ArchiveParser();

	void parseArchive(const QString& archivePath);

signals:
	void parsingFailed(const QString& errorMsg);
	void entryFound(const QString& entryPath, bool bIsDir, quint64 entrySize);


protected:
	void run() override;

private:
	QString m_archivePath;

};