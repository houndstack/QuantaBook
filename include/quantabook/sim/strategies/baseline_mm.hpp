#pragma once

#include "quantabook/sim/agent.hpp"

#include <optional>

namespace quantabook::sim {

class SymmetricBaselineMm final : public IAgent {
  public:
    SymmetricBaselineMm(AgentId agent_id, BaselineMmConfig config);

    AgentId id() const override;
    std::string name() const override;
    SimTime wakeup_interval() const override;
    void on_sim_start(const MarketObservation& observation, std::vector<AgentAction>& out_actions) override;
    void on_wakeup(const MarketObservation& observation, std::vector<AgentAction>& out_actions) override;

  private:
    void quote(const MarketObservation& observation, std::vector<AgentAction>& out_actions);

    AgentId agent_id_{};
    BaselineMmConfig config_{};
    std::optional<OrderId> active_bid_id_{};
    std::optional<OrderId> active_ask_id_{};
    OrderId next_local_order_id_{1};
};

} // namespace quantabook::sim

