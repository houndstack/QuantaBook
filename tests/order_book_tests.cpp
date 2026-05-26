#include "quantabook/order_book.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace quantabook;

TEST_CASE("empty book has no best bid or ask") {
    OrderBook book;
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE_FALSE(book.best_ask().has_value());
}

TEST_CASE("adding one bid and one ask updates top of book") {
    OrderBook book;
    SequenceNumber seq = 1;

    REQUIRE(book.submit_limit_order(1, Side::Buy, 10000, 10, 1, seq).accepted);
    REQUIRE(book.submit_limit_order(2, Side::Sell, 10010, 12, 2, seq).accepted);

    REQUIRE(book.best_bid() == 10000);
    REQUIRE(book.best_ask() == 10010);
}

TEST_CASE("non crossing limit orders rest on the book") {
    OrderBook book;
    SequenceNumber seq = 1;

    auto buy = book.submit_limit_order(1, Side::Buy, 10000, 10, 1, seq);
    auto sell = book.submit_limit_order(2, Side::Sell, 10005, 10, 2, seq);

    REQUIRE(buy.trades.empty());
    REQUIRE(sell.trades.empty());
    REQUIRE(book.best_bid() == 10000);
    REQUIRE(book.best_ask() == 10005);
}

TEST_CASE("invalid zero quantity orders are rejected") {
    OrderBook book;
    SequenceNumber seq = 1;

    auto result = book.submit_limit_order(1, Side::Buy, 10000, 0, 1, seq);
    REQUIRE_FALSE(result.accepted);
    REQUIRE(result.unfilled_quantity == 0);
    REQUIRE_FALSE(book.best_bid().has_value());
}
