#pragma once

#include "IDGenerator.hpp"
#include <GameType.hpp>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Singleton {
    class MessageBus: public Singleton<MessageBus>
    {
        using SubscriberQueue = std::unordered_map<IDCode, std::function<void(std::shared_ptr<const Base::MessageBase>)>>;
        using MessageQueue = std::unordered_map<IDCode, std::vector<std::shared_ptr<const Base::MessageBase>>>;
        friend class Singleton<MessageBus>;
    public:
        
        // 发送消息
        template <typename T>
        requires std::derived_from<T, Base::MessageBase>
        auto send(IDCode receiver, std::shared_ptr<T> msg);

        // 广播
        template <typename T>
        requires std::derived_from<T, Base::MessageBase>
        auto broadcast(std::shared_ptr<T> msg);

        // 订阅消息
        template <typename T>
        requires std::derived_from<T, Base::MessageBase>
        auto subscribe(IDCode receiver, std::function<void(std::shared_ptr<const T>)> callback);

        // 取消订阅
        auto unsubscribe(IDCode receiver)-> void;

        // 处理消息
        auto handle() -> void;

    protected:
        MessageBus();
        ~MessageBus();
        MessageBus(const MessageBus&) = delete;
        MessageBus& operator=(const MessageBus&) = delete;

    private:
        std::shared_mutex subscribers_mutex;
        SubscriberQueue subscribers;
        std::unordered_map<IDCode, std::shared_ptr<std::mutex>> messages_inner_mutex;
        std::shared_mutex messages_mutex;
        MessageQueue messages;
        std::shared_mutex id_map_mutex;
        std::unordered_map<std::type_index, std::vector<IDCode>> id_map;
        std::atomic_int_fast64_t count = 0;
    };

    template <typename T>
    requires std::derived_from<T, Base::MessageBase>
    auto MessageBus::send(IDCode receiver, std::shared_ptr<T> msg) {
        {
            std::shared_lock<std::shared_mutex> lock1(messages_mutex);
            auto inner_mutex = messages_inner_mutex.find(receiver);
            if (inner_mutex != messages_inner_mutex.end()) {
                std::lock_guard<std::mutex> lock2(*inner_mutex->second);
                auto& queue = messages[receiver];
                queue.push_back(std::move(msg));
            }
        }
        count++;
    }

    template <typename T>
    requires std::derived_from<T, Base::MessageBase>
    auto MessageBus::subscribe(IDCode receiver, std::function<void(std::shared_ptr<const T>)> callback) {
        {
            std::unique_lock<std::shared_mutex> lock(subscribers_mutex);
            subscribers.insert_or_assign(receiver, [callback](std::shared_ptr<const Base::MessageBase> msg) {
                callback(std::static_pointer_cast<const T>(msg));
            });
        }
        {
            std::unique_lock<std::shared_mutex> lock(messages_mutex);
            messages.try_emplace(receiver, std::vector<std::shared_ptr<const Base::MessageBase>>());
            messages_inner_mutex.try_emplace(receiver, std::make_shared<std::mutex>());
        }
        {
            std::unique_lock<std::shared_mutex> lock(id_map_mutex);
            auto it = id_map.find(typeid(T));
            if (it != id_map.end()) {
                it->second.emplace_back(receiver);
            } else {
                id_map[typeid(T)].emplace_back(receiver);
            }
        }
    }

    template <typename T>
    requires std::derived_from<T, Base::MessageBase>
    auto MessageBus::broadcast(std::shared_ptr<T> msg) {
        {
            std::shared_lock<std::shared_mutex> lock(id_map_mutex);
            auto it = id_map.find(typeid(T));
            if (it != id_map.end()) {
                for(auto id: it->second) {
                    send<T>(id, msg);
                }
            }
        }
    }
}
