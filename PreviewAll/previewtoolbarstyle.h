#pragma once

#include <QFont>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSize>
#include <QWidget>

namespace PreviewToolbarStyle
{
	inline constexpr int Height = 34;
	inline constexpr int ContentIconSize = 22;
	inline constexpr int ButtonSize = 28;
	inline constexpr int ButtonIconSize = 18;
	inline constexpr int FontPointSize = 10;

	inline void apply(QWidget* toolbar, QHBoxLayout* layout)
	{
		QFont font("Microsoft YaHei UI");
		font.setPointSize(FontPointSize);
		toolbar->setFont(font);
		toolbar->setFixedHeight(Height);
		layout->setContentsMargins(6, 2, 6, 2);
		layout->setSpacing(4);
	}

	inline void applyButton(QPushButton* button)
	{
		button->setFixedSize(ButtonSize, ButtonSize);
		button->setIconSize(QSize(ButtonIconSize, ButtonIconSize));
		button->setStyleSheet(
			"QPushButton { border: 1px solid transparent; border-radius: 4px; padding: 4px; background: transparent; }"
			"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
			"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }"
			"QPushButton:checked { border-color: rgba(128, 128, 128, 150); background-color: rgba(128, 128, 128, 40); }"
		);
	}
}
