#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono> 
#include "SubscriptionManager.h"
#include "ClientSession.h"
#include "../common/logger.h" 

using boost::asio::ip::tcp;

void start_cleanup_timer(boost::asio::io_context& io_context, SubscriptionManager& manager);

int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context io_context;

        SubscriptionManager manager;

        start_cleanup_timer(io_context, manager);

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));
        Logger::info("Broker listening on 0.0.0.0:8080");

        std::function<void()> do_accept;
        do_accept = [&]() {
            acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    Logger::info("New connection from " + socket.remote_endpoint().address().to_string());
                    auto session = std::make_shared<ClientSession>(std::move(socket), manager);
                    session->start();
                } else {
                    Logger::error("Accept error: " + ec.message());
                }
                do_accept();
            });
        };

        do_accept();

        unsigned int nthreads = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < nthreads - 1; ++i) { 
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        Logger::info("Running io_context on " + std::to_string(nthreads) + " threads.");

        // run u glavnoj niti
        io_context.run();

        for (auto &t : threads) if (t.joinable()) t.join();
    } catch (std::exception& e) {
        Logger::error("Broker Fatal: " + std::string(e.what()));
    }

    return 0;
}

void start_cleanup_timer(boost::asio::io_context& io_context, SubscriptionManager& manager) {
    auto timer = std::make_shared<boost::asio::steady_timer>(io_context, 
        std::chrono::seconds(5)); 
    
    timer->async_wait([&io_context, &manager, timer](const boost::system::error_code& ec) {
        if (!ec) {
            manager.cleanup_dead_sessions(); 
        } else {
            Logger::error("Cleanup timer error: " + ec.message());
        }
        start_cleanup_timer(io_context, manager); 
    });
}