#pragma once

#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

// Own one preview load. Destruction invalidates queued/running work without
// waiting for an uninterruptible decoder or archive library call.
class PreviewTask
{
public:
	PreviewTask() = default;
	PreviewTask(const PreviewTask&) = delete;
	PreviewTask& operator=(const PreviewTask&) = delete;
	~PreviewTask() { cancel(); }

	void cancel()
	{
		if (m_cancelled && !m_cancelled->exchange(true, std::memory_order_relaxed)
			&& m_unblock)
			m_unblock();
		m_unblock = {};
		m_cancelled.reset();
	}

	template <typename Worker, typename Complete>
	void start(QThreadPool& pool, QObject* receiver, Worker worker, Complete complete,
		std::function<void()> unblock = {})
	{
		cancel();
		auto cancelled = std::make_shared<std::atomic_bool>(false);
		m_cancelled = cancelled;
		m_unblock = std::move(unblock);
		QPointer<QObject> guardedReceiver(receiver);
		pool.start(QRunnable::create(
			[cancelled, guardedReceiver, worker = std::move(worker),
			 complete = std::move(complete)]() mutable {
				if (cancelled->load(std::memory_order_relaxed))
					return;
				auto result = worker(*cancelled);
				if (cancelled->load(std::memory_order_relaxed))
					return;
				QCoreApplication* app = QCoreApplication::instance();
				if (!app)
					return;
				QMetaObject::invokeMethod(app,
					[guardedReceiver, cancelled, result = std::move(result),
					 complete = std::move(complete)]() mutable {
						if (cancelled->load(std::memory_order_relaxed) || !guardedReceiver)
							return;
						complete(std::move(result));
					}, Qt::QueuedConnection);
			}));
	}

private:
	std::shared_ptr<std::atomic_bool> m_cancelled;
	std::function<void()> m_unblock;
};
