#include "quantabook/sim/strategies/baseline_mm.hpp"

namespace quantabook::sim {

SymmetricBaselineMm::SymmetricBaselineMm(const AgentId agent_id, const BaselineMmConfig config)
    : agent_id_(agent_id), config_(config) {}

AgentId SymmetricBaselineMm::id() const { return agent_id_; }
std::string SymmetricBaselineMm::name() const { return "SymmetricBaselineMM"; }
SimTime SymmetricBaselineMm::wakeup_interval() const { return config_.wakeup_interval; }

void SymmetricBaselineMm::on_sim_start(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
    quote(observation, out_actions);
}

void SymmetricBaselineMm::on_wakeup(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
    quote(observation, out_actions);
}

void SymmetricBaselineMm::quote(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
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

    const OrderId bid_id = next_local_order_id_++;
    const OrderId ask_id = next_local_order_id_++;
    active_bid_id_ = bid_id;
    active_ask_id_ = ask_id;

    out_actions.push_back(SubmitLimitAction{
        bid_id, Side::Buy, *center - config_.quote_offset_ticks, config_.quote_quantity,
    });
    out_actions.push_back(SubmitLimitAction{
        ask_id, Side::Sell, *center + config_.quote_offset_ticks, config_.quote_quantity,
    });
}

} // namespace quantabook::sim

