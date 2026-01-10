#include <MessageBus.hpp>
#include <mutex>
#include <shared_mutex>

namespace Singleton {
    MessageBus::MessageBus() {

    }

    MessageBus::~MessageBus() {

    }

    auto MessageBus::handle() -> void {
        if (count <= 0) {
            return;
        }
        
        {
            std::shared_lock<std::shared_mutex> lock1(subscribers_mutex);
            std::shared_lock<std::shared_mutex> lock2(messages_mutex);
            for(auto& [id, inner_mutex] : messages_inner_mutex) {
                std::lock_guard<std::mutex> inner_lock(*inner_mutex);
                auto msg_queue_it = messages.find(id);
                if (msg_queue_it != messages.end()) {
                    auto& msg_queue = msg_queue_it->second;
                    for (auto& msg : msg_queue) {
                        auto sub_callbak_it = subscribers.find(id);
                        if (sub_callbak_it != subscribers.end()) {
                            sub_callbak_it->second(msg);
                        }
                    }
                    msg_queue.clear();
                }
            }
        }
    }

    auto MessageBus::unsubscribe(IDCode receiver)-> void {
        {
            std::unique_lock<std::shared_mutex> lock(subscribers_mutex);
            subscribers.erase(receiver);
        }
        {
            std::unique_lock<std::shared_mutex> lock(messages_mutex);
            messages.erase(receiver);
            messages_inner_mutex.erase(receiver);
        }
    }
}