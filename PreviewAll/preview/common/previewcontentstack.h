#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class QStackedLayout;
class QTimer;

// Shared content state below PreviewTitleBar. Loading starts immediately;
// the indicator appears only if the load outlasts the short anti-flicker delay.
class PreviewContentStack : public QWidget
{
	Q_OBJECT
public:
	enum class State { Loading, Ready, Empty, Failed };
	explicit PreviewContentStack(QWidget* parent = nullptr);
	void addPage(QWidget* page);
	void showLoading();
	void showPage(QWidget* page);
	void showEmpty(const QString& message);
	void showError(const QString& message);
	State state() const { return m_state; }

private:
	void showMessage(const QString& message, State state);
	State m_state = State::Loading;
	QStackedLayout* m_stack = nullptr;
	QWidget* m_loadingPage = nullptr;
	QWidget* m_messagePage = nullptr;
	QWidget* m_indicator = nullptr;
	QLabel* m_message = nullptr;
	QTimer* m_loadingDelay = nullptr;
};
