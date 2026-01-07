#include "imageviewstatusbar.h"
#include <QApplication>
#include <QStyleHints>
#include <QLineEdit>
#include <QLabel>
#include <QFileInfo>
#include <QImageReader>


ImageViewStatusBar::ImageViewStatusBar(const QString& imagePath, QWidget* parent)
	: QWidget(parent)
	, m_imagePath(imagePath)
{
	setFixedHeight(35);
	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->setContentsMargins(5, 0, 5, 0);
	m_mainLayout->setSpacing(5);
	addResolutionLab();
	m_mainLayout->addStretch();
	addAdaptiveBtn();
	addScaleComboBox();
	addZoomOutBtn();
	addScaleSlider();
	addZoomInBtn();
}

ImageViewStatusBar::~ImageViewStatusBar()
{}

void ImageViewStatusBar::onScaleFactorChanged(qreal scaleFactor)
{
	int percent = qRound(scaleFactor * devicePixelRatioF() * 100);
	m_bIsInternalChange = true;
	m_scaleComboBox->setCurrentText(QString("%1%").arg(percent));
	m_scaleSlider->setValue(percent);
	m_bIsInternalChange = false;
}

void ImageViewStatusBar::resizeEvent(QResizeEvent* resizeEvent)
{
	QWidget::resizeEvent(resizeEvent);

	QList<QWidget*> controls
	{
		m_adaptiveBtn,
		m_scaleComboBox,
		m_zoomOutBtn,
		m_scaleSlider,
		m_zoomInBtn
	};

	int usedWidth = 10;
	int spacing = m_mainLayout->spacing();

	for (int i = 0; i < m_mainLayout->count(); ++i)
	{
		QWidget* w = m_mainLayout->itemAt(i)->widget();
		if (!w) 
			continue;
		if (controls.contains(w))
		{
			w->setVisible(false);
		}
		else
		{
			w->setVisible(true);
			usedWidth += w->width() + spacing;
		}
	}

	int stretchWidth = 20;
	if (m_adaptiveBtn && usedWidth + m_adaptiveBtn->width() <= width() - stretchWidth)
	{
		m_adaptiveBtn->setVisible(true);
		usedWidth += m_adaptiveBtn->width() + spacing;
	}

	if (m_scaleComboBox && usedWidth + m_scaleComboBox->width() <= width() - stretchWidth)
	{
		m_scaleComboBox->setVisible(true);
		usedWidth += m_scaleComboBox->width() + spacing;
	}

	if (m_zoomOutBtn && m_scaleSlider && m_zoomInBtn &&
		usedWidth + m_zoomOutBtn->width() + spacing + m_scaleSlider->width() + spacing + m_zoomInBtn->width() <= width() - stretchWidth)
	{
		m_zoomOutBtn->setVisible(true);
		m_scaleSlider->setVisible(true);
		m_zoomInBtn->setVisible(true);
	}
}

void ImageViewStatusBar::addResolutionLab()
{
	QLabel* resolutionIconLab = new QLabel(this);
	resolutionIconLab->setPixmap(QIcon(":/icons/image.svg").pixmap(24, 24));
	QImageReader reader(m_imagePath);
	QSize imageSize = reader.size();
	QLabel* resolutionTextLab = new QLabel(QString("%1 x %2").arg(imageSize.width()).arg(imageSize.height()), this);

	m_mainLayout->addWidget(resolutionIconLab);
	m_mainLayout->addWidget(resolutionTextLab);
}

void ImageViewStatusBar::addAdaptiveBtn()
{
	m_adaptiveBtn = new QPushButton(this);
	m_adaptiveBtn->setIcon(QIcon(":/icons/maximize.svg"));
	connect(m_adaptiveBtn, &QPushButton::clicked, this, &ImageViewStatusBar::adaptiveScale);
	m_mainLayout->addWidget(m_adaptiveBtn);
}

void ImageViewStatusBar::addScaleComboBox()
{
	m_scaleComboBox = new QComboBox(this);
	m_scaleComboBox->setFixedHeight(30);
	m_scaleComboBox->setEditable(true);
	m_scaleComboBox->setFocusPolicy(Qt::NoFocus);
	m_scaleComboBox->addItems({ "10%", "25%", "50%", "75%", "100%", "200%", "400%", "800%" });
	connect(m_scaleComboBox, &QComboBox::currentTextChanged, this, &ImageViewStatusBar::handleComboBoxChanged);
	m_mainLayout->addWidget(m_scaleComboBox);
}

void ImageViewStatusBar::addZoomOutBtn()
{
	m_zoomOutBtn = new QPushButton(this);
	m_zoomOutBtn->setIcon(QIcon(":/icons/zoom-out.svg"));
	connect(m_zoomOutBtn, &QPushButton::clicked, this, &ImageViewStatusBar::handleZoomOut);
	m_mainLayout->addWidget(m_zoomOutBtn);
}

void ImageViewStatusBar::addScaleSlider()
{
	m_scaleSlider = new QSlider(Qt::Horizontal, this);
	m_scaleSlider->setFixedSize(80, 15);
	m_scaleSlider->setMinimum(1);
	m_scaleSlider->setMaximum(800);
	connect(m_scaleSlider, &QSlider::valueChanged, this, &ImageViewStatusBar::handleSliderChanged);
	m_mainLayout->addWidget(m_scaleSlider);
}

void ImageViewStatusBar::addZoomInBtn()
{
	m_zoomInBtn = new QPushButton(this);
	m_zoomInBtn->setIcon(QIcon(":/icons/zoom-in.svg"));
	connect(m_zoomInBtn, &QPushButton::clicked, this, &ImageViewStatusBar::handleZoomIn);
	m_mainLayout->addWidget(m_zoomInBtn);
}

void ImageViewStatusBar::handleComboBoxChanged(const QString& text)
{
	if (m_bIsInternalChange)
		return;

	bool ok = false;
	int percent = QString(text).replace("%", "").toInt(&ok);

	m_bIsInternalChange = true;
	m_scaleSlider->setValue(percent);
	m_bIsInternalChange = false;
	emit scaleFactorChanged(percent / 100.0 / devicePixelRatioF());
}

void ImageViewStatusBar::handleSliderChanged(int value)
{
	if (m_bIsInternalChange)
		return;

	m_bIsInternalChange = true;
	m_scaleComboBox->setCurrentText(QString("%1%").arg(value));
	m_bIsInternalChange = false;
	emit scaleFactorChanged(value / 100.0 / devicePixelRatioF());
}

void ImageViewStatusBar::handleZoomOut()
{
	int curPercent = m_scaleSlider->value();
	int newPercent = curPercent % 10 == 0 ? curPercent - 10 : curPercent - (curPercent % 10);
	if (newPercent < m_scaleSlider->minimum())
		newPercent = m_scaleSlider->minimum();

	m_scaleSlider->setValue(newPercent);
}

void ImageViewStatusBar::handleZoomIn()
{
	int curPercent = m_scaleSlider->value();
	int newPercent = curPercent % 10 == 0 ? curPercent + 10 : curPercent - (curPercent % 10) + 10;
	if (newPercent > m_scaleSlider->maximum())
		newPercent = m_scaleSlider->maximum();

	m_scaleSlider->setValue(newPercent);
}
