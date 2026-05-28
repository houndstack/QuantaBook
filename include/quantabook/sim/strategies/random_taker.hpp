#pragma once

#include "quantabook/sim/agent.hpp"

#include <random>

namespace quantabook::sim {

class RandomTaker final : public IAgent {
  public:
    RandomTaker(AgentId agent_id, RandomTakerConfig config, std::uint64_t global_seed);

    AgentId id() const override;
    std::string name() const override;
    SimTime wakeup_interval() const override;
    void on_sim_start(const MarketObservation& observation, std::vector<AgentAction>& out_actions) override;
    void on_wakeup(const MarketObservation& observation, std::vector<AgentAction>& out_actions) override;

  private:
    void maybe_submit(const MarketObservation& observation, std::vector<AgentAction>& out_actions);

    AgentId agent_id_{};
    RandomTakerConfig config_{};
    std::mt19937_64 rng_;
    OrderId next_local_order_id_{1};
};

} // namespace quantabook::sim

