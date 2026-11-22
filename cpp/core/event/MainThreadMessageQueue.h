#pragma once

#include <functional>
#include <list>
#include <mutex>

namespace krkr::event {
    // Producers may post from any thread. Only the engine thread drains the
    // queue or destroys callback owners. A nested drain sees newly posted work.
    class MainThreadMessageQueue {
    public:
        void Post(std::function<void()> callback, const void *owner = nullptr,
                  unsigned message = 0);
        // Message zero matches every pending message for this owner.
        void Cancel(const void *owner, unsigned message = 0);
        void Dispatch();

    private:
        struct Message {
            std::function<void()> callback;
            const void *owner;
            unsigned id;
        };

        struct Batch {
            explicit Batch(MainThreadMessageQueue &queue);
            ~Batch();

            MainThreadMessageQueue &queue;
            std::list<Message> messages;
            Batch *previous = nullptr;
        };

        std::mutex mutex;
        std::list<Message> pending;
        Batch *active = nullptr;
    };

    // Supplied by the application, whose queue outlives the engine producers.
    MainThreadMessageQueue &MainThreadMessages();
} // namespace krkr::event
