#pragma once

#include <QRunnable>
#include <QThreadPool>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

// A small, process-wide scheduler for preview work. The blocking lane keeps
// third-party calls which cannot promptly stop out of the responsive lane.
class PreviewTask;

class PreviewTaskQueue
{
public:
	enum class Lane { Responsive, Blocking };

	static PreviewTaskQueue& instance();
	// Called only while the application is shutting down, after all pages close.
	void waitForDone();

private:
	friend class PreviewTask;
	PreviewTaskQueue();
	PreviewTaskQueue(const PreviewTaskQueue&) = delete;
	PreviewTaskQueue& operator=(const PreviewTaskQueue&) = delete;
	bool submit(Lane lane, std::shared_ptr<std::atomic_bool> cancelled,
		std::unique_ptr<QRunnable> work);

	struct Job
	{
		std::shared_ptr<std::atomic_bool> cancelled;
		std::unique_ptr<QRunnable> work;
	};
	struct Queue
	{
		QThreadPool pool;
		std::mutex mutex;
		std::condition_variable idle;
		std::deque<Job> pending;
		int active = 0;
	};

	Queue& queueFor(Lane lane);
	void launch(Queue& queue, Job job);
	void finished(Queue& queue);
	static void discardCancelled(Queue& queue);

	Queue m_responsive;
	Queue m_blocking;
};
