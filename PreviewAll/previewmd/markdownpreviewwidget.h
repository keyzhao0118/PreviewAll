#pragma once

#include <QByteArray>
#include <QString>
#include <QWidget>

#include <atomic>
#include <memory>

class QTextBrowser;

class MarkdownPreviewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MarkdownPreviewWidget(const QString& filePath, QWidget* parent = nullptr);
	~MarkdownPreviewWidget() override;

private:
	enum class LoadError
	{
		None,
		Unavailable,
		ReadFailed,
	};

	void loadFile();
	void showLoadError(LoadError error);

	QString m_filePath;
	std::shared_ptr<std::atomic_bool> m_loadCancelled;
	QTextBrowser* m_renderedView = nullptr;
};
