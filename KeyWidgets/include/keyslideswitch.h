#pragma once
#include <QWidget>
#include <QPropertyAnimation>

class KeySlideSwitch : public QWidget
{
	Q_OBJECT
		Q_PROPERTY(qreal offset READ offset WRITE setOffset)

public:
	explicit KeySlideSwitch(QWidget* parent = nullptr);
	~KeySlideSwitch() = default;
	void setChecked(bool checked);

signals:
	void toggled(bool checked);

protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;

private:
	// 用于动画的属性
	qreal offset() const;
	void setOffset(qreal value);

private:
	bool m_checked = false;         // 是否开启
	qreal m_offset = 0.0;           // 滑块位置
	QPropertyAnimation* m_anim = nullptr;
};


