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
	void requirePassword(bool& bCancel, QString& password);
	void updateProgress(qint64 completed, qint64 total);
	void entryFound(const QString& entryPath, bool bIsDir, qint64 entrySize);
	void parsingFailed();


protected:
	void run() override;

private:
	QString m_archivePath;
	QString m_password;

};