#include "keycardwidget.h"
#include <QPainter>
#include <QPainterPath>

KeyCardWidget::KeyCardWidget(QWidget* parent /*= nullptr*/)
	: QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAutoFillBackground(false);
}

void KeyCardWidget::setBackgroundColor(const QColor& backgroundColor)
{
	if (backgroundColor != m_backgroundColor)
	{
		m_backgroundColor = backgroundColor;
		update();
	}
}

void KeyCardWidget::setShadowColor(const QColor& shadowColor)
{
	if (shadowColor != m_shadowColor)
	{
		m_shadowColor = shadowColor;
		update();
	}
}

void KeyCardWidget::setRadius(int radius)
{
	if (radius != m_radius)
	{
		m_radius = radius;
		update();
	}
}

void KeyCardWidget::setShadowSize(int shadowSize)
{
	if (shadowSize != m_shadowSize)
	{
		m_shadowSize = shadowSize;
		update();
	}
}

void KeyCardWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	//绘制阴影，从最内层绘制到最外层
	for (int lay = 1; lay <= m_shadowSize; ++lay)
	{
		int alpha = m_shadowColor.alpha() * (m_shadowSize - lay) / m_shadowSize;
		QColor curLayShadowColor = m_shadowColor;
		curLayShadowColor.setAlpha(alpha);

		QPainterPath curLayShadowPath;
		curLayShadowPath.addRoundedRect(
			lay,
			lay,
			width() - 2 * lay,
			height() - 2 * lay,
			m_radius + lay,
			m_radius + lay);
		painter.fillPath(curLayShadowPath, curLayShadowColor);
	}

	QPainterPath cardPath;
	cardPath.addRoundedRect(rect().adjusted(m_shadowSize, m_shadowSize, -m_shadowSize, -m_shadowSize), m_radius, m_radius);
	painter.fillPath(cardPath, m_backgroundColor);
}
