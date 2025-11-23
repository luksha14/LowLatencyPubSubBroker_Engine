#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <array>
#include <vector> 
#include "../src/common/message.h"
#include "../src/common/serializer.h"
#include "../src/common/logger.h" 

using boost::asio::ip::tcp;

class SubscriberClient : public std::enable_shared_from_this<SubscriberClient> {
public:
    static constexpr size_t PAYLOAD_SIZE = sizeof(TradeMessage);

    SubscriberClient(boost::asio::io_context& io, const std::string& host, const std::string& port, int topic_id)
        : socket_(io), resolver_(io), topic_id_(topic_id) {

    }

    void start(const std::string& host, const std::string& port) {
        do_resolve_and_connect(host, port);
    }

private:
    void do_resolve_and_connect(const std::string& host, const std::string& port) {
        auto self = shared_from_this();
        resolver_.async_resolve(host, port, 
            [this, self](boost::system::error_code ec, tcp::resolver::results_type results) {
                if (ec) {
                    Logger::error("Resolve error: " + ec.message());
                    return;
                }
                boost::asio::async_connect(socket_, results,
                    [this, self](boost::system::error_code ec, const tcp::endpoint& ) {
                        if (ec) {
                            Logger::error("Connect error: " + ec.message());
                            return;
                        }
                        Logger::info("Subscriber connected to broker.");
                        do_send_subscribe();
                    });
            });
    }

    void do_send_subscribe() {
        // Priprema bafera za asinkrono slanje
        sub_message_.reserve(1 + sizeof(int32_t));
        sub_message_.push_back(static_cast<uint8_t>(MsgType::SUBSCRIBE));
        serializer::write_int32_be(sub_message_, static_cast<int32_t>(topic_id_));

        auto self = shared_from_this();
        boost::asio::async_write(socket_, boost::asio::buffer(sub_message_.data(), sub_message_.size()),
            [this, self](boost::system::error_code ec, std::size_t ) {
                if (ec) {
                    Logger::error("Subscribe send error: " + ec.message());
                    return;
                }
                Logger::info("Subscribed to topic " + std::to_string(topic_id_));
                
                do_read_header();
            });
    }

    void do_read_header() {
        auto self = shared_from_this();
        boost::asio::async_read(socket_, boost::asio::buffer(&msg_type_, 1),
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    Logger::error("Connection closed or read error: " + ec.message());
                    return;
                }
                if (msg_type_ == static_cast<uint8_t>(MsgType::DATA)) {
                    do_read_data();
                } else {
                    Logger::warn("Received unexpected message type: " + std::to_string(static_cast<int>(msg_type_)));
                }
            });
    }

    void do_read_data() {
        auto self = shared_from_this();
        boost::asio::async_read(socket_, boost::asio::buffer(payload_buf_.data(), PAYLOAD_SIZE),
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    Logger::error("Data read error: " + ec.message());
                    return;
                }
                
                const uint8_t* p = payload_buf_.data();
                int32_t topic_id = serializer::read_int32_be(p); p += 4;
                uint64_t ts = serializer::read_uint64_be(p); p += 8;
                double price = serializer::read_double_be(p); p += 8;
                double qty = serializer::read_double_be(p);

                std::cout << "[SUB: " << topic_id_ << "] topic=" << topic_id 
                          << " price=" << price << " qty=" << qty << "\n";
                
                do_read_header();
            });
    }

private:
    tcp::socket socket_;
    tcp::resolver resolver_;
    int topic_id_;
    uint8_t msg_type_; 
    std::array<uint8_t, PAYLOAD_SIZE> payload_buf_;
    std::vector<uint8_t> sub_message_; 
};

int main(int argc, char* argv[]) {
    try {
        int topic = 1;
        if (argc >= 2) topic = std::atoi(argv[1]);

        boost::asio::io_context io;

        auto client = std::make_shared<SubscriberClient>(io, "127.0.0.1", "8080", topic);
        client->start("127.0.0.1", "8080");
        Logger::info("Subscriber started for Topic " + std::to_string(topic));

        io.run();

    } catch (std::exception& e) {
        Logger::error("Subscriber Fatal Error: " + std::string(e.what()));
    }
    return 0;
}