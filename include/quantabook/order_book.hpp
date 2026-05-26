#pragma once

#include "quantabook/order.hpp"
#include "quantabook/trade.hpp"

#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace quantabook {

struct SubmitResult {
    bool accepted{false};
    Quantity filled_quantity{0};
    Quantity unfilled_quantity{0};
    std::vector<Trade> trades{};
};

class OrderBook {
  public:
    SubmitResult submit_limit_order(OrderId order_id,
                                    Side side,
                                    PriceTicks limit_price,
                                    Quantity quantity,
                                    SequenceNumber sequence,
                                    SequenceNumber& next_trade_sequence);

    SubmitResult submit_market_order(OrderId order_id,
                                     Side side,
                                     Quantity quantity,
                                     SequenceNumber sequence,
                                     SequenceNumber& next_trade_sequence);

    bool cancel_order(OrderId order_id);

    std::optional<PriceTicks> best_bid() const;
    std::optional<PriceTicks> best_ask() const;

  private:
    using PriceLevel = std::list<Order>;
    using AskBook = std::map<PriceTicks, PriceLevel>;
    using BidBook = std::map<PriceTicks, PriceLevel, std::greater<PriceTicks>>;

    struct OrderLocator {
        Side side{};
        PriceTicks price{};
        PriceLevel::iterator it{};
    };

    AskBook asks_;
    BidBook bids_;
    std::unordered_map<OrderId, OrderLocator> order_index_;

    bool order_id_exists(OrderId order_id) const;

    void add_resting_order(const Order& order);

    template <typename OppBook, typename CrossFn>
    void match_against_book(Order& incoming,
                            OppBook& opposite,
                            SequenceNumber& next_trade_sequence,
                            std::vector<Trade>& trades,
                            CrossFn crosses);
};

} // namespace quantabook
