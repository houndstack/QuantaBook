#include "quantabook/sim/strategies/inventory_aware_mm.hpp"

#include <algorithm>

namespace quantabook::sim {

InventoryAwareMm::InventoryAwareMm(const AgentId agent_id, const InventoryAwareMmConfig config)
    : agent_id_(agent_id), config_(config) {}

AgentId InventoryAwareMm::id() const { return agent_id_; }
std::string InventoryAwareMm::name() const { return "InventoryAwareMM"; }
SimTime InventoryAwareMm::wakeup_interval() const { return config_.wakeup_interval; }

void InventoryAwareMm::on_sim_start(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
    quote(observation, out_actions);
}

void InventoryAwareMm::on_wakeup(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
    quote(observation, out_actions);
}

void InventoryAwareMm::quote(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
    std::optional<PriceTicks> center = observation.midpoint;
    if (!center.has_value()) {
        center = observation.visible_reference_value;
    }
    if (!center.has_value()) {
        return;
    }

    if (active_bid_id_.has_value()) {
        out_actions.push_back(CancelAction{*active_bid_id_});
    }
    if (active_ask_id_.has_value()) {
        out_actions.push_back(CancelAction{*active_ask_id_});
    }

    const std::int64_t inv = std::clamp(observation.own_inventory, -config_.max_inventory_abs, config_.max_inventory_abs);
    const PriceTicks skew = static_cast<PriceTicks>(inv) * config_.inventory_skew_per_unit_ticks;
    const PriceTicks reservation = *center - skew;

    const PriceTicks bid_price = reservation - config_.base_quote_offset_ticks;
    const PriceTicks ask_price = reservation + config_.base_quote_offset_ticks;
    if (bid_price >= ask_price) {
        return;
    }

    const OrderId bid_id = next_local_order_id_++;
    const OrderId ask_id = next_local_order_id_++;
    active_bid_id_ = bid_id;
    active_ask_id_ = ask_id;

    out_actions.push_back(SubmitLimitAction{bid_id, Side::Buy, bid_price, config_.quote_quantity});
    out_actions.push_back(SubmitLimitAction{ask_id, Side::Sell, ask_price, config_.quote_quantity});
}

} // namespace quantabook::sim

