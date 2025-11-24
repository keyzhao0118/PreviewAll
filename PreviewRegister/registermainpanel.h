#pragma once

#include <QWidget>
#include <QSystemTrayIcon>

class RegisterMainPanel : public QWidget
{
public:
	explicit RegisterMainPanel(QSystemTrayIcon* trayIcon = nullptr, QWidget* parent = nullptr);
	~RegisterMainPanel();

public slots:
	void onActivatedTrayIcon(QSystemTrayIcon::ActivationReason reason);

protected:
	virtual void showEvent(QShowEvent* event) override;
	virtual void closeEvent(QCloseEvent* event) override;

private:
	QSystemTrayIcon* m_trayIcon = nullptr;
};