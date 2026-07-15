#include "imageviewstatusbar.h"
#include "previewtoolbarstyle.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

ImageViewStatusBar::ImageViewStatusBar(const QString& imagePath, QWidget* parent)
	: QWidget(parent)
{
	auto* layout = new QHBoxLayout(this);
	PreviewToolbarStyle::apply(this, layout);

	auto* typeIcon = new QSvgWidget(":/svg/image.svg", this);
	typeIcon->setFixedSize(PreviewToolbarStyle::ContentIconSize, PreviewToolbarStyle::ContentIconSize);

	auto* fileLabel = new QLabel(QFileInfo(imagePath).fileName(), this);
	fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto* adaptiveButton = new QPushButton(this);
	adaptiveButton->setIcon(QIcon(":/svg/expand.svg"));
	adaptiveButton->setToolTip(tr("Adaptive window"));
	PreviewToolbarStyle::applyButton(adaptiveButton);
	connect(adaptiveButton, &QPushButton::clicked, this, &ImageViewStatusBar::adaptiveScale);

	layout->addWidget(typeIcon);
	layout->addWidget(fileLabel);
	layout->addStretch();
	layout->addWidget(adaptiveButton);
}