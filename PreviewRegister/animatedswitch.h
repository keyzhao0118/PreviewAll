#pragma once
#include <QWidget>
#include <QPropertyAnimation>

class AnimatedSwitch : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(qreal offset READ offset WRITE setOffset)

public:
	explicit AnimatedSwitch(QWidget* parent = nullptr);

	QSize sizeHint() const override {
		return QSize(50, 25);
	}

	bool isChecked() const { return m_checked; }

signals:
	void toggled(bool checked);

protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;

private:
	// 用于动画的属性
	qreal offset() const { return m_offset; }
	void setOffset(qreal value);

private:
	bool m_checked = false;         // 是否开启
	qreal m_offset = 0.0;           // 滑块位置
	QPropertyAnimation* m_anim = nullptr;
};
