#pragma once

#include <QMenu>

class QAction;

class PreviewAllMenu : public QMenu
{
	Q_OBJECT

public:
	explicit PreviewAllMenu(QWidget* parent = nullptr);
	~PreviewAllMenu();

private:
	void initUi();
	void initConnect();
	void initCheckState();

private:
	QAction* m_actImagePreview = nullptr;
	QAction* m_actArchivePreview = nullptr;
	QAction* m_actCodePreview = nullptr;
	QAction* m_actExit = nullptr;
};