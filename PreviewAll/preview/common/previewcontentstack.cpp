#include "previewcontentstack.h"

#include <QLabel>
#include <QBasicTimer>
#include <QHideEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QShowEvent>
#include <QStackedLayout>
#include <QTimer>
#include <QTimerEvent>
#include <QVBoxLayout>

namespace {
class PreviewSpinner final : public QWidget
{
public:
	explicit PreviewSpinner(QWidget* parent) : QWidget(parent) { setFixedSize(44, 44); }

protected:
	void showEvent(QShowEvent* event) override
	{
		QWidget::showEvent(event);
		m_timer.start(40, this);
	}

	void hideEvent(QHideEvent* event) override
	{
		m_timer.stop();
		QWidget::hideEvent(event);
	}

	void timerEvent(QTimerEvent* event) override
	{
		if (event->timerId() != m_timer.timerId())
		{
			QWidget::timerEvent(event);
			return;
		}
		m_angle = (m_angle + 12) % 360;
		update();
	}

	void paintEvent(QPaintEvent*) override
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(QPen(palette().color(QPalette::Highlight), 3.5, Qt::SolidLine, Qt::RoundCap));
		painter.drawArc(QRectF(5, 5, 34, 34), (90 - m_angle) * 16, 270 * 16);
	}

private:
	QBasicTimer m_timer;
	int m_angle = 0;
};
}

PreviewContentStack::PreviewContentStack(QWidget* parent)
	: QWidget(parent)
{
	m_stack = new QStackedLayout(this);
	m_stack->setContentsMargins(0, 0, 0, 0);

	m_loadingPage = new QWidget(this);
	m_loadingPage->setAutoFillBackground(true);
	auto* loadingLayout = new QVBoxLayout(m_loadingPage);
	loadingLayout->setContentsMargins(0, 0, 0, 0);
	loadingLayout->addStretch();
	m_indicator = new QWidget(m_loadingPage);
	auto* indicatorLayout = new QVBoxLayout(m_indicator);
	indicatorLayout->setAlignment(Qt::AlignCenter);
	indicatorLayout->addWidget(new PreviewSpinner(m_indicator), 0, Qt::AlignCenter);
	auto* loadingText = new QLabel(tr("Loading..."), m_indicator);
	loadingText->setAlignment(Qt::AlignCenter);
	indicatorLayout->addWidget(loadingText);
	loadingLayout->addWidget(m_indicator, 0, Qt::AlignCenter);
	loadingLayout->addStretch();
	m_indicator->hide();
	m_stack->addWidget(m_loadingPage);

	m_messagePage = new QWidget(this);
	m_messagePage->setAutoFillBackground(true);
	auto* messageLayout = new QVBoxLayout(m_messagePage);
	m_message = new QLabel(m_messagePage);
	m_message->setAlignment(Qt::AlignCenter);
	m_message->setWordWrap(true);
	messageLayout->addWidget(m_message);
	m_stack->addWidget(m_messagePage);

	m_loadingDelay = new QTimer(this);
	m_loadingDelay->setSingleShot(true);
	m_loadingDelay->setInterval(150);
	connect(m_loadingDelay, &QTimer::timeout, this, [this] {
		if (m_stack->currentWidget() == m_loadingPage)
		{
			m_indicator->show();
		}
	});
	showLoading();
}

void PreviewContentStack::addPage(QWidget* page)
{
	if (page && m_stack->indexOf(page) < 0)
		m_stack->addWidget(page);
}

void PreviewContentStack::showLoading()
{
	m_state = State::Loading;
	m_indicator->hide();
	m_stack->setCurrentWidget(m_loadingPage);
	m_loadingDelay->start();
}

void PreviewContentStack::showPage(QWidget* page)
{
	if (!page)
		return;
	m_state = State::Ready;
	m_loadingDelay->stop();
	m_indicator->hide();
	addPage(page);
	m_stack->setCurrentWidget(page);
}

void PreviewContentStack::showEmpty(const QString& message)
{
	showMessage(message, State::Empty);
}

void PreviewContentStack::showError(const QString& message)
{
	showMessage(message, State::Failed);
}

void PreviewContentStack::showMessage(const QString& message, State state)
{
	m_message->setText(message);
	m_loadingDelay->stop();
	m_indicator->hide();
	m_state = state;
	m_stack->setCurrentWidget(m_messagePage);
}
