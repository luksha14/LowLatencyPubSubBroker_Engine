#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include "../src/common/message.h"
#include "../src/common/serializer.h"
#include "../src/common/logger.h" 

using boost::asio::ip::tcp;

class PublisherClient : public std::enable_shared_from_this<PublisherClient> {
public:
    PublisherClient(boost::asio::io_context& io) 
        : socket_(io), resolver_(io), timer_(io), message_count_(0) {
    }
    void start(const std::string& host, const std::string& port) {
        do_resolve(host, port);
    }

private:
    tcp::socket socket_;
    tcp::resolver resolver_;
    boost::asio::steady_timer timer_;
    std::vector<uint8_t> out_message_;
    int message_count_;
    std::mt19937_64 rng_ {std::random_device{}()};
    std::uniform_real_distribution<double> price_dist {100.0, 200.0};
    std::uniform_real_distribution<double> qty_dist {0.1, 5.0};
    std::uniform_int_distribution<int32_t> topic_dist {1, 3};

    void do_resolve(const std::string& host, const std::string& port) {
        auto self = shared_from_this();
        resolver_.async_resolve(host, port, 
            [this, self](boost::system::error_code ec, tcp::resolver::results_type results) {
                if (!ec) {
                    do_connect(results);
                } else {
                    Logger::error("Publisher Resolve error: " + ec.message());
                }
            });
    }

    void do_connect(const tcp::resolver::results_type& endpoints) {
        auto self = shared_from_this();
        boost::asio::async_connect(socket_, endpoints,
            [this, self](boost::system::error_code ec, const tcp::endpoint& ) {
                if (!ec) {
                    Logger::info("Publisher connected to broker");
                    start_send_loop(); 
                } else {
                    Logger::error("Publisher Connect error: " + ec.message());
                }
            });
    }

    void start_send_loop() {
        if (message_count_ >= 2000) return; 

        timer_.expires_after(std::chrono::seconds(1)); 
        
        auto self = shared_from_this();
        timer_.async_wait(
            [this, self](const boost::system::error_code& ec) {
                if (!ec) {
                    do_send_message(); 
                } else if (ec != boost::asio::error::operation_aborted) {
                    Logger::error("Publisher Timer error: " + ec.message());
                }
            });
    }

    void do_send_message() {
        TradeMessage msg;
        msg.topic_id = topic_dist(rng_);
        msg.timestamp_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        msg.price = price_dist(rng_);
        msg.quantity = qty_dist(rng_);

        out_message_.clear(); 
        out_message_.push_back(static_cast<uint8_t>(MsgType::DATA));
        serializer::write_int32_be(out_message_, msg.topic_id);
        serializer::write_uint64_be(out_message_, msg.timestamp_ms);
        serializer::write_double_be(out_message_, msg.price);
        serializer::write_double_be(out_message_, msg.quantity);

        auto self = shared_from_this();
        boost::asio::async_write(socket_, boost::asio::buffer(out_message_.data(), out_message_.size()),
            [this, self, msg](boost::system::error_code ec, std::size_t ) {
                if (ec) {
                    Logger::error("Publish send error: " + ec.message());
                    return;
                }
                message_count_++;
                if (message_count_ % 10 == 0) { 
                    Logger::info("Published message " + std::to_string(message_count_) + " to topic " + std::to_string(msg.topic_id));
                }
                start_send_loop(); 
            });
    }
};

int main() {
    try {
        boost::asio::io_context io;
        
        auto publisher = std::make_shared<PublisherClient>(io);

        publisher->start("127.0.0.1", "8080");

        io.run();

    } catch (std::exception& e) {
        Logger::error("Publisher Fatal Error: " + std::string(e.what()));
    }
    return 0;
}