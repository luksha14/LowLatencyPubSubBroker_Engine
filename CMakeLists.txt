cmake_minimum_required(VERSION 3.16)
project(low_latency_pubsub LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_WIN32_WINNT=0x0A00)

find_package(Boost REQUIRED CONFIG COMPONENTS system asio)

add_executable(broker
    src/broker/main.cpp
    src/broker/ClientSession.cpp
    src/broker/SubscriptionManager.cpp
)

add_executable(publisher
    client_pub/main.cpp
)

add_executable(subscriber
    client_sub/main.cpp
)

target_include_directories(broker PRIVATE src src/common)
target_include_directories(publisher PRIVATE src src/common)
target_include_directories(subscriber PRIVATE src src/common)

target_link_libraries(broker
    Boost::system
    Boost::asio
    ws2_32
    Mswsock
)

target_link_libraries(publisher
    Boost::system
    Boost::asio
    ws2_32
    Mswsock
)

target_link_libraries(subscriber
    Boost::system
    Boost::asio
    ws2_32
    Mswsock
)