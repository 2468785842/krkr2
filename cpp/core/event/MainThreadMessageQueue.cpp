#include "MainThreadMessageQueue.h"

#include <utility>

namespace krkr::event {
    void MainThreadMessageQueue::Post(std::function<void()> callback,
                                      const void *owner, unsigned message) {
        std::lock_guard<std::mutex> lock(mutex);
        pending.push_back({ std::move(callback), owner, message });
    }

    void MainThreadMessageQueue::Cancel(const void *owner, unsigned message) {
        // Destroy captured objects after unlocking in case a destructor posts.
        std::list<Message> discarded;
        std::lock_guard<std::mutex> lock(mutex);
        const auto cancel = [&](std::list<Message> &messages) {
            for(auto it = messages.begin(); it != messages.end();) {
                const auto current = it++;
                if(current->owner == owner &&
                   (!message || current->id == message))
                    discarded.splice(discarded.end(), messages, current);
            }
        };
        cancel(pending);
        for(auto *batch = active; batch; batch = batch->previous)
            cancel(batch->messages);
    }

    MainThreadMessageQueue::Batch::Batch(MainThreadMessageQueue &owner) :
        queue(owner) {
        std::lock_guard<std::mutex> lock(queue.mutex);
        messages.splice(messages.end(), queue.pending);
        previous = queue.active;
        queue.active = this;
    }

    MainThreadMessageQueue::Batch::~Batch() {
        std::lock_guard<std::mutex> lock(queue.mutex);
        queue.active = previous;
    }

    void MainThreadMessageQueue::Dispatch() {
        Batch batch(*this);
        for(;;) {
            std::function<void()> callback;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if(batch.messages.empty())
                    return;
                callback.swap(batch.messages.front().callback);
                batch.messages.pop_front();
            }
            callback();
        }
    }
} // namespace krkr::event
