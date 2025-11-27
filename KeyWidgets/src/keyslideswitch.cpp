#include "keyslideswitch.h"
#include <QPainter>
#include <QMouseEvent>

KeySlideSwitch::KeySlideSwitch(QWidget* parent /*= nullptr*/)
	: QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAutoFillBackground(false);

	m_anim = new QPropertyAnimation(this, "offset", this);
	m_anim->setDuration(200);
	m_anim->setEasingCurve(QEasingCurve::InOutQuad);

	// 初始化 offset 位置
	m_offset = 0;
}

void KeySlideSwitch::setChecked(bool checked)
{
	if (m_checked != checked)
	{
		m_checked = checked;
		m_anim->stop();
		m_anim->setStartValue(m_offset);
		m_anim->setEndValue(m_checked ? 1.0 : 0.0);
		m_anim->start();
	}
}

void KeySlideSwitch::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_checked = !m_checked;

		m_anim->stop();
		m_anim->setStartValue(m_offset);
		m_anim->setEndValue(m_checked ? 1.0 : 0.0);
		m_anim->start();

		emit toggled(m_checked);
	}
}

qreal KeySlideSwitch::offset() const
{
	return m_offset;
}

void KeySlideSwitch::setOffset(qreal value)
{
	m_offset = value;
	update();
}

void KeySlideSwitch::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const int w = width();
	const int h = height();
	const int margin = 3;

	// 背景圆角矩形
	QColor bg = m_checked ? QColor(0, 170, 255) : QColor(180, 180, 180);
	p.setBrush(bg);
	p.setPen(Qt::NoPen);
	p.drawRoundedRect(rect(), h / 2.0, h / 2.0);

	// 滑块
	int diameter = h - margin * 2;
	int x = margin + (w - h) * m_offset;

	p.setBrush(Qt::white);
	p.drawEllipse(QRectF(x, margin, diameter, diameter));
}
