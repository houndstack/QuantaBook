#include "quantabook/matching_engine.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace quantabook;

TEST_CASE("crossing buy limit order matches resting ask") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Sell, 10010, 10);

    auto r = engine.submit_limit_order(2, Side::Buy, 10012, 6);
    REQUIRE(r.filled_quantity == 6);
    REQUIRE(r.unfilled_quantity == 0);
    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].execution_price == 10010);
    REQUIRE(engine.best_ask() == 10010);
}

TEST_CASE("crossing sell limit order matches resting bid") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Buy, 10000, 10);

    auto r = engine.submit_limit_order(2, Side::Sell, 9998, 4);
    REQUIRE(r.filled_quantity == 4);
    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].execution_price == 10000);
    REQUIRE(engine.best_bid() == 10000);
}

TEST_CASE("trade occurs at resting order price") {
    MatchingEngine engine;
    engine.submit_limit_order(11, Side::Sell, 10020, 5);
    auto r = engine.submit_limit_order(12, Side::Buy, 10025, 5);

    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].execution_price == 10020);
}

TEST_CASE("partial fill leaves correct remaining quantity") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Sell, 10010, 10);

    auto r = engine.submit_limit_order(2, Side::Buy, 10010, 15);
    REQUIRE(r.filled_quantity == 10);
    REQUIRE(r.unfilled_quantity == 5);
    REQUIRE(engine.best_bid() == 10010);
    REQUIRE_FALSE(engine.best_ask().has_value());
}

TEST_CASE("multi level sweep consumes prices in correct order") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Sell, 10010, 5);
    engine.submit_limit_order(2, Side::Sell, 10011, 5);
    engine.submit_limit_order(3, Side::Sell, 10012, 5);

    auto r = engine.submit_limit_order(4, Side::Buy, 10012, 12);
    REQUIRE(r.trades.size() == 3);
    REQUIRE(r.trades[0].execution_price == 10010);
    REQUIRE(r.trades[1].execution_price == 10011);
    REQUIRE(r.trades[2].execution_price == 10012);
    REQUIRE(r.unfilled_quantity == 0);
    REQUIRE(engine.best_ask() == 10012);
}

TEST_CASE("fifo priority holds for two orders at same price") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Sell, 10010, 4);
    engine.submit_limit_order(2, Side::Sell, 10010, 4);

    auto r = engine.submit_limit_order(3, Side::Buy, 10010, 6);
    REQUIRE(r.trades.size() == 2);
    REQUIRE(r.trades[0].sell_order_id == 1);
    REQUIRE(r.trades[0].executed_quantity == 4);
    REQUIRE(r.trades[1].sell_order_id == 2);
    REQUIRE(r.trades[1].executed_quantity == 2);
}

TEST_CASE("cancellation removes an active order") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Buy, 10000, 10);
    REQUIRE(engine.cancel_order(1));
    REQUIRE_FALSE(engine.best_bid().has_value());
}

TEST_CASE("cancelling unknown or already filled order fails cleanly") {
    MatchingEngine engine;
    REQUIRE_FALSE(engine.cancel_order(999));

    engine.submit_limit_order(1, Side::Sell, 10010, 5);
    engine.submit_limit_order(2, Side::Buy, 10010, 5);
    REQUIRE_FALSE(engine.cancel_order(1));
}

TEST_CASE("market order consumes available liquidity but never rests") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Sell, 10010, 5);
    auto r = engine.submit_market_order(2, Side::Buy, 3);

    REQUIRE(r.filled_quantity == 3);
    REQUIRE(r.unfilled_quantity == 0);
    REQUIRE_FALSE(engine.cancel_order(2));
    REQUIRE(engine.best_ask() == 10010);
}

TEST_CASE("oversized market order reports unfilled quantity") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Buy, 10000, 4);

    auto r = engine.submit_market_order(2, Side::Sell, 10);
    REQUIRE(r.filled_quantity == 4);
    REQUIRE(r.unfilled_quantity == 6);
    REQUIRE_FALSE(engine.best_bid().has_value());
}

TEST_CASE("filled price levels disappear from book") {
    MatchingEngine engine;
    engine.submit_limit_order(1, Side::Sell, 10010, 5);
    engine.submit_limit_order(2, Side::Buy, 10010, 5);

    REQUIRE_FALSE(engine.best_ask().has_value());
}

TEST_CASE("zero quantity market orders are rejected") {
    MatchingEngine engine;
    auto r = engine.submit_market_order(1, Side::Buy, 0);
    REQUIRE_FALSE(r.accepted);
    REQUIRE(r.filled_quantity == 0);
}
