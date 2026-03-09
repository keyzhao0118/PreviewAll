#pragma once

#include <QWidget>
#include <QTextDocument>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

class QLabel;
class QFile;
class QTextStream;
class QShortcut;
class CodeEditorWidget;
class CodeSearchBar;

class CodePreviewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CodePreviewWidget(const QString& filePath, QWidget* parent = nullptr);
	~CodePreviewWidget();

private:
	void initUi();
	void initStatusBar();
	void initTextEditor();
	void initSearchBar();
	void initHighlighter();
	void initShortcuts();
	void loadFile();
	void loadNextChunk();
	void updateInfoLabel();

	// search
	void performSearch(const QString& text);
	void highlightAllMatches();
	void findNext();
	void findPrevious();
	void clearSearchHighlights();
	void navigateToMatch(int index);

private slots:
	void onSearchBtnClicked();
	void onScrollValueChanged(int value);

private:
	QString m_filePath;

	QWidget* m_statusBar = nullptr;
	QLabel* m_infoLab = nullptr;
	CodeEditorWidget* m_textEditor = nullptr;
	CodeSearchBar* m_searchBar = nullptr;

	KSyntaxHighlighting::Repository* m_repository = nullptr;
	KSyntaxHighlighting::SyntaxHighlighter* m_highlighter = nullptr;

	// chunked loading
	QFile* m_file = nullptr;
	QTextStream* m_textStream = nullptr;
	bool m_isFullyLoaded = false;
	bool m_isLoadingChunk = false;
	int m_totalLineCount = 0;
	static constexpr int ChunkLineCount = 5000;
	static constexpr qint64 FileSizeThreshold = 1 * 1024 * 1024; // 1MB

	// search state
	QList<QTextCursor> m_matchPositions;
	int m_currentMatchIndex = -1;
};

