#pragma once

#include <QSharedPointer>
#include <QMutex>
#include <QWaitCondition>
#include <Windows.h>
#include <7zip/Archive/IArchive.h>
#include <Common/MyCom.h>

class ArchiveTree;
class ArchiveTreeNode;

class ArchiveParser : public QObject
{
	Q_OBJECT

public:
	explicit ArchiveParser(const QString& archivePath, QObject* parent = nullptr);
	~ArchiveParser();

	void stopParse();

	const ArchiveTreeNode* getRootNode() const;
	quint64 getFileCount() const;
	quint64 getFolderCount() const;

public slots:
	void parseArchive();

signals:
	void encryptArchive();
	void updateProgress(quint64 completed, quint64 total);
	void parseFailed();
	void parseSucceed();

private:
	HRESULT tryOpenArchive(
		const QString& archivePath,
		IArchiveOpenCallback* openCallback,
		CMyComPtr<IInArchive>& outInArchive);

	bool processArchive(CMyComPtr<IInArchive> archive);
	bool checkStopParse();

private:
	QString m_archivePath;
	QSharedPointer<ArchiveTree> m_archiveTree;

	QMutex m_mutex;
	QWaitCondition m_waitCondition;
	bool m_bStopParse = false;

};
