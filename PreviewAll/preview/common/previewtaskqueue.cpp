#include "previewtaskqueue.h"

#include <algorithm>
#include <utility>

namespace
{
	constexpr int MaxConcurrentPerLane = 2;
	constexpr size_t MaxPendingPerLane = 16;
}

PreviewTaskQueue& PreviewTaskQueue::instance()
{
	static PreviewTaskQueue queue;
	return queue;
}

PreviewTaskQueue::PreviewTaskQueue()
{
	for (QThreadPool* pool : { &m_responsive.pool, &m_blocking.pool })
	{
		pool->setMaxThreadCount(MaxConcurrentPerLane);
		pool->setExpiryTimeout(30000);
	}
}

PreviewTaskQueue::Queue& PreviewTaskQueue::queueFor(Lane lane)
{
	return lane == Lane::Blocking ? m_blocking : m_responsive;
}

bool PreviewTaskQueue::submit(Lane lane, std::shared_ptr<std::atomic_bool> cancelled,
	std::unique_ptr<QRunnable> work)
{
	Queue& queue = queueFor(lane);
	Job job { std::move(cancelled), std::move(work) };
	{
		std::lock_guard lock(queue.mutex);
		discardCancelled(queue);
		if (queue.active == MaxConcurrentPerLane && queue.pending.size() >= MaxPendingPerLane)
			return false;
		if (queue.active == MaxConcurrentPerLane)
		{
			queue.pending.push_back(std::move(job));
			return true;
		}
		++queue.active;
	}
	launch(queue, std::move(job));
	return true;
}

void PreviewTaskQueue::launch(Queue& queue, Job job)
{
	queue.pool.start(QRunnable::create([this, &queue, job = std::move(job)]() mutable {
		if (!job.cancelled->load(std::memory_order_relaxed))
			job.work->run();
		job.work.reset();
		finished(queue);
	}));
}

void PreviewTaskQueue::finished(Queue& queue)
{
	Job next;
	{
		std::lock_guard lock(queue.mutex);
		--queue.active;
		discardCancelled(queue);
		if (queue.pending.empty())
		{
			if (queue.active == 0)
				queue.idle.notify_all();
			return;
		}
		next = std::move(queue.pending.front());
		queue.pending.pop_front();
		++queue.active;
	}
	launch(queue, std::move(next));
}

void PreviewTaskQueue::waitForDone()
{
	for (Queue* queue : { &m_responsive, &m_blocking })
	{
		std::unique_lock lock(queue->mutex);
		queue->idle.wait(lock, [queue] { return queue->active == 0 && queue->pending.empty(); });
		lock.unlock();
		queue->pool.waitForDone();
	}
}

void PreviewTaskQueue::discardCancelled(Queue& queue)
{
	queue.pending.erase(std::remove_if(queue.pending.begin(), queue.pending.end(), [](const Job& job) {
		return job.cancelled->load(std::memory_order_relaxed);
	}), queue.pending.end());
}
