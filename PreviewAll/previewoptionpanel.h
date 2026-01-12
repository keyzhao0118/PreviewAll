#pragma once

#include <QWidget>
#include <QSystemTrayIcon>

class QVBoxLayout;
class KeySlideSwitch;
class PreviewOptionPanel : public QWidget
{
	Q_OBJECT
public:
	explicit PreviewOptionPanel(QSystemTrayIcon* trayIcon = nullptr, QWidget* parent = nullptr);
	~PreviewOptionPanel();

public slots:
	void windowToTop();
	void onActivatedTrayIcon(QSystemTrayIcon::ActivationReason reason);

protected:
	virtual void showEvent(QShowEvent* event) override;
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void addSwitchCard(const QString& title, const QStringList& extList, KeySlideSwitch** switchControl);

private:
	QVBoxLayout* m_mainLayout = nullptr;
	
	KeySlideSwitch* m_imageSwitch = nullptr;
	KeySlideSwitch* m_archiveSwitch = nullptr;
	KeySlideSwitch* m_codeSwitch = nullptr;
	
	QSystemTrayIcon* m_trayIcon = nullptr;
};