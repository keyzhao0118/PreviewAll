#include "pdfpreviewwidget.h"
#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QSvgRenderer>
#include <QThread>
#include <QPdfDocument>
#include <QPdfView>

PdfPreviewWidget::PdfPreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
{
	setWindowFlags(Qt::FramelessWindowHint);
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(0);

	initStatusBar();

	m_stackedLayout = new QStackedLayout();
	m_mainLayout->addLayout(m_stackedLayout);

	startLoadPdf();
}

PdfPreviewWidget::~PdfPreviewWidget()
{
}

void PdfPreviewWidget::initStatusBar()
{
	QWidget* statusBar = new QWidget(this);
	QFont statusBarFont("Microsoft YaHei");
	statusBarFont.setPointSize(9);
	statusBar->setFont(statusBarFont);
	statusBar->setFixedHeight(50);

	QHBoxLayout* statusBarLayout = new QHBoxLayout(statusBar);
	statusBarLayout->setContentsMargins(10, 0, 10, 0);
	statusBarLayout->setSpacing(10);

	QLabel* pdfIconLab = new QLabel(statusBar);
	QSvgRenderer svgRenderer(QString(":/svg/pdf.svg"));
	QPixmap pdfPix(30, 30);
	pdfPix.fill(Qt::transparent);
	QPainter painter(&pdfPix);
	svgRenderer.render(&painter);
	painter.end();
	pdfIconLab->setPixmap(pdfPix);

	m_statusLab = new QLabel(statusBar);

	m_zoomOutBtn = new QPushButton(statusBar);
	m_zoomOutBtn->setIcon(QIcon(":/svg/zoom-out.svg"));
	m_zoomOutBtn->setIconSize(QSize(30, 30));
	m_zoomOutBtn->setToolTip(tr("Zoom out"));
	m_zoomOutBtn->setStyleSheet(
		"QPushButton { border: none; border-radius: 4px; padding: 4px; background: transparent; }"
		"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
		"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }"
	);
	connect(m_zoomOutBtn, &QPushButton::clicked, this, &PdfPreviewWidget::onZoomOutClicked);

	m_zoomInBtn = new QPushButton(statusBar);
	m_zoomInBtn->setIcon(QIcon(":/svg/zoom-in.svg"));
	m_zoomInBtn->setIconSize(QSize(30, 30));
	m_zoomInBtn->setToolTip(tr("Zoom in"));
	m_zoomInBtn->setStyleSheet(
		"QPushButton { border: none; border-radius: 4px; padding: 4px; background: transparent; }"
		"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
		"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }"
	);
	connect(m_zoomInBtn, &QPushButton::clicked, this, &PdfPreviewWidget::onZoomInClicked);

	m_fitWidthBtn = new QPushButton(statusBar);
	m_fitWidthBtn->setIcon(QIcon(":/svg/expand.svg"));
	m_fitWidthBtn->setIconSize(QSize(30, 30));
	m_fitWidthBtn->setToolTip(tr("Fit width"));
	m_fitWidthBtn->setStyleSheet(
		"QPushButton { border: none; border-radius: 4px; padding: 4px; background: transparent; }"
		"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
		"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }"
	);
	connect(m_fitWidthBtn, &QPushButton::clicked, this, &PdfPreviewWidget::onFitWidthClicked);

	statusBarLayout->addWidget(pdfIconLab);
	statusBarLayout->addWidget(m_statusLab);
	statusBarLayout->addStretch();
	statusBarLayout->addWidget(m_zoomOutBtn);
	statusBarLayout->addWidget(m_zoomInBtn);
	statusBarLayout->addWidget(m_fitWidthBtn);

	m_mainLayout->addWidget(statusBar);
}

void PdfPreviewWidget::startLoadPdf()
{
	showLoadingPage();

	QPointer<PdfPreviewWidget> that(this);
	QThread* loadThread = QThread::create([that]() {
		if (!that)
			return;

		QPdfDocument* doc = new QPdfDocument(nullptr);
		doc->load(that->m_filePath);

		if (doc->status() == QPdfDocument::Status::Ready) {
			int pageCount = doc->pageCount();
			doc->moveToThread(qApp->thread());
			QMetaObject::invokeMethod(that, [that, doc, pageCount]() {
				if (!that) {
					delete doc;
					return;
				}
				that->m_pdfDocument = doc;
				doc->setParent(that);
				that->showPreviewPage(pageCount);
			}, Qt::QueuedConnection);
		} else {
			delete doc;
			QMetaObject::invokeMethod(that, [that]() {
				if (that)
					that->showErrorPage();
			}, Qt::QueuedConnection);
		}
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void PdfPreviewWidget::showLoadingPage()
{
	if (!m_loadingPage) {
		m_loadingPage = new QWidget(this);
		m_loadingPage->setStyleSheet("background-color: palette(base);");

		QLabel* loadingLab = new QLabel(m_loadingPage);
		loadingLab->setAlignment(Qt::AlignCenter);

		QMovie* movie = new QMovie(":/gif/loading.gif", QByteArray(), loadingLab);
		movie->setScaledSize(QSize(50, 50));
		loadingLab->setMovie(movie);
		movie->start();

		QVBoxLayout* layout = new QVBoxLayout(m_loadingPage);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addStretch();
		layout->addWidget(loadingLab, 0, Qt::AlignCenter);
		layout->addStretch();

		m_stackedLayout->addWidget(m_loadingPage);
	}

	m_stackedLayout->setCurrentWidget(m_loadingPage);
	m_statusLab->setText(tr("Loading"));
}

void PdfPreviewWidget::showErrorPage()
{
	if (!m_errorPage) {
		m_errorPage = new QWidget(this);
		m_errorPage->setStyleSheet("background-color: palette(base);");

		QLabel* errorLab = new QLabel(m_errorPage);
		errorLab->setText(tr("Failed to load PDF"));
		errorLab->setAlignment(Qt::AlignCenter);

		QFont errorFont("Microsoft YaHei");
		errorFont.setPointSize(9);
		errorLab->setFont(errorFont);

		QVBoxLayout* layout = new QVBoxLayout(m_errorPage);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addStretch();
		layout->addWidget(errorLab, 0, Qt::AlignCenter);
		layout->addStretch();

		m_stackedLayout->addWidget(m_errorPage);
	}

	m_stackedLayout->setCurrentWidget(m_errorPage);
	m_statusLab->setText(tr("Error"));
}

void PdfPreviewWidget::showPreviewPage(int pageCount)
{
	if (!m_previewPage) {
		m_previewPage = new QWidget(this);
		m_previewPage->setStyleSheet("background-color: palette(base);");

		m_pdfView = new QPdfView(m_previewPage);
		m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
		m_pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);
		m_pdfView->setDocument(m_pdfDocument);

		QVBoxLayout* previewLayout = new QVBoxLayout(m_previewPage);
		previewLayout->setContentsMargins(0, 0, 0, 0);
		previewLayout->setSpacing(0);
		previewLayout->addWidget(m_pdfView);

		m_stackedLayout->addWidget(m_previewPage);
	}

	m_stackedLayout->setCurrentWidget(m_previewPage);

	QFileInfo fi(m_filePath);
	qint64 fileSize = fi.size();
	QString sizeStr;
	if (fileSize >= 1024 * 1024)
		sizeStr = QString::number(fileSize / (1024.0 * 1024.0), 'f', 1) + " MB";
	else if (fileSize >= 1024)
		sizeStr = QString::number(fileSize / 1024.0, 'f', 1) + " KB";
	else
		sizeStr = QString::number(fileSize) + " B";

	m_statusLab->setText(tr("Pages: %1 | %2").arg(pageCount).arg(sizeStr));
}

void PdfPreviewWidget::onZoomInClicked()
{
	if (!m_pdfView)
		return;
	m_pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
	m_pdfView->setZoomFactor(m_pdfView->zoomFactor() * 1.25);
}

void PdfPreviewWidget::onZoomOutClicked()
{
	if (!m_pdfView)
		return;
	m_pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
	m_pdfView->setZoomFactor(m_pdfView->zoomFactor() / 1.25);
}

void PdfPreviewWidget::onFitWidthClicked()
{
	if (!m_pdfView)
		return;
	m_pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);
}
