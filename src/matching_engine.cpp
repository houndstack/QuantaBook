#include "quantabook/matching_engine.hpp"

namespace quantabook {

SubmitResult MatchingEngine::submit_limit_order(const OrderId order_id,
                                                const Side side,
                                                const PriceTicks limit_price,
                                                const Quantity quantity) {
    SubmitResult result = book_.submit_limit_order(order_id, side, limit_price, quantity, next_order_sequence_++, next_trade_sequence_);
    trades_.insert(trades_.end(), result.trades.begin(), result.trades.end());
    return result;
}

SubmitResult MatchingEngine::submit_market_order(const OrderId order_id, const Side side, const Quantity quantity) {
    SubmitResult result = book_.submit_market_order(order_id, side, quantity, next_order_sequence_++, next_trade_sequence_);
    trades_.insert(trades_.end(), result.trades.begin(), result.trades.end());
    return result;
}

bool MatchingEngine::cancel_order(const OrderId order_id) {
    return book_.cancel_order(order_id);
}

std::optional<PriceTicks> MatchingEngine::best_bid() const { return book_.best_bid(); }
std::optional<PriceTicks> MatchingEngine::best_ask() const { return book_.best_ask(); }
const std::vector<Trade>& MatchingEngine::trades() const { return trades_; }

} // namespace quantabook
