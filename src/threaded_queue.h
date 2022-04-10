#include <future>
#include <list>

#ifndef IMAGE_PROCESSING_MODULE_H
#define IMAGE_PROCESSING_MODULE_H

template <class T>
class ThreadedQueue {
public:
    ThreadedQueue(size_t max_size=0): max_size{max_size}, _queue{}, mtx{}, cv{} {};
    ~ThreadedQueue();
    bool empty();
    bool full();
    size_t size();

    /* Blocks if queue full. */
    void enqueue(T item);

    void wait_if_full();

    /* Blocks if queue empty. */
    T dequeue();

private:
    size_t max_size;
    std::list<T> _queue;
    std::mutex mtx;
    std::condition_variable cv;
};


template <class T>
ThreadedQueue<T>::~ThreadedQueue() {
    std::lock_guard<std::mutex> lk(mtx);
}

template <class T>
bool ThreadedQueue<T>::empty() {
    std::lock_guard<std::mutex> lk(mtx);
    return _queue.empty();
}

template <class T>
bool ThreadedQueue<T>::full() {
    std::lock_guard<std::mutex> lk(mtx);
    return max_size == 0 ? false : _queue.size() >= max_size;
}

template <class T>
size_t ThreadedQueue<T>::size() {
    std::lock_guard<std::mutex> lk(mtx);
    return _queue.size();
}

template <class T>
void ThreadedQueue<T>::enqueue(T item) {
    bool was_empty{};
    {
        std::unique_lock<std::mutex> lk(mtx);
        was_empty = _queue.empty();
        cv.wait(lk, [this]{return max_size == 0 ? true : _queue.size() < max_size;});
        _queue.push_back(item);
    }
    if (was_empty) {
        cv.notify_all();
    }
}

template <class T>
void ThreadedQueue<T>::wait_if_full() {
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [this]{return max_size == 0 ? true : _queue.size() < max_size;});
}

template <class T>
T ThreadedQueue<T>::dequeue() {
    bool was_full{};
    T item{};
    {
        std::unique_lock<std::mutex> lk(mtx);
        was_full = max_size == 0 ? false : _queue.size() >= max_size;
        cv.wait(lk, [this]{return !_queue.empty();});
        item = _queue.front();
        _queue.pop_front();
    }
    if (was_full) {
        cv.notify_all();
    }
    return item;
}

#endif  // IMAGE_PROCESSING_MODULE
