#pragma once
#include <cstdint>

#pragma pack(push, 1)

struct TradeMessage {
    int topic_id;
    uint64_t timestamp_ms;
    double price;
    double quantity;
};

#pragma pack(pop)

enum class MsgType : uint8_t {
    SUBSCRIBE = 0x01,
    DATA      = 0x02
};
