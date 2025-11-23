#include "SubscriptionManager.h"
#include "ClientSession.h"
#include <algorithm>
#include "../common/logger.h"

void SubscriptionManager::subscribe(int topic_id, std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto &vec = subs_[topic_id];
    for (auto &w : vec) {
        if (auto s = w.lock()) {
            if (s == session) return;
        }
    }
    vec.emplace_back(session);
}

void SubscriptionManager::unsubscribe(int topic_id, std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = subs_.find(topic_id);
    if (it == subs_.end()) return;
    auto &vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const std::weak_ptr<ClientSession>& w) {
        auto s = w.lock();
        return !s || s == session;
    }), vec.end());
    if (vec.empty()) subs_.erase(it);
}

void SubscriptionManager::unsubscribe_all(std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& [topic, subs] : subs_) {
        subs.erase(std::remove_if(subs.begin(), subs.end(),
            [&](const std::weak_ptr<ClientSession>& wp) {
                return wp.expired() || wp.lock() == session;
            }),
            subs.end());
    }
    Logger::info("Client auto-unsubscribed from all topics.");
}

std::vector<std::shared_ptr<ClientSession>> SubscriptionManager::get_subscribers(int topic_id) {
    std::vector<std::shared_ptr<ClientSession>> out;
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = subs_.find(topic_id);
    if (it == subs_.end()) return out;
    auto &vec = it->second;

    for (auto &w : vec) {
        if (auto s = w.lock()) out.push_back(s);
    }
    return out;
}

// Metoda za periodično čišćenje (poziva se iz timera)
void SubscriptionManager::cleanup_dead_sessions() {
    std::lock_guard<std::mutex> lock(mtx_);
    size_t cleaned_count = 0;
    
    for (auto it = subs_.begin(); it != subs_.end(); ) {
        auto& vec = it->second;
        
        size_t old_size = vec.size();
        vec.erase(std::remove_if(vec.begin(), vec.end(), 
            [&](const std::weak_ptr<ClientSession>& w) {
                return w.expired();
            }), vec.end());
        
        cleaned_count += (old_size - vec.size());

        if (vec.empty()) {
            it = subs_.erase(it);
        } else {
            ++it;
        }
    }
    if (cleaned_count > 0) {
        Logger::info("Cleanup complete. Removed " + std::to_string(cleaned_count) + " dead sessions.");
    }
}