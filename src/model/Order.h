#pragma once
#include <string>
#include <nlohmann/json.hpp>

enum class OrderStatus {
    RESERVED,
    REJECTED,
    PRODUCING,
    CONFIRMED,
    RELEASED
};

NLOHMANN_JSON_SERIALIZE_ENUM(OrderStatus, {
    { OrderStatus::RESERVED,  "RESERVED"  },
    { OrderStatus::REJECTED,  "REJECTED"  },
    { OrderStatus::PRODUCING, "PRODUCING" },
    { OrderStatus::CONFIRMED, "CONFIRMED" },
    { OrderStatus::RELEASED,  "RELEASED"  },
})

struct Order {
    std::string orderId;       // ORD-YYYYMMDD-NNNN
    std::string sampleId;
    std::string customerName;
    int quantity;
    OrderStatus status;
    std::string createdAt;     // YYYY-MM-DD HH:MM:SS
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Order, orderId, sampleId, customerName, quantity, status, createdAt)
