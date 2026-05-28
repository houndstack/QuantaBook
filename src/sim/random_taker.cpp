#include "quantabook/sim/strategies/random_taker.hpp"

namespace quantabook::sim {

RandomTaker::RandomTaker(const AgentId agent_id, const RandomTakerConfig config, const std::uint64_t global_seed)
    : agent_id_(agent_id), config_(config), rng_(global_seed + config.seed_offset + agent_id) {}

AgentId RandomTaker::id() const { return agent_id_; }
std::string RandomTaker::name() const { return "RandomTaker"; }
SimTime RandomTaker::wakeup_interval() const { return config_.wakeup_interval; }

void RandomTaker::on_sim_start(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
    maybe_submit(observation, out_actions);
}

void RandomTaker::on_wakeup(const MarketObservation& observation, std::vector<AgentAction>& out_actions) {
    maybe_submit(observation, out_actions);
}

void RandomTaker::maybe_submit(const MarketObservation&, std::vector<AgentAction>& out_actions) {
    std::uniform_int_distribution<int> coin(0, 99);
    if (coin(rng_) < 60) {
        const Side side = (coin(rng_) < 50) ? Side::Buy : Side::Sell;
        out_actions.push_back(SubmitMarketAction{next_local_order_id_++, side, config_.order_quantity});
    }
}

} // namespace quantabook::sim

