#pragma once

#include "quantabook/matching_engine.hpp"
#include "quantabook/sim/accounting.hpp"
#include "quantabook/sim/agent.hpp"
#include "quantabook/sim/config.hpp"
#include "quantabook/sim/events.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace quantabook::sim {

struct EventLogRow {
    SimTime time{};
    EventSequence sequence{};
    std::string event_type{};
    AgentId agent_id{};
    OrderId order_id{};
    std::string side{};
    PriceTicks price{};
    Quantity quantity{};
    std::string status{};
};

class Simulator {
  public:
    explicit Simulator(SimulationConfig config);

    void add_agent(std::unique_ptr<IAgent> agent);
    void run();
    void export_csv(const std::string& output_dir) const;

    const std::vector<EventLogRow>& event_log() const;
    const std::vector<FillRecord>& fill_log() const;
    const std::unordered_map<AgentId, AgentAccount>& accounts() const;
    const MatchingEngine& engine() const;

  private:
    struct EventCompare {
        bool operator()(const ScheduledEvent& lhs, const ScheduledEvent& rhs) const;
    };

    void schedule_event(SimTime time, EventPayload payload);
    void process_event(const ScheduledEvent& event);
    void process_background_step(SimTime time);
    void dispatch_agent_actions(AgentId agent_id, SimTime time, const std::vector<AgentAction>& actions);
    MarketObservation make_observation(SimTime time, std::optional<AgentId> observer_agent_id = std::nullopt) const;
    std::optional<PriceTicks> current_midpoint() const;
    std::optional<PriceTicks> current_mark_price() const;
    void apply_trades_to_accounts(SimTime time, const std::vector<Trade>& trades);
    void snapshot_accounts(SimTime time);
    OrderId make_background_order_id();
    OrderId compose_agent_order_id(AgentId agent_id, OrderId local_order_id) const;
    void write_event_log_csv(const std::string& path) const;
    void write_fill_log_csv(const std::string& path) const;
    void write_pnl_csv(const std::string& path) const;

    SimulationConfig config_;
    MatchingEngine engine_{};
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, EventCompare> event_queue_{};
    std::unordered_map<AgentId, std::unique_ptr<IAgent>> agents_{};
    std::unordered_map<AgentId, AgentAccount> accounts_{};
    std::unordered_map<OrderId, AgentId> order_owner_{};
    std::vector<OrderId> background_live_orders_{};
    std::vector<EventLogRow> event_log_{};
    std::vector<FillRecord> fill_log_{};
    std::mt19937_64 rng_;
    EventSequence next_event_sequence_{1};
    PriceTicks fair_value_{};
    std::optional<PriceTicks> last_trade_price_{};
    OrderId next_background_order_id_{0xE000000000000000ULL};
};

} // namespace quantabook::sim
