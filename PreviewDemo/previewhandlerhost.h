#pragma once

#include <QWidget>
#include <wrl/client.h>

struct IPreviewHandler;
struct IUnknown;

namespace Microsoft::WRL {
	template <typename T>
	class ComPtr;
}

class PreviewHandlerHost : public QWidget
{
	Q_OBJECT

public:
	explicit PreviewHandlerHost(QWidget* parent = nullptr);
	~PreviewHandlerHost() override;

	bool loadFile(const QString& filePath, QString* outError = nullptr);
	void unload();

protected:
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;

private:
	void updateHandlerRect();
	static bool resolvePreviewHandlerClsid(const QString& filePath, CLSID* outClsid, QString* outError);

	Microsoft::WRL::ComPtr<IUnknown> m_unknown;
	Microsoft::WRL::ComPtr<IPreviewHandler> m_handler;
	QString m_loadedFilePath;
};



