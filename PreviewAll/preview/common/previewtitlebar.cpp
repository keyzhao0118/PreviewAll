#include "previewtitlebar.h"
#include "previewtitlebarstyle.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QUrl>

PreviewTitleBar::PreviewTitleBar(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(QFileInfo(filePath).absoluteFilePath())
{
	auto* layout = new QHBoxLayout(this);
	PreviewTitleBarStyle::apply(this, layout);

	auto* appIcon = new QLabel(this);
	appIcon->setFixedSize(PreviewTitleBarStyle::IconSize, PreviewTitleBarStyle::IconSize);
	appIcon->setPixmap(QIcon(":/svg/previewall.svg").pixmap(
		PreviewTitleBarStyle::IconSize, PreviewTitleBarStyle::IconSize));

	const QString fileName = QFileInfo(m_filePath).fileName();
	auto* fileNameLabel = new QLabel(fileName, this);
	fileNameLabel->setMinimumWidth(0);
	fileNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	fileNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	fileNameLabel->setToolTip(fileName);

	m_openButton = new QPushButton(this);
	m_openButton->setIcon(QIcon(":/svg/open.svg"));
	m_openButton->setToolTip(tr("Open with default application"));
	m_openButton->setAccessibleName(tr("Open with default application"));
	PreviewTitleBarStyle::applyOpenButton(m_openButton);

	layout->addWidget(appIcon);
	layout->addWidget(fileNameLabel, 1);
	layout->addWidget(m_openButton);

	connect(m_openButton, &QPushButton::clicked, this, &PreviewTitleBar::openWithDefaultApplication);
}

void PreviewTitleBar::openWithDefaultApplication()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath));
}
