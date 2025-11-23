#pragma once
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <vector>
#include <mutex>
#include <array> 
#include "../common/message.h"
#include "../common/serializer.h"

class SubscriptionManager;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:

    static constexpr size_t PAYLOAD_SIZE = sizeof(TradeMessage);

    ClientSession(boost::asio::ip::tcp::socket socket, SubscriptionManager& mgr);
    ~ClientSession();

    void start();
    bool closed_ = false;
    void deliver_raw(const std::vector<uint8_t>& data);
    
    // Metoda za automatsko odjavljivanje pozvana iz asinkronog callbacka
    void handle_error_and_close(); 

private:
    void do_read_header();
    void do_read_subscribe();
    void do_read_data();
    void do_write();

    boost::asio::ip::tcp::socket socket_;
    SubscriptionManager& manager_;

    // write queue (thread-safe queue)
    std::deque<std::vector<uint8_t>> write_queue_;
    std::mutex write_mtx_;

    // read buffers
    uint8_t msg_type_; 
    // Fiksni bafer za čitanje cijelog payload-a (28 bajtova)
    std::array<uint8_t, PAYLOAD_SIZE> payload_buf_; 
};