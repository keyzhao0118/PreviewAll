#pragma once

#include <QWidget>
#include <QSystemTrayIcon>

class QVBoxLayout;
class KeySlideSwitch;
class PreviewAppPanel : public QWidget
{
public:
	explicit PreviewAppPanel(QSystemTrayIcon* trayIcon = nullptr, QWidget* parent = nullptr);
	~PreviewAppPanel();

public slots:
	void onActivatedTrayIcon(QSystemTrayIcon::ActivationReason reason);

protected:
	virtual void showEvent(QShowEvent* event) override;
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void addSwitchCard(const QString& title, const QString& text, const QStringList& extList, KeySlideSwitch* switchControl);

private:
	QVBoxLayout* m_mainLayout = nullptr;
	KeySlideSwitch* m_textSwitch = nullptr;
	KeySlideSwitch* m_imageSwitch = nullptr;
	KeySlideSwitch* m_videoSwitch = nullptr;
	
	QSystemTrayIcon* m_trayIcon = nullptr;
};