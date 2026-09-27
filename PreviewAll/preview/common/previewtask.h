#pragma once

#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>
#include "previewtaskqueue.h"

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
			&& m_running && m_running->load(std::memory_order_relaxed) && m_unblock)
			m_unblock();
		m_unblock = {};
		m_running.reset();
		m_cancelled.reset();
	}

	// The receiver is a UI-thread lifetime guard; complete also runs on the UI
	// thread. A rejected load (no receiver or full queue) needs a terminal state.
	template <typename Worker, typename Complete>
	bool start(QObject* receiver, Worker worker, Complete complete,
		PreviewTaskQueue::Lane lane = PreviewTaskQueue::Lane::Responsive,
		std::function<void()> unblock = {})
	{
		cancel();
		if (!receiver)
			return false;
		auto cancelled = std::make_shared<std::atomic_bool>(false);
		auto running = std::make_shared<std::atomic_bool>(false);
		m_cancelled = cancelled;
		m_running = running;
		m_unblock = std::move(unblock);
		QPointer<QObject> guardedReceiver(receiver);
		auto work = std::unique_ptr<QRunnable>(QRunnable::create(
			[cancelled, running, guardedReceiver, worker = std::move(worker),
			 complete = std::move(complete)]() mutable {
				running->store(true, std::memory_order_relaxed);
				if (cancelled->load(std::memory_order_relaxed))
				{
					running->store(false, std::memory_order_relaxed);
					return;
				}
				auto result = worker(*cancelled);
				running->store(false, std::memory_order_relaxed);
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
		const bool accepted = PreviewTaskQueue::instance().submit(lane, cancelled, std::move(work));
		if (!accepted)
		{
			m_unblock = {};
			m_running.reset();
			m_cancelled.reset();
		}
		return accepted;
	}

private:
	std::shared_ptr<std::atomic_bool> m_cancelled;
	std::shared_ptr<std::atomic_bool> m_running;
	std::function<void()> m_unblock;
};
