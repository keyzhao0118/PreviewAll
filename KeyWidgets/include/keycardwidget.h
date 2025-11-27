#pragma once

#include <QWidget>
#include <QColor>

class KeyCardWidget : public QWidget
{
	Q_OBJECT

public:
	explicit KeyCardWidget(QWidget* parent = nullptr);
	~KeyCardWidget() = default;

	void setBackgroundColor(const QColor& backgroundColor);
	void setShadowColor(const QColor& shadowColor);
	void setRadius(int radius);
	void setShadowSize(int shadowSize);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QColor m_backgroundColor = Qt::white;
	QColor m_shadowColor = QColor(0, 0, 0, 3);
	int m_radius = 12;
	int m_shadowSize = 0;

};
