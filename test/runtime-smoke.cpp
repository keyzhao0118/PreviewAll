#include "previewarchive/archiveparser.h"
#include "previewarchive/archivepreviewwidget.h"
#include "previewarchive/archivetreewidget.h"
#include "previewmd/markdownpreviewwidget.h"
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QImageReader>
#include <QFont>
#include <QLibrary>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTreeWidget>
#include <QTranslator>
#include <QDebug>
#include <QTimer>
#include <cstdio>

// Exercise deployed plugins and the actual archive parser without registry writes.
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    if (argc != 2)
        return 2;
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
    const QDir fixtures(QString::fromLocal8Bit(argv[1]));
    int failures = 0;
    auto check = [&failures](bool ok, const QString& name) {
        const QByteArray message = QString("%1 %2\n").arg(ok ? "PASS" : "FAIL", name).toLocal8Bit();
        std::fwrite(message.constData(), 1, static_cast<size_t>(message.size()), ok ? stdout : stderr);
        std::fflush(ok ? stdout : stderr);
        if (!ok) ++failures;
    };

    QLibrary handler(QCoreApplication::applicationDirPath() + "/PreviewAllHandler.dll");
    const bool handlerReady = handler.load() && handler.resolve("DllGetClassObject") && handler.resolve("DllCanUnloadNow");
    check(handlerReady, "COM DLL loads and exports entry points" +
          (handlerReady ? QString() : ": " + handler.errorString()));
    const auto formats = QImageReader::supportedImageFormats();
    for (const auto& format : {"png", "jpeg", "bmp", "gif", "ico", "svg", "tiff", "webp"})
        check(formats.contains(format), QString("Image plugin: ") + format);
    for (const auto& file : {"png-alpha.png", "jpeg-baseline.jpg", "bitmap.bmp",
                             "animated.gif", "icon-multisize.ico", "tiff-deflate.tif", "webp-lossless.webp"}) {
        QImageReader reader(fixtures.filePath(QString("images/") + file));
        const auto image = reader.read();
        check(!image.isNull(), QString("Decode: ") + file +
              (image.isNull() ? ": " + reader.errorString() : QString()));
    }

    QTextDocument markdownDocument;
    markdownDocument.setMarkdown(
        "# Heading\n\n**bold text** and `inline code`\n\n- first item\n- second item",
        QTextDocument::MarkdownDialectGitHub);
    const QTextBlock headingBlock = markdownDocument.begin();
    QTextCursor boldCursor = markdownDocument.find("bold text");
    QTextBlock listBlock = markdownDocument.begin();
    while (listBlock.isValid() && listBlock.text() != "first item")
        listBlock = listBlock.next();
    const bool markdownBasicsRender = markdownDocument.toPlainText().contains("Heading")
        && headingBlock.blockFormat().headingLevel() == 1
        && !boldCursor.isNull() && boldCursor.charFormat().fontWeight() >= QFont::Bold
        && listBlock.isValid() && listBlock.textList();
    check(markdownBasicsRender, "Markdown basic headings, emphasis, and lists");

    MarkdownPreviewWidget markdownPreview(fixtures.filePath("markdown/rich-syntax.md"));
    markdownPreview.setAttribute(Qt::WA_DontShowOnScreen);
    markdownPreview.resize(394, 515);
    markdownPreview.show();
    QTextBrowser* markdownBrowser = markdownPreview.findChild<QTextBrowser*>();
    QEventLoop markdownLoadLoop;
    QTimer markdownPoll;
    markdownPoll.setInterval(10);
    QObject::connect(&markdownPoll, &QTimer::timeout, &markdownLoadLoop, [&] {
        if (markdownBrowser && markdownBrowser->toPlainText().contains("PreviewAll Markdown fixture"))
            markdownLoadLoop.quit();
    });
    QTimer::singleShot(5000, &markdownLoadLoop, &QEventLoop::quit);
    markdownPoll.start();
    markdownLoadLoop.exec();
    markdownPoll.stop();
    check(markdownBrowser
              && markdownBrowser->toPlainText().contains("PreviewAll Markdown fixture")
              && markdownBrowser->document()->thread() == app.thread(),
          "Markdown file loads and its document returns to the UI thread");
    app.processEvents();
    if (markdownBrowser && markdownBrowser->horizontalScrollBar()->maximum() > 0) {
        std::fprintf(stderr, "Markdown viewport=%d idealWidth=%.1f scrollMax=%d\n",
                     markdownBrowser->viewport()->width(), markdownBrowser->document()->idealWidth(),
                     markdownBrowser->horizontalScrollBar()->maximum());
        for (QTextBlock block = markdownBrowser->document()->begin(); block.isValid(); block = block.next()) {
            if (block.blockFormat().nonBreakableLines())
                std::fprintf(stderr, "Nonbreakable: %s\n", block.text().left(100).toUtf8().constData());
        }
    }
    check(markdownBrowser && markdownBrowser->horizontalScrollBar()->maximum() == 0,
          "Markdown content fits a narrow Explorer preview pane");

    ArchivePreviewWidget archivePreview(fixtures.filePath("archives/plain.zip"));
    QEventLoop archiveLoadLoop;
    QTimer archivePoll;
    archivePoll.setInterval(10);
    QObject::connect(&archivePoll, &QTimer::timeout, &archiveLoadLoop, [&] {
		ArchiveTreeWidget* archiveTree = archivePreview.findChild<ArchiveTreeWidget*>();
        if (archiveTree && archiveTree->topLevelItemCount() > 0)
            archiveLoadLoop.quit();
    });
    QTimer::singleShot(5000, &archiveLoadLoop, &QEventLoop::quit);
    archivePoll.start();
    archiveLoadLoop.exec();
    archivePoll.stop();
    ArchiveTreeWidget* archiveTree = archivePreview.findChild<ArchiveTreeWidget*>();
    check(archiveTree && archiveTree->topLevelItemCount() > 0,
          "Archive preview parses on a worker and populates its tree on the UI thread");

    ArchivePreviewWidget encryptedPreview(fixtures.filePath("archives/password-header.7z"));
    QEventLoop encryptedLoadLoop;
    QTimer encryptedPoll;
    bool passwordSubmitted = false;
    encryptedPoll.setInterval(10);
    QObject::connect(&encryptedPoll, &QTimer::timeout, &encryptedLoadLoop, [&] {
		if (!passwordSubmitted)
		{
			if (QLineEdit* passwordEdit = encryptedPreview.findChild<QLineEdit*>())
			{
				QPushButton* submitButton = passwordEdit->parentWidget()->findChild<QPushButton*>();
				if (submitButton)
				{
					passwordSubmitted = true;
					passwordEdit->setText("PreviewAll-Test-123!");
					submitButton->click();
				}
			}
		}
		if (ArchiveTreeWidget* archiveTree = encryptedPreview.findChild<ArchiveTreeWidget*>();
			archiveTree && archiveTree->topLevelItemCount() > 0)
			encryptedLoadLoop.quit();
    });
    QTimer::singleShot(7000, &encryptedLoadLoop, &QEventLoop::quit);
    encryptedPoll.start();
    encryptedLoadLoop.exec();
    encryptedPoll.stop();
    check(passwordSubmitted && encryptedPreview.findChild<ArchiveTreeWidget*>(),
          "Archive password requests cross from the worker to the UI and resume parsing");

    for (const auto& file : {"plain.zip", "plain.7z", "plain-rar5.rar", "password-header.7z"}) {
        ArchiveParser parser(fixtures.filePath(QString("archives/") + file));
        bool succeeded = false;
        QObject::connect(&parser, &ArchiveParser::parseSucceed, [&] { succeeded = true; });
        QObject::connect(&parser, &ArchiveParser::requestPassword, [&] { parser.setPassword("PreviewAll-Test-123!"); });
        parser.parseArchive();
        check(succeeded && parser.getFileCount() > 0, QString("Archive: ") + file);
    }
    for (const auto& locale : {"zh_CN", "en_US"}) {
        QTranslator translator;
        check(translator.load(QCoreApplication::applicationDirPath() + "/translations/previewall_" + locale + ".qm"),
              QString("Translation: ") + locale);
    }
    return failures ? 1 : 0;
}
