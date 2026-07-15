#include "imageviewstatusbar.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

ImageViewStatusBar::ImageViewStatusBar(const QString& imagePath, QWidget* parent)
	: QWidget(parent)
{
	setFixedHeight(34);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(6, 2, 6, 2);
	layout->setSpacing(4);

	auto* typeIcon = new QSvgWidget(":/svg/image.svg", this);
	typeIcon->setFixedSize(22, 22);

	auto* fileLabel = new QLabel(QFileInfo(imagePath).fileName(), this);
	fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto* adaptiveButton = new QPushButton(this);
	adaptiveButton->setFixedSize(28, 28);
	adaptiveButton->setIcon(QIcon(":/svg/expand.svg"));
	adaptiveButton->setIconSize(QSize(18, 18));
	adaptiveButton->setToolTip(tr("Adaptive window"));
	adaptiveButton->setStyleSheet(
		"QPushButton { border: 1px solid transparent; border-radius: 4px; padding: 4px; background: transparent; }"
		"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
		"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }"
	);
	connect(adaptiveButton, &QPushButton::clicked, this, &ImageViewStatusBar::adaptiveScale);

	layout->addWidget(typeIcon);
	layout->addWidget(fileLabel);
	layout->addStretch();
	layout->addWidget(adaptiveButton);
}