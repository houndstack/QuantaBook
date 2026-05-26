#include "quantabook/matching_engine.hpp"

#include <chrono>
#include <iostream>

int main() {
    using namespace quantabook;
    using clock = std::chrono::steady_clock;

    MatchingEngine engine;

    constexpr int num_submissions = 200000;
    constexpr int cancel_every = 5;

    int cancellations = 0;
    const auto start = clock::now();

    for (int i = 1; i <= num_submissions; ++i) {
        const Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
        const PriceTicks price = (side == Side::Buy) ? 10000 - (i % 25) : 10001 + (i % 25);
        engine.submit_limit_order(static_cast<OrderId>(i), side, price, 10);

        if (i % cancel_every == 0) {
            const auto cancel_id = static_cast<OrderId>(i - 2);
            if (engine.cancel_order(cancel_id)) {
                ++cancellations;
            }
        }
    }

    const auto elapsed = std::chrono::duration<double>(clock::now() - start).count();
    const auto trades_generated = engine.trades().size();
    const auto events = static_cast<double>(num_submissions + cancellations);

    std::cout << "Submissions: " << num_submissions << "\n";
    std::cout << "Cancellations: " << cancellations << "\n";
    std::cout << "Trades generated: " << trades_generated << "\n";
    std::cout << "Elapsed seconds: " << elapsed << "\n";
    std::cout << "Approx events/sec: " << (events / elapsed) << "\n";

    return 0;
}
