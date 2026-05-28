#include "quantabook/sim/simulator.hpp"
#include "quantabook/sim/strategies/baseline_mm.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>

using namespace quantabook;
using namespace quantabook::sim;

namespace {

class ScriptedAgent final : public IAgent {
  public:
    ScriptedAgent(const AgentId id, const SimTime interval, std::vector<AgentAction> start_actions, std::vector<AgentAction> wake_actions)
        : id_(id), interval_(interval), start_actions_(std::move(start_actions)), wake_actions_(std::move(wake_actions)) {}

    AgentId id() const override { return id_; }
    std::string name() const override { return "ScriptedAgent"; }
    SimTime wakeup_interval() const override { return interval_; }

    void on_sim_start(const MarketObservation&, std::vector<AgentAction>& out_actions) override {
        out_actions.insert(out_actions.end(), start_actions_.begin(), start_actions_.end());
    }

    void on_wakeup(const MarketObservation&, std::vector<AgentAction>& out_actions) override {
        out_actions.insert(out_actions.end(), wake_actions_.begin(), wake_actions_.end());
    }

  private:
    AgentId id_{};
    SimTime interval_{1};
    std::vector<AgentAction> start_actions_{};
    std::vector<AgentAction> wake_actions_{};
};

class VisibilitySpyAgent final : public IAgent {
  public:
    explicit VisibilitySpyAgent(const AgentId id) : id_(id) {}

    AgentId id() const override { return id_; }
    std::string name() const override { return "VisibilitySpy"; }
    SimTime wakeup_interval() const override { return 1; }

    void on_sim_start(const MarketObservation& observation, std::vector<AgentAction>&) override {
        saw_visible_reference_ = saw_visible_reference_ || observation.visible_reference_value.has_value();
    }

    void on_wakeup(const MarketObservation& observation, std::vector<AgentAction>&) override {
        saw_visible_reference_ = saw_visible_reference_ || observation.visible_reference_value.has_value();
    }

    bool saw_visible_reference() const { return saw_visible_reference_; }

  private:
    AgentId id_{};
    bool saw_visible_reference_{false};
};

} // namespace

TEST_CASE("deterministic run under fixed seed produces identical logs") {
    SimulationConfig cfg{};
    cfg.seed = 12345;
    cfg.start_time = 0;
    cfg.end_time = 20;
    cfg.initial_fair_value = 10000;
    cfg.expose_reference_value_to_agents = true;
    cfg.background.enabled = true;
    cfg.background.interval = 1;

    Simulator a(cfg);
    Simulator b(cfg);
    a.add_agent(std::make_unique<SymmetricBaselineMm>(1, BaselineMmConfig{}));
    b.add_agent(std::make_unique<SymmetricBaselineMm>(1, BaselineMmConfig{}));

    a.run();
    b.run();

    REQUIRE(a.event_log().size() == b.event_log().size());
    REQUIRE(a.fill_log().size() == b.fill_log().size());
    for (std::size_t i = 0; i < a.event_log().size(); ++i) {
        REQUIRE(a.event_log()[i].time == b.event_log()[i].time);
        REQUIRE(a.event_log()[i].sequence == b.event_log()[i].sequence);
        REQUIRE(a.event_log()[i].event_type == b.event_log()[i].event_type);
        REQUIRE(a.event_log()[i].status == b.event_log()[i].status);
    }
}

TEST_CASE("fills update cash and inventory correctly") {
    SimulationConfig cfg{};
    cfg.background.enabled = false;
    cfg.start_time = 0;
    cfg.end_time = 3;
    cfg.initial_fair_value = 10000;

    Simulator sim(cfg);
    sim.add_agent(std::make_unique<ScriptedAgent>(
        1, 10, std::vector<AgentAction>{SubmitLimitAction{1, Side::Buy, 100, 1}}, std::vector<AgentAction>{}));
    sim.add_agent(std::make_unique<ScriptedAgent>(
        2, 1, std::vector<AgentAction>{}, std::vector<AgentAction>{SubmitMarketAction{1, Side::Sell, 1}}));

    sim.run();

    const auto& accounts = sim.accounts();
    const auto& buyer = accounts.at(1);
    const auto& seller = accounts.at(2);

    REQUIRE(buyer.inventory == 1);
    REQUIRE(buyer.cash == -100);
    REQUIRE(seller.inventory == -1);
    REQUIRE(seller.cash == 100);
}

TEST_CASE("strategy cannot see forbidden reference value when disabled") {
    SimulationConfig cfg{};
    cfg.background.enabled = false;
    cfg.expose_reference_value_to_agents = false;
    cfg.end_time = 2;

    auto spy = std::make_unique<VisibilitySpyAgent>(7);
    VisibilitySpyAgent* spy_ptr = spy.get();

    Simulator sim(cfg);
    sim.add_agent(std::move(spy));
    sim.run();

    REQUIRE_FALSE(spy_ptr->saw_visible_reference());
}

TEST_CASE("event ordering is deterministic by time then sequence") {
    SimulationConfig cfg{};
    cfg.background.enabled = true;
    cfg.background.interval = 1;
    cfg.end_time = 10;
    cfg.seed = 9;
    cfg.expose_reference_value_to_agents = true;

    Simulator sim(cfg);
    sim.add_agent(std::make_unique<SymmetricBaselineMm>(1, BaselineMmConfig{}));
    sim.run();

    const auto& log = sim.event_log();
    REQUIRE_FALSE(log.empty());
    for (std::size_t i = 1; i < log.size(); ++i) {
        const auto& prev = log[i - 1];
        const auto& curr = log[i];
        REQUIRE((curr.time > prev.time || (curr.time == prev.time && curr.sequence > prev.sequence)));
    }
}

TEST_CASE("simple mark to market pnl arithmetic is correct") {
    SimulationConfig cfg{};
    cfg.background.enabled = false;
    cfg.start_time = 0;
    cfg.end_time = 3;
    cfg.initial_fair_value = 10000;

    Simulator sim(cfg);
    sim.add_agent(std::make_unique<ScriptedAgent>(
        1, 10, std::vector<AgentAction>{SubmitLimitAction{1, Side::Buy, 100, 1}}, std::vector<AgentAction>{}));
    sim.add_agent(std::make_unique<ScriptedAgent>(
        2, 1, std::vector<AgentAction>{}, std::vector<AgentAction>{SubmitMarketAction{1, Side::Sell, 1}}));
    sim.run();

    const auto& buyer = sim.accounts().at(1);
    const auto& seller = sim.accounts().at(2);
    REQUIRE_FALSE(buyer.pnl_series.empty());
    REQUIRE_FALSE(seller.pnl_series.empty());

    const auto& buyer_last = buyer.pnl_series.back();
    const auto& seller_last = seller.pnl_series.back();
    REQUIRE(buyer_last.mark_price == 100);
    REQUIRE(seller_last.mark_price == 100);
    REQUIRE(buyer_last.mtm_pnl == 0);
    REQUIRE(seller_last.mtm_pnl == 0);
}
