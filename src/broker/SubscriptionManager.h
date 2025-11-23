#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

// forward
class ClientSession;

class SubscriptionManager {
public:
    void subscribe(int topic_id, std::shared_ptr<ClientSession> session);
    void unsubscribe(int topic_id, std::shared_ptr<ClientSession> session);
    void unsubscribe_all(std::shared_ptr<ClientSession> session);
    std::vector<std::shared_ptr<ClientSession>> get_subscribers(int topic_id); 
    void cleanup_dead_sessions();

private:
    std::unordered_map<int, std::vector<std::weak_ptr<ClientSession>>> subs_;
    std::mutex mtx_;
};