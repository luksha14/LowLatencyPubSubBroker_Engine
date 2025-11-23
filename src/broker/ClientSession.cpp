#include "ClientSession.h"
#include "SubscriptionManager.h"
#include <iostream>
#include <cstring>
#include "../common/logger.h"

using boost::asio::ip::tcp;

ClientSession::ClientSession(tcp::socket socket, SubscriptionManager& mgr)
    : socket_(std::move(socket)), manager_(mgr), msg_type_(0) {
}

ClientSession::~ClientSession() {
    if (!closed_) {
        Logger::warn("ClientSession destroyed before proper closure.");
    }
}

void ClientSession::handle_error_and_close() {
    if (closed_) return;

    closed_ = true;
    Logger::warn("Client disconnected/error, auto-unsubscribing.");
    
    manager_.unsubscribe_all(shared_from_this()); 
}

void ClientSession::start() {
    do_read_header();
}

void ClientSession::do_read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(&msg_type_, 1),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) {
                handle_error_and_close(); 
                return;
            }
            // Logika za čitanje ovisno o tipu poruke
            if (msg_type_ == static_cast<uint8_t>(MsgType::SUBSCRIBE)) {
                do_read_subscribe();
            } else if (msg_type_ == static_cast<uint8_t>(MsgType::DATA)) {
                do_read_data();
            } else {
                Logger::error("Received unknown msg type: " + std::to_string(static_cast<int>(msg_type_)));
                handle_error_and_close();
                return;
            }
        });
}

void ClientSession::do_read_subscribe() {
    auto self = shared_from_this();
    // read 4-byte topic id (big-endian) u prvih 4 bajta payload_buf_
    boost::asio::async_read(socket_, boost::asio::buffer(payload_buf_.data(), 4),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                handle_error_and_close();
                return;
            }
            int32_t topic = serializer::read_int32_be(payload_buf_.data());
            manager_.subscribe(topic, self);
            Logger::info("Client subscribed to topic " + std::to_string(topic));
            
            // continue reading next messages
            do_read_header();
        });
}

void ClientSession::do_read_data() {
    auto self = shared_from_this();
    // read payload (28 bytes) direktno u fiksni bafer
    boost::asio::async_read(socket_, boost::asio::buffer(payload_buf_.data(), PAYLOAD_SIZE),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) {
                handle_error_and_close();
                return;
            }
            
            const uint8_t* p = payload_buf_.data();
            int32_t topic = serializer::read_int32_be(p); p += 4;
            uint64_t ts = serializer::read_uint64_be(p); p += 8;
            double price = serializer::read_double_be(p); p += 8;
            double qty = serializer::read_double_be(p); // 

            auto subscribers = manager_.get_subscribers(topic);

            if (!subscribers.empty()) {
                Logger::info("Broker: Received DATA for Topic " + std::to_string(topic) + 
                             ", routing to " + std::to_string(subscribers.size()) + " subscribers.");
            } else {
                Logger::info("Broker: Received DATA for Topic " + std::to_string(topic) + 
                             ", but found 0 subscribers.");
            }

            std::vector<uint8_t> out;
            out.reserve(1 + PAYLOAD_SIZE);
            out.push_back(static_cast<uint8_t>(MsgType::DATA));
            out.insert(out.end(), payload_buf_.begin(), payload_buf_.end()); 

            for (auto &sub : subscribers) {
                if (sub) sub->deliver_raw(out);
            }
            do_read_header();
        });
}
void ClientSession::deliver_raw(const std::vector<uint8_t>& msg) {
    std::vector<uint8_t> buffer_copy = msg;
    
    auto self = shared_from_this();

    boost::asio::async_write(socket_, boost::asio::buffer(buffer_copy.data(), buffer_copy.size()),
        [this, self, buffer_copy](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                Logger::error("Subscriber deliver error: " + ec.message());
                handle_error_and_close(); 
            }
        });
}

void ClientSession::do_write() {
    auto self = shared_from_this();
    std::lock_guard<std::mutex> lock(write_mtx_);
    if (write_queue_.empty()) return;

    auto &buf = write_queue_.front();
    boost::asio::async_write(socket_, boost::asio::buffer(buf.data(), buf.size()),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                handle_error_and_close();
                return;
            }
            std::lock_guard<std::mutex> lock2(write_mtx_);
            write_queue_.pop_front();
            if (!write_queue_.empty()) {
                do_write();
            }
        });
}