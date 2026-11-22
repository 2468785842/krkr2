#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <thread>
#include <vector>

#include "MainThreadMessageQueue.h"
#include "NativeEventQueue.h"

namespace krkr::event {
    MainThreadMessageQueue &MainThreadMessages() {
        static MainThreadMessageQueue queue;
        return queue;
    }
} // namespace krkr::event

TEST_CASE("main thread messages preserve FIFO and nested pump boundaries",
          "[events]") {
    krkr::event::MainThreadMessageQueue queue;
    std::vector<int> seen;
    queue.Post([&] {
        seen.push_back(1);
        queue.Post([&] { seen.push_back(3); });
    });
    queue.Post([&] {
        seen.push_back(2);
        queue.Dispatch();
        queue.Post([&] { seen.push_back(4); });
    });

    queue.Dispatch();
    REQUIRE(seen == std::vector<int>{ 1, 2, 3 });
    queue.Dispatch();
    REQUIRE(seen == std::vector<int>{ 1, 2, 3, 4 });
}

TEST_CASE(
    "main thread cancellation matches an owner and message across batches",
    "[events]") {
    krkr::event::MainThreadMessageQueue queue;
    int firstOwner = 0, secondOwner = 0;
    std::vector<int> seen;
    queue.Post([&] {
        queue.Post([&] {
            queue.Post([&] { seen.push_back(100); }, &firstOwner, 10);
            queue.Cancel(&firstOwner, 10);
        });
        queue.Post([&] { seen.push_back(200); }, &firstOwner, 10);
        queue.Dispatch();
    });
    queue.Post([&] { seen.push_back(300); }, &firstOwner, 10);
    queue.Post([&] { seen.push_back(1); }, &secondOwner, 10);
    queue.Post([&] { seen.push_back(2); }, &firstOwner, 20);

    queue.Dispatch();
    queue.Dispatch();
    REQUIRE(seen == std::vector<int>{ 1, 2 });
}

TEST_CASE("main thread batch unwinds after an exception", "[events]") {
    krkr::event::MainThreadMessageQueue queue;
    std::vector<int> seen;
    queue.Post([&] {
        queue.Post([&] { seen.push_back(1); });
        throw std::runtime_error("callback failed");
    });
    queue.Post([&] { seen.push_back(2); });

    REQUIRE_THROWS_AS(queue.Dispatch(), std::runtime_error);
    queue.Dispatch();
    REQUIRE(seen == std::vector<int>{ 1 });
}

TEST_CASE("worker posts execute on the thread that pumps engine messages",
          "[events]") {
    krkr::event::MainThreadMessageQueue queue;
    std::thread::id called;
    std::thread worker(
        [&] { queue.Post([&] { called = std::this_thread::get_id(); }); });
    worker.join();

    REQUIRE(called == std::thread::id{});
    queue.Dispatch();
    REQUIRE(called == std::this_thread::get_id());
}

TEST_CASE("native events carry a cancellable message ID and copied payload",
          "[events]") {
    struct Receiver {
        std::vector<std::intptr_t> received;
        void Receive(NativeEvent &event) {
            received.push_back(event.WParam);
            event.WParam = 0;
        }
    } receiver;
    auto &messages = krkr::event::MainThreadMessages();
    NativeEventQueue<Receiver> events(&receiver, &Receiver::Receive);
    NativeEvent copy(17);
    copy.WParam = 42;
    messages.Post([&] { events.Clear(16); });
    events.PostEvent(NativeEvent(16));
    events.PostEvent(copy);

    messages.Dispatch();
    REQUIRE(receiver.received == std::vector<std::intptr_t>{ 42 });
    REQUIRE(copy.WParam == 42);

    messages.Post([&] { events.Deallocate(); });
    events.PostEvent(copy);
    messages.Dispatch();
    REQUIRE(receiver.received == std::vector<std::intptr_t>{ 42 });
}
