#include "preview/archive/archiveparser.h"
#include "preview/archive/archivepreviewpage.h"
#include "preview/archive/archivetreewidget.h"
#include "preview/common/previewcontentstack.h"
#include "preview/common/previewpage.h"
#include "preview/previewwidget.h"
#include "preview/common/previewtitlebar.h"
#include "preview/image/imagepreviewpage.h"
#include "preview/image/imageviewportwidget.h"
#include "preview/common/previewtask.h"
#include "preview/markdown/markdownpreviewpage.h"
#include "preview/previewpagefactory.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImageReader>
#include <QFont>
#include <QLibrary>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QSemaphore>
#include <QScrollBar>
#include <QStackedLayout>
#include <QTextBrowser>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTreeWidget>
#include <QTranslator>
#include <QDebug>
#include <QTimer>
#include <QThreadPool>
#include <cstdio>
#include <memory>
#include <functional>

// Exercise deployed plugins and the actual archive parser without registry writes.
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    if (argc != 2 && !(argc == 3 && QString::fromLocal8Bit(argv[2]) == "--slow"))
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

    PreviewContentStack loadingState;
    loadingState.setAttribute(Qt::WA_DontShowOnScreen);
    loadingState.show();
    auto* pendingPage = new QWidget(&loadingState);
    loadingState.addPage(pendingPage);
    check(!pendingPage->isVisible(), "Pending content is hidden behind the loading page");
    QLabel* loadingLabel = nullptr;
    for (QLabel* label : loadingState.findChildren<QLabel*>())
        if (label->text() == "Loading...") loadingLabel = label;
    check(loadingLabel && !loadingLabel->isVisible(),
          "Shared loading indicator starts hidden");
    QEventLoop loadingLoop;
    QTimer::singleShot(220, &loadingLoop, &QEventLoop::quit);
    loadingLoop.exec();
    check(loadingLabel && loadingLabel->isVisible(),
          "Shared loading indicator appears after the delay");
    loadingState.showError("Loaded");
    check(loadingLabel && !loadingLabel->isVisible(),
          "Shared loading indicator stops on a result");

    PreviewWidget imagePreview(fixtures.filePath("images/png-alpha.png"));
    QEventLoop imageLoadLoop;
    QTimer imagePoll;
    imagePoll.setInterval(10);
    QObject::connect(&imagePoll, &QTimer::timeout, &imageLoadLoop, [&] {
        auto* content = imagePreview.findChild<PreviewContentStack*>();
        auto* page = imagePreview.findChild<ImagePreviewPage*>();
        if (content && page && content->layout()
            && static_cast<QStackedLayout*>(content->layout())->currentWidget() == page)
            imageLoadLoop.quit();
    });
    QTimer::singleShot(5000, &imageLoadLoop, &QEventLoop::quit);
    imagePoll.start();
    imageLoadLoop.exec();
    imagePoll.stop();
    auto* imageContent = imagePreview.findChild<PreviewContentStack*>();
    auto* imagePage = imagePreview.findChild<ImagePreviewPage*>();
    auto* imageViewport = imagePreview.findChild<ImageViewPortWidget*>();
    check(imageContent && imagePage && imageViewport && imageContent->layout()
              && static_cast<QStackedLayout*>(imageContent->layout())->currentWidget() == imagePage,
          "Image load replaces the shared loading state");

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

    PreviewWidget markdownPreview(fixtures.filePath("markdown/rich-syntax.md"));
    markdownPreview.setAttribute(Qt::WA_DontShowOnScreen);
    markdownPreview.resize(394, 515);
    markdownPreview.show();
    QTextBrowser* markdownBrowser = markdownPreview.findChild<QTextBrowser*>();
    check(markdownBrowser && !markdownBrowser->isVisible(),
          "Markdown content stays hidden until parsing finishes");
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

    PreviewWidget archivePreview(fixtures.filePath("archives/plain.zip"));
    auto hasCommonShell = [](PreviewWidget& widget) {
        return widget.findChildren<PreviewTitleBar*>(QString(), Qt::FindDirectChildrenOnly).size() == 1
            && widget.findChildren<PreviewContentStack*>(QString(), Qt::FindDirectChildrenOnly).size() == 1;
    };
    check(hasCommonShell(imagePreview) && hasCommonShell(markdownPreview)
              && hasCommonShell(archivePreview)
              && imagePreview.findChild<ImagePreviewPage*>()
              && markdownPreview.findChild<MarkdownPreviewPage*>()
              && archivePreview.findChild<ArchivePreviewPage*>(),
          "One PreviewWidget shell hosts each format-specific page");
    check(PreviewPageFactory::extensions(PreviewFormat::Image).contains(".png")
              && PreviewPageFactory::extensions(PreviewFormat::Archive).contains(".7z")
              && PreviewPageFactory::extensions(PreviewFormat::Markdown).contains(".md")
              && !PreviewPageFactory::extensions(PreviewFormat::Markdown).contains(".png"),
          "The format catalog exposes the registered extensions");
    PreviewWidget unsupportedPreview(fixtures.filePath("unknown.xyz"));
    bool unsupportedReported = false;
    for (const QLabel* label : unsupportedPreview.findChildren<QLabel*>())
        unsupportedReported |= label->text().contains("cannot be previewed");
    check(hasCommonShell(unsupportedPreview) && unsupportedReported
              && unsupportedPreview.findChild<PreviewContentStack*>()->state() == PreviewContentStack::State::Failed
              && !unsupportedPreview.findChild<PreviewPage*>(),
          "Unsupported suffix keeps the common shell and shows an explanation");
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

    PreviewWidget emptyArchive(fixtures.filePath("archives/empty.zip"));
    QEventLoop emptyLoop;
    QTimer emptyPoll;
    bool emptyReported = false;
    emptyPoll.setInterval(10);
    QObject::connect(&emptyPoll, &QTimer::timeout, &emptyLoop, [&] {
        for (const QLabel* label : emptyArchive.findChildren<QLabel*>())
            emptyReported |= label->text().contains("archive is empty");
        if (emptyReported) emptyLoop.quit();
    });
    QTimer::singleShot(5000, &emptyLoop, &QEventLoop::quit);
    emptyPoll.start();
    emptyLoop.exec();
    emptyPoll.stop();
    check(emptyReported && emptyArchive.findChild<PreviewContentStack*>()->state() == PreviewContentStack::State::Empty
              && !emptyArchive.findChild<ArchiveTreeWidget*>(),
          "Empty archive has a distinct content state");

    PreviewWidget encryptedPreview(fixtures.filePath("archives/password-header.7z"));
    QEventLoop encryptedLoadLoop;
    QTimer encryptedPoll;
    bool encryptedHeadersReported = false;
    encryptedPoll.setInterval(10);
    QObject::connect(&encryptedPoll, &QTimer::timeout, &encryptedLoadLoop, [&] {
		for (QLabel* label : encryptedPreview.findChildren<QLabel*>())
			if (label->text().contains("encrypted headers"))
				encryptedHeadersReported = true;
		if (encryptedHeadersReported)
			encryptedLoadLoop.quit();
    });
    QTimer::singleShot(7000, &encryptedLoadLoop, &QEventLoop::quit);
    encryptedPoll.start();
    encryptedLoadLoop.exec();
    encryptedPoll.stop();
    check(encryptedHeadersReported && !encryptedPreview.findChild<ArchiveTreeWidget*>(),
          "Encrypted archive headers report unavailable without prompting");

    for (const auto& file : {"plain.zip", "plain.7z", "plain-rar5.rar", "password-content.7z"}) {
        ArchiveParser parser(fixtures.filePath(QString("archives/") + file));
        const auto result = parser.parseArchive();
        check(result == ArchiveParser::ParseResult::Succeeded && parser.getFileCount() > 0,
              QString("Archive: ") + file);
    }
    ArchiveParser protectedHeaders(fixtures.filePath("archives/password-header.7z"));
    check(protectedHeaders.parseArchive() == ArchiveParser::ParseResult::HeaderEncrypted,
          "Archive parser rejects encrypted headers without waiting for input");

    QThreadPool busyPool;
    busyPool.setMaxThreadCount(1);
    QSemaphore gate;
    busyPool.start([&gate] { gate.acquire(); });
    QObject taskReceiver;
    PreviewTask obsoleteTask;
    bool obsoleteResultDelivered = false;
    obsoleteTask.start(busyPool, &taskReceiver,
        [](const std::atomic_bool&) { return 42; },
        [&obsoleteResultDelivered](int) { obsoleteResultDelivered = true; });
    obsoleteTask.cancel();
    gate.release();
    busyPool.waitForDone();
    app.processEvents();
    check(!obsoleteResultDelivered, "Cancelled queued preview work never updates the UI");

    QThreadPool runningPool;
    runningPool.setMaxThreadCount(1);
    QSemaphore entered;
    QSemaphore release;
    PreviewTask runningTask;
    bool runningResultDelivered = false;
    runningTask.start(runningPool, &taskReceiver,
        [&entered, &release](const std::atomic_bool&) {
            entered.release();
            release.acquire();
            return 7;
        },
        [&runningResultDelivered](int) { runningResultDelivered = true; });
    entered.acquire();
    runningTask.cancel();
    release.release();
    runningPool.waitForDone();
    app.processEvents();
    check(!runningResultDelivered, "Cancelled running preview work never updates the UI");
    for (const auto& locale : {"zh_CN", "en_US"}) {
        QTranslator translator;
        check(translator.load(QCoreApplication::applicationDirPath() + "/translations/previewall_" + locale + ".qm"),
              QString("Translation: ") + locale);
    }
    QTranslator qtTranslator;
    const bool qtTranslationLoaded = qtTranslator.load(
        QLocale("zh_CN"), "qt", "_", QCoreApplication::applicationDirPath() + "/translations");
    check(qtTranslationLoaded, "Qt translation: zh_CN");
    if (qtTranslationLoaded) {
        app.installTranslator(&qtTranslator);
        QTextBrowser browser;
        browser.setPlainText("PreviewAll");
        std::unique_ptr<QMenu> menu(browser.createStandardContextMenu());
        bool hasChineseSelectAll = false;
        for (const QAction* action : menu->actions())
            hasChineseSelectAll |= action->text().contains(QString::fromUtf8("全选"));
        check(hasChineseSelectAll, "Qt text context menu follows the Chinese UI locale");
        app.removeTranslator(&qtTranslator);
    }
    if (argc == 3) {
        auto checkSlowPreview = [&](QWidget& preview, const QString& name,
                                    const std::function<bool()>& ready) {
            preview.setAttribute(Qt::WA_DontShowOnScreen);
            preview.resize(400, 500);
            QElapsedTimer elapsed;
            elapsed.start();
            preview.show();
            bool indicatorSeen = false;
            QEventLoop loop;
            QTimer poll;
            poll.setInterval(10);
            QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
                for (QLabel* candidate : preview.findChildren<QLabel*>())
                    if (candidate->text() == "Loading..." && candidate->isVisible())
                        indicatorSeen = true;
                if (ready()) loop.quit();
            });
            QTimer::singleShot(30000, &loop, &QEventLoop::quit);
            poll.start();
            loop.exec();
            poll.stop();
            std::fprintf(stdout, "SLOW %s: %lld ms, loading indicator %s\n",
                         name.toUtf8().constData(), static_cast<long long>(elapsed.elapsed()),
                         indicatorSeen ? "visible" : "not observed");
            check(ready() && indicatorSeen, name + " shows loading then content");
        };
        {
            PreviewWidget preview(fixtures.filePath("images/slow-decode-6144x4096.jpg"));
            auto* content = preview.findChild<PreviewContentStack*>();
            auto* page = preview.findChild<ImagePreviewPage*>();
            checkSlowPreview(preview, "Slow image", [=] {
                return content && page && static_cast<QStackedLayout*>(content->layout())->currentWidget() == page;
            });
        }
        {
            QElapsedTimer parseTime;
            parseTime.start();
            ArchiveParser parser(fixtures.filePath("archives/slow-many-entries.zip"));
            const auto parseResult = parser.parseArchive();
            std::fprintf(stdout, "SLOW Archive parser: %lld ms, %llu files\n",
                         static_cast<long long>(parseTime.elapsed()),
                         static_cast<unsigned long long>(parser.getFileCount()));
            check(parseResult == ArchiveParser::ParseResult::Succeeded,
                  "Slow archive parses successfully");
            PreviewWidget preview(fixtures.filePath("archives/slow-many-entries.zip"));
            checkSlowPreview(preview, "Slow archive", [&] {
                auto* tree = preview.findChild<ArchiveTreeWidget*>();
                return tree && tree->topLevelItemCount() == 1
                    && tree->topLevelItem(0)->childCount() == 250;
            });
            auto* tree = preview.findChild<ArchiveTreeWidget*>();
            if (tree && tree->topLevelItemCount() == 1
                && tree->topLevelItem(0)->childCount() == 250) {
                auto* folder = tree->topLevelItem(0)->child(0);
                check(folder->childCount() == 0, "Archive folders remain lazy before expansion");
                folder->setExpanded(true);
                check(folder->childCount() == 1000, "Archive folder loads its entries on expansion");
            }
        }
        {
            PreviewWidget preview(fixtures.filePath("markdown/slow-syntax.md"));
            auto* content = preview.findChild<PreviewContentStack*>();
            auto* page = preview.findChild<MarkdownPreviewPage*>();
            checkSlowPreview(preview, "Slow Markdown", [=] {
                return content && page && static_cast<QStackedLayout*>(content->layout())->currentWidget() == page;
            });
        }
    }
    return failures ? 1 : 0;
}
