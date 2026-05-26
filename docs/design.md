# QuantaBook Phase 1 Design

## Scope
This document covers Phase 1 only: a deterministic C++20 limit order book and matching foundation.

## Responsibilities

### `OrderBook`
- Owns resting liquidity and all active-order state.
- Implements price-time priority matching.
- Supports insert, match, cancellation, and top-of-book queries.
- Maintains internal invariants and order ID index.

### `MatchingEngine`
- Public entry point for submitting/cancelling orders.
- Assigns sequence numbers for deterministic FIFO ordering.
- Collects and exposes generated trades and order-submit outcomes.
- Delegates matching/book mutations to `OrderBook`.

## Types and Price Representation
- Prices are integer ticks (`PriceTicks`) only.
- No floating-point logic in matching or book comparisons.

Initial aliases (readability-first):
- `OrderId`
- `SequenceNumber`
- `Quantity`
- `PriceTicks`

Tradeoff:
- Aliases are lightweight and easy to read in interview discussion.
- Strong wrapper types give better type safety but add ceremony and operator boilerplate.
- Phase 1 uses aliases for clarity and speed of iteration; wrappers can be introduced later if needed.

## Data Structures

### Price-level books
- Asks: `std::map<PriceTicks, PriceLevel>` (ascending, best ask at `begin()`).
- Bids: `std::map<PriceTicks, PriceLevel, std::greater<PriceTicks>>` (descending, best bid at `begin()`).

### FIFO within level
- `PriceLevel` stores `std::list<RestingOrder>`.
- Orders append at tail; fills consume from head.
- This preserves strict FIFO for same-price orders.

### Order ID lookup / cancellation
- `std::unordered_map<OrderId, OrderLocator> order_index`.
- `OrderLocator` stores side, price, and list iterator.
- Cancellation finds locator in O(1) average and erases from list in O(1).
- If a level becomes empty, remove the map entry.

Why list over deque/vector for Phase 1:
- Arbitrary cancellation in deque/vector is O(n) and complicates stable references.
- `list` gives stable iterators and straightforward correctness.
- Cache-locality cost is accepted for this foundation.

## Matching Rules

### Aggressive limit orders
- Incoming buy limit matches best asks while `ask_price <= buy_limit`.
- Incoming sell limit matches best bids while `bid_price >= sell_limit`.
- Match resting head order first (FIFO).
- Trade execution price is resting order price.
- If incoming order remains after crossing stops, residual rests at incoming limit price.

### Market orders
- Market buy/sell sweeps opposite side best prices FIFO until quantity exhausted or book empty.
- Market orders never rest.
- Unfilled remainder is reported explicitly in submit result.

## Partial Fills
- Orders track `original_quantity` and `remaining_quantity`.
- Every trade decrements both participating orders' remaining quantity.
- Filled resting orders are removed from level and order index.

## Invariants
- No active order has `remaining_quantity == 0`.
- Every active order is present in exactly one price level and in `order_index`.
- Best bid price is strictly less than best ask price when both exist (book has no crossed resting state).
- Level order is FIFO by insertion sequence.
- Prices remain integer ticks everywhere in core matching path.

## Complexity Targets (Average/Typical)
- Insert resting limit order: O(log P) to find/create level + O(1) append/index.
- Cancel by order ID: O(1) lookup + O(1) erase + O(log P) possible empty-level erase.
- Best bid/ask query: O(1) via `begin()` checks.
- Matching an aggressive order: O(K * (1 + log P_cleanup)) where K is number of fills/consumed orders.

## Deliberately Postponed to Later Phases
- Latency model, exchange clock, and event scheduling.
- Advanced order types (IOC/FOK/stop/pegged/hidden).
- Self-trade prevention and participant/account model.
- Fees/rebates and queue-position alpha modeling.
- Multi-asset routing and cross-venue behavior.
- Strategy performance interpretation and market-making claims.
