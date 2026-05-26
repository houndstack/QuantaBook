#include "quantabook/matching_engine.hpp"

#include <iostream>

int main() {
    quantabook::MatchingEngine engine;

    engine.submit_limit_order(1, quantabook::Side::Buy, 10000, 10);
    engine.submit_limit_order(2, quantabook::Side::Buy, 9995, 8);
    engine.submit_limit_order(3, quantabook::Side::Sell, 10010, 7);
    engine.submit_limit_order(4, quantabook::Side::Sell, 10015, 6);

    auto result = engine.submit_market_order(5, quantabook::Side::Buy, 9);

    std::cout << "Trades from last order:\n";
    for (const auto& t : result.trades) {
        std::cout << "  buy=" << t.buy_order_id << " sell=" << t.sell_order_id << " px=" << t.execution_price
                  << " qty=" << t.executed_quantity << " seq=" << t.execution_sequence << "\n";
    }

    std::cout << "Unfilled quantity: " << result.unfilled_quantity << "\n";

    std::cout << "Best bid: ";
    if (auto bid = engine.best_bid()) {
        std::cout << *bid;
    } else {
        std::cout << "none";
    }

    std::cout << "\nBest ask: ";
    if (auto ask = engine.best_ask()) {
        std::cout << *ask;
    } else {
        std::cout << "none";
    }
    std::cout << "\n";

    return 0;
}
