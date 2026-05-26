#pragma once

#include "quantabook/types.hpp"

#include <optional>

namespace quantabook {

enum class Side { Buy, Sell };
enum class OrderType { Limit, Market };

struct Order {
    OrderId order_id{};
    Side side{};
    OrderType order_type{};
    std::optional<PriceTicks> limit_price{};
    Quantity original_quantity{};
    Quantity remaining_quantity{};
    SequenceNumber sequence{};
};

} // namespace quantabook
