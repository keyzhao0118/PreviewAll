#pragma once

#include <QThread>

class ArchiveExtractor : public QThread
{
	Q_OBJECT

public:
	explicit ArchiveExtractor(QObject* parent = nullptr);
	~ArchiveExtractor();

	void extractArchive(const QString& archivePath, const QString& destDirPath);

signals:
	void requirePassword(bool& bCancel, QString& password);
	void updateProgress(quint64 completed, quint64 total);

	void extractFailed();
	void extractSucceed();


protected:
	void run() override;

private:
	QString m_archivePath;
	QString m_destDirPath;
	QString m_password;
};