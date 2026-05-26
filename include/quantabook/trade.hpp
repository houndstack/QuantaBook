#pragma once

#include "quantabook/types.hpp"

namespace quantabook {

struct Trade {
    OrderId buy_order_id{};
    OrderId sell_order_id{};
    PriceTicks execution_price{};
    Quantity executed_quantity{};
    SequenceNumber execution_sequence{};
};

} // namespace quantabook
