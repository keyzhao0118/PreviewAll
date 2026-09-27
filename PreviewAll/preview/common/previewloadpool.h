#pragma once

#include <QThreadPool>

inline QThreadPool& previewLoadPool()
{
	static QThreadPool pool;
	static const bool configured = [] {
		pool.setMaxThreadCount(2);
		pool.setExpiryTimeout(30000);
		return true;
	}();
	Q_UNUSED(configured);
	return pool;
}
