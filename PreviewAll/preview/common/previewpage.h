#pragma once

#include <QString>
#include <QWidget>

// Format-specific content. The native shell and state transitions belong to
// PreviewWidget; pages render their format and report loading outcomes.
class PreviewPage : public QWidget
{
	Q_OBJECT

public:
	~PreviewPage() override = default;
	virtual void startPreview() = 0;

signals:
	void loading();
	void ready();
	void empty(const QString& message);
	void failed(const QString& message);

protected:
	explicit PreviewPage(const QString& filePath, QWidget* parent = nullptr);
	const QString& filePath() const { return m_filePath; }

private:
	const QString m_filePath;
};
