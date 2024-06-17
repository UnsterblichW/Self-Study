
#ifndef PRODUCER_CONSUMER_QUEUE_H
#define PRODUCER_CONSUMER_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <atomic>
#include <type_traits>

template <typename T>
class ProducerConsumerQueue
{
private:
    mutable std::mutex _queueLock;
    std::queue<T> _queue;
    std::condition_variable _condition;
    std::atomic<bool> _shutdown;

public:

    ProducerConsumerQueue() : _shutdown(false) { }

    void Push(T const& value)
    {
        std::lock_guard<std::mutex> lock(_queueLock);
        _queue.push(value);

        _condition.notify_one();
    }

    void Push(T&& value)
    {
        std::lock_guard<std::mutex> lock(_queueLock);
        _queue.push(std::move(value));

        _condition.notify_one();
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(_queueLock);

        return _queue.empty();
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(_queueLock);

        return _queue.size();
    }

    bool Pop(T& value)
    {
        std::lock_guard<std::mutex> lock(_queueLock);

        if (_queue.empty() || _shutdown)
            return false;

        value = std::move(_queue.front());

        _queue.pop();

        return true;
    }

    void WaitAndPop(T& value)
    {
        std::unique_lock<std::mutex> lock(_queueLock);

        // we could be using .wait(lock, predicate) overload here but it is broken
        // https://connect.microsoft.com/VisualStudio/feedback/details/1098841
        while (_queue.empty() && !_shutdown)
            _condition.wait(lock);

        if (_queue.empty() || _shutdown)
            return;

        value = _queue.front();

        _queue.pop();
    }

    void Cancel()
    {
        std::unique_lock<std::mutex> lock(_queueLock);

        while (!_queue.empty())
        {
            T& value = _queue.front();

            if constexpr (std::is_pointer_v<T>)
                delete value;

            _queue.pop();
        }

        _shutdown = true;

        _condition.notify_all();
    }
};

#endif // PRODUCER_CONSUMER_QUEUE_H
