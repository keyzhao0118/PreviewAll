#include "codesearchbar.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>

CodeSearchBar::CodeSearchBar(QWidget* parent)
	: QWidget(parent)
{
	setFixedHeight(30);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(5, 0, 5, 0);
	layout->setSpacing(5);

	m_searchInput = new QLineEdit(this);
	m_searchInput->setPlaceholderText(tr("Search..."));
	m_searchInput->setMinimumWidth(120);
	m_searchInput->setMaximumWidth(300);
	m_searchInput->installEventFilter(this);

	m_matchLab = new QLabel(this);
	m_matchLab->setMinimumWidth(50);

	QString btnStyle =
		"QPushButton { border: none; border-radius: 4px; padding: 4px; background: transparent; }"
		"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
		"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }";

	QString toggleStyle =
		"QPushButton { border: 1px solid transparent; border-radius: 4px; padding: 4px; background: transparent; font-weight: bold; }"
		"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
		"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }"
		"QPushButton:checked { border-color: rgba(128, 128, 128, 150); background-color: rgba(128, 128, 128, 40); }";

	m_caseBtn = new QPushButton("Aa", this);
	m_caseBtn->setToolTip(tr("Match Case"));
	m_caseBtn->setFixedSize(28, 28);
	m_caseBtn->setCheckable(true);
	m_caseBtn->setStyleSheet(toggleStyle);
	connect(m_caseBtn, &QPushButton::toggled, this, [this](bool checked) {
		m_caseSensitive = checked;
		emit searchChanged(m_searchInput->text());
	});

	m_prevBtn = new QPushButton(this);
	m_prevBtn->setIcon(QIcon(":/svg/chevron-up.svg"));
	m_prevBtn->setIconSize(QSize(20, 20));
	m_prevBtn->setFixedSize(28, 28);
	m_prevBtn->setToolTip(tr("Previous Match"));
	m_prevBtn->setStyleSheet(btnStyle);
	// Rotate icon 180 degrees for "up" arrow
	connect(m_prevBtn, &QPushButton::clicked, this, &CodeSearchBar::findPrevious);

	m_nextBtn = new QPushButton(this);
	m_nextBtn->setIcon(QIcon(":/svg/chevron-down.svg"));
	m_nextBtn->setIconSize(QSize(20, 20));
	m_nextBtn->setFixedSize(28, 28);
	m_nextBtn->setToolTip(tr("Next Match"));
	m_nextBtn->setStyleSheet(btnStyle);
	connect(m_nextBtn, &QPushButton::clicked, this, &CodeSearchBar::findNext);

	m_closeBtn = new QPushButton(this);
	m_closeBtn->setIcon(QIcon(":/svg/x.svg"));
	m_closeBtn->setIconSize(QSize(20, 20));
	m_closeBtn->setFixedSize(28, 28);
	m_closeBtn->setToolTip(tr("Close"));
	m_closeBtn->setStyleSheet(btnStyle);
	connect(m_closeBtn, &QPushButton::clicked, this, &CodeSearchBar::closed);

	layout->addWidget(m_searchInput);
	layout->addWidget(m_matchLab);
	layout->addWidget(m_caseBtn);
	layout->addWidget(m_prevBtn);
	layout->addWidget(m_nextBtn);
	layout->addWidget(m_closeBtn);
	layout->addStretch();

	connect(m_searchInput, &QLineEdit::textChanged, this, &CodeSearchBar::searchChanged);

	QFont font("Microsoft YaHei");
	font.setPointSize(10);
	setFont(font);
}

QString CodeSearchBar::searchText() const
{
	return m_searchInput->text();
}

bool CodeSearchBar::isCaseSensitive() const
{
	return m_caseSensitive;
}

void CodeSearchBar::setMatchInfo(int current, int total)
{
	if (m_searchInput->text().isEmpty())
	{
		m_matchLab->setText(QString());
		m_searchInput->setStyleSheet(QString());
		return;
	}

	if (total == 0)
	{
		m_matchLab->setText(tr("No matches"));
		m_searchInput->setStyleSheet("QLineEdit { border: 1px solid red; }");
	}
	else
	{
		m_matchLab->setText(QString("%1/%2").arg(current).arg(total));
		m_searchInput->setStyleSheet(QString());
	}
}

void CodeSearchBar::focusInput()
{
	m_searchInput->setFocus();
	m_searchInput->selectAll();
}

bool CodeSearchBar::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_searchInput && event->type() == QEvent::KeyPress)
	{
		auto* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Escape)
		{
			emit closed();
			return true;
		}
		if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
		{
			if (keyEvent->modifiers() & Qt::ShiftModifier)
				emit findPrevious();
			else
				emit findNext();
			return true;
		}
	}
	return QWidget::eventFilter(obj, event);
}
