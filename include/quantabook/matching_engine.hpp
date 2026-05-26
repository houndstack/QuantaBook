#pragma once

#include "quantabook/order_book.hpp"

#include <optional>
#include <vector>

namespace quantabook {

class MatchingEngine {
  public:
    SubmitResult submit_limit_order(OrderId order_id, Side side, PriceTicks limit_price, Quantity quantity);
    SubmitResult submit_market_order(OrderId order_id, Side side, Quantity quantity);
    bool cancel_order(OrderId order_id);

    std::optional<PriceTicks> best_bid() const;
    std::optional<PriceTicks> best_ask() const;

    const std::vector<Trade>& trades() const;

  private:
    OrderBook book_;
    SequenceNumber next_order_sequence_{1};
    SequenceNumber next_trade_sequence_{1};
    std::vector<Trade> trades_;
};

} // namespace quantabook
