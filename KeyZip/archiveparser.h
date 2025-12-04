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
	void updateProgress(quint64 completed, quint64 total);
	void entryFound(const QString& path, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime);
	void parsingFailed();
	void parsingSucceed();


protected:
	void run() override;

private:
	QString m_archivePath;
	QString m_password;

};