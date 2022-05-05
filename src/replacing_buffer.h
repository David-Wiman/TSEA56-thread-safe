#include <future>
#include <list>

#ifndef REPLACING_BUFFER
#define REPLACING_BUFFER

template <class T>
class ReplacingBuffer {
public:
    ReplacingBuffer(): buffer{}, has_item{false}, mtx{}, cv{} {};
    ~ReplacingBuffer();
    bool empty();
    bool full();

    /* Replaces item if buffer full. */
    void store(T &item);

    void wait_if_full();

    /* Blocks if buffer empty. */
    T extract();

private:
    T buffer;
    bool has_item;
    std::mutex mtx;
    std::condition_variable cv;
};


template <class T>
ReplacingBuffer<T>::~ReplacingBuffer() {
    std::lock_guard<std::mutex> lk(mtx);
}

template <class T>
bool ReplacingBuffer<T>::empty() {
    std::lock_guard<std::mutex> lk(mtx);
    return !has_item;
}

template <class T>
bool ReplacingBuffer<T>::full() {
    std::lock_guard<std::mutex> lk(mtx);
    return has_item;
}

template <class T>
void ReplacingBuffer<T>::store(T &item) {
    bool was_empty{};
    {
        std::lock_guard<std::mutex> lk(mtx);
        was_empty = !has_item;
        buffer = std::move(item);
        has_item = true;
    }
    if (was_empty) {
        cv.notify_all();
    }
}

template <class T>
void ReplacingBuffer<T>::wait_if_full() {
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [this]{return !has_item;});
}

template <class T>
T ReplacingBuffer<T>::extract() {
    bool was_full{};
    T item{};
    {
        std::unique_lock<std::mutex> lk(mtx);
        was_full = has_item;
        cv.wait(lk, [this]{return has_item;});
        item = std::move(buffer);
    }
    if (was_full) {
        cv.notify_all();
    }
    return item;
}

#endif  // REPLACING_BUFFER
