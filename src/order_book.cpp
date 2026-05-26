#include "quantabook/order_book.hpp"

#include <utility>

namespace quantabook {

bool OrderBook::order_id_exists(const OrderId order_id) const {
    return order_index_.find(order_id) != order_index_.end();
}

void OrderBook::add_resting_order(const Order& order) {
    if (order.side == Side::Buy) {
        auto& level = bids_[*order.limit_price];
        level.push_back(order);
        auto it = std::prev(level.end());
        order_index_[order.order_id] = OrderLocator{order.side, *order.limit_price, it};
    } else {
        auto& level = asks_[*order.limit_price];
        level.push_back(order);
        auto it = std::prev(level.end());
        order_index_[order.order_id] = OrderLocator{order.side, *order.limit_price, it};
    }
}

template <typename OppBook, typename CrossFn>
void OrderBook::match_against_book(Order& incoming,
                                   OppBook& opposite,
                                   SequenceNumber& next_trade_sequence,
                                   std::vector<Trade>& trades,
                                   CrossFn crosses) {
    while (incoming.remaining_quantity > 0 && !opposite.empty()) {
        auto level_it = opposite.begin();
        const PriceTicks level_price = level_it->first;
        if (!crosses(level_price)) {
            break;
        }

        auto& level_orders = level_it->second;
        while (incoming.remaining_quantity > 0 && !level_orders.empty()) {
            auto resting_it = level_orders.begin();
            Order& resting = *resting_it;
            const Quantity executed = std::min(incoming.remaining_quantity, resting.remaining_quantity);

            incoming.remaining_quantity -= executed;
            resting.remaining_quantity -= executed;

            Trade trade{};
            if (incoming.side == Side::Buy) {
                trade.buy_order_id = incoming.order_id;
                trade.sell_order_id = resting.order_id;
            } else {
                trade.buy_order_id = resting.order_id;
                trade.sell_order_id = incoming.order_id;
            }
            trade.execution_price = level_price;
            trade.executed_quantity = executed;
            trade.execution_sequence = next_trade_sequence++;
            trades.push_back(trade);

            if (resting.remaining_quantity == 0) {
                order_index_.erase(resting.order_id);
                level_orders.erase(resting_it);
            }
        }

        if (level_orders.empty()) {
            opposite.erase(level_it);
        }
    }
}

SubmitResult OrderBook::submit_limit_order(const OrderId order_id,
                                           const Side side,
                                           const PriceTicks limit_price,
                                           const Quantity quantity,
                                           const SequenceNumber sequence,
                                           SequenceNumber& next_trade_sequence) {
    SubmitResult result{};
    if (quantity == 0 || seen_order_ids_.contains(order_id) || order_id_exists(order_id)) {
        result.unfilled_quantity = quantity;
        return result;
    }
    seen_order_ids_.insert(order_id);

    Order incoming{order_id, side, OrderType::Limit, limit_price, quantity, quantity, sequence};
    if (side == Side::Buy) {
        match_against_book(incoming, asks_, next_trade_sequence, result.trades, [&](const PriceTicks ask_price) {
            return ask_price <= limit_price;
        });
    } else {
        match_against_book(incoming, bids_, next_trade_sequence, result.trades, [&](const PriceTicks bid_price) {
            return bid_price >= limit_price;
        });
    }

    result.accepted = true;
    result.filled_quantity = quantity - incoming.remaining_quantity;
    result.unfilled_quantity = incoming.remaining_quantity;

    if (incoming.remaining_quantity > 0) {
        add_resting_order(incoming);
    }

    return result;
}

SubmitResult OrderBook::submit_market_order(const OrderId order_id,
                                            const Side side,
                                            const Quantity quantity,
                                            const SequenceNumber sequence,
                                            SequenceNumber& next_trade_sequence) {
    SubmitResult result{};
    if (quantity == 0 || seen_order_ids_.contains(order_id) || order_id_exists(order_id)) {
        result.unfilled_quantity = quantity;
        return result;
    }
    seen_order_ids_.insert(order_id);

    Order incoming{order_id, side, OrderType::Market, std::nullopt, quantity, quantity, sequence};
    if (side == Side::Buy) {
        match_against_book(incoming, asks_, next_trade_sequence, result.trades, [&](const PriceTicks) { return true; });
    } else {
        match_against_book(incoming, bids_, next_trade_sequence, result.trades, [&](const PriceTicks) { return true; });
    }

    result.accepted = true;
    result.filled_quantity = quantity - incoming.remaining_quantity;
    result.unfilled_quantity = incoming.remaining_quantity;
    return result;
}

bool OrderBook::cancel_order(const OrderId order_id) {
    const auto loc_it = order_index_.find(order_id);
    if (loc_it == order_index_.end()) {
        return false;
    }

    const OrderLocator loc = loc_it->second;
    if (loc.side == Side::Buy) {
        auto level_it = bids_.find(loc.price);
        if (level_it == bids_.end()) {
            return false;
        }
        level_it->second.erase(loc.it);
        if (level_it->second.empty()) {
            bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(loc.price);
        if (level_it == asks_.end()) {
            return false;
        }
        level_it->second.erase(loc.it);
        if (level_it->second.empty()) {
            asks_.erase(level_it);
        }
    }

    order_index_.erase(loc_it);
    return true;
}

std::optional<PriceTicks> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<PriceTicks> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

} // namespace quantabook
