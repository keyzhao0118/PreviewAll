#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class CodeSearchBar : public QWidget
{
	Q_OBJECT

public:
	explicit CodeSearchBar(QWidget* parent = nullptr);

	QString searchText() const;
	bool isCaseSensitive() const;
	void setMatchInfo(int current, int total);
	void focusInput();

signals:
	void searchChanged(const QString& text);
	void findNext();
	void findPrevious();
	void closed();

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	QLineEdit* m_searchInput = nullptr;
	QPushButton* m_prevBtn = nullptr;
	QPushButton* m_nextBtn = nullptr;
	QPushButton* m_caseBtn = nullptr;
	QPushButton* m_closeBtn = nullptr;
	QLabel* m_matchLab = nullptr;
	bool m_caseSensitive = false;
};
