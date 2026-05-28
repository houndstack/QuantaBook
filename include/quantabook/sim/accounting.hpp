#pragma once

#include "quantabook/order.hpp"
#include "quantabook/sim/types.hpp"
#include "quantabook/trade.hpp"

#include <vector>

namespace quantabook::sim {

struct FillRecord {
    SimTime time{};
    AgentId agent_id{};
    OrderId order_id{};
    Side side{};
    PriceTicks price{};
    Quantity quantity{};
    SequenceNumber trade_sequence{};
};

struct PnlSnapshot {
    SimTime time{};
    AgentId agent_id{};
    std::int64_t cash{};
    std::int64_t inventory{};
    PriceTicks mark_price{};
    std::int64_t mtm_pnl{};
};

struct AgentAccount {
    AgentId agent_id{};
    std::int64_t cash{0};
    std::int64_t inventory{0};
    PriceTicks mark_price{0};
    std::vector<FillRecord> fills{};
    std::vector<PnlSnapshot> pnl_series{};
};

} // namespace quantabook::sim
