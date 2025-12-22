#pragma once

#include <QThread>
#include <QDateTime>
#include <QSharedPointer>

struct ArchiveEntry
{
	ArchiveEntry(const QString& path, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime)
		: m_path(path)
		, m_bIsDir(bIsDir)
		, m_compressedSize(compressedSize)
		, m_originalSize(originalSize)
		, m_mtime(mtime)
	{}

	QString m_path;
	bool m_bIsDir = false;
	quint64 m_compressedSize = 0;
	quint64 m_originalSize = 0;
	QDateTime m_mtime;
};

class ArchiveTree;
class ArchiveTreeNode;

class ArchiveParser : public QThread
{
	Q_OBJECT

public:
	explicit ArchiveParser(QObject* parent = nullptr);
	~ArchiveParser();

	void parseArchive(const QString& archivePath);

	const ArchiveTreeNode* getRootNode() const;
	quint64 getFileCount() const;
	quint64 getFolderCount() const;

signals:
	void requirePassword(bool& bCancel, QString& password);
	void updateProgress(quint64 completed, quint64 total);
	void parseFailed();
	void parseSucceed();

protected:
	void run() override;

private:
	QString m_archivePath;
	QString m_password;

	QSharedPointer<ArchiveTree> m_archiveTree;

};