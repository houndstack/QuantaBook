#include "quantabook/sim/simulator.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace quantabook::sim {

namespace {
constexpr AgentId kBackgroundAgentId = 0;
}

bool Simulator::EventCompare::operator()(const ScheduledEvent& lhs, const ScheduledEvent& rhs) const {
    if (lhs.time != rhs.time) {
        return lhs.time > rhs.time;
    }
    return lhs.sequence > rhs.sequence;
}

Simulator::Simulator(SimulationConfig config)
    : config_(config), rng_(config.seed), fair_value_(config.initial_fair_value) {}

void Simulator::add_agent(std::unique_ptr<IAgent> agent) {
    if (!agent) {
        throw std::invalid_argument("agent must not be null");
    }
    const AgentId agent_id = agent->id();
    agents_[agent_id] = std::move(agent);
    accounts_[agent_id] = AgentAccount{agent_id, 0, 0, fair_value_, {}, {}};
}

void Simulator::run() {
    schedule_event(config_.start_time, FairValueUpdateEvent{fair_value_});
    if (config_.background.enabled) {
        schedule_event(config_.start_time, AgentWakeupEvent{kBackgroundAgentId});
    }

    for (const auto& [agent_id, agent] : agents_) {
        std::vector<AgentAction> actions;
        actions.reserve(4);
        agent->on_sim_start(make_observation(config_.start_time), actions);
        dispatch_agent_actions(agent_id, config_.start_time, actions);
        schedule_event(config_.start_time + agent->wakeup_interval(), AgentWakeupEvent{agent_id});
    }

    while (!event_queue_.empty()) {
        const ScheduledEvent event = event_queue_.top();
        if (event.time > config_.end_time) {
            break;
        }
        event_queue_.pop();
        process_event(event);
        snapshot_accounts(event.time);
    }
}

void Simulator::schedule_event(const SimTime time, EventPayload payload) {
    event_queue_.push(ScheduledEvent{time, next_event_sequence_++, std::move(payload)});
}

void Simulator::process_event(const ScheduledEvent& event) {
    std::visit(
        [&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, AddLimitOrderEvent>) {
                const SubmitResult result =
                    engine_.submit_limit_order(payload.order_id, payload.side, payload.price, payload.quantity);
                if (result.accepted) {
                    order_owner_[payload.order_id] = payload.agent_id;
                }
                event_log_.push_back(EventLogRow{
                    event.time, event.sequence, "AddLimitOrderEvent", payload.agent_id, payload.order_id,
                    payload.side == Side::Buy ? "Buy" : "Sell", payload.price, payload.quantity,
                    result.accepted ? "accepted" : "rejected",
                });
                apply_trades_to_accounts(event.time, result.trades);
            } else if constexpr (std::is_same_v<T, MarketOrderEvent>) {
                const SubmitResult result = engine_.submit_market_order(payload.order_id, payload.side, payload.quantity);
                if (result.accepted) {
                    order_owner_[payload.order_id] = payload.agent_id;
                }
                event_log_.push_back(EventLogRow{
                    event.time, event.sequence, "MarketOrderEvent", payload.agent_id, payload.order_id,
                    payload.side == Side::Buy ? "Buy" : "Sell", 0, payload.quantity, result.accepted ? "accepted" : "rejected",
                });
                apply_trades_to_accounts(event.time, result.trades);
            } else if constexpr (std::is_same_v<T, CancelOrderEvent>) {
                const bool ok = engine_.cancel_order(payload.order_id);
                if (ok) {
                    order_owner_.erase(payload.order_id);
                }
                event_log_.push_back(EventLogRow{
                    event.time, event.sequence, "CancelOrderEvent", payload.agent_id, payload.order_id, "", 0, 0,
                    ok ? "cancelled" : "not_found",
                });
            } else if constexpr (std::is_same_v<T, FairValueUpdateEvent>) {
                fair_value_ = payload.new_fair_value;
                event_log_.push_back(EventLogRow{
                    event.time, event.sequence, "FairValueUpdateEvent", 0, 0, "", fair_value_, 0, "applied",
                });
            } else if constexpr (std::is_same_v<T, AgentWakeupEvent>) {
                if (payload.agent_id == kBackgroundAgentId) {
                    process_background_step(event.time);
                    schedule_event(event.time + config_.background.interval, AgentWakeupEvent{kBackgroundAgentId});
                    return;
                }

                const auto it = agents_.find(payload.agent_id);
                if (it == agents_.end()) {
                    return;
                }
                std::vector<AgentAction> actions;
                actions.reserve(4);
                it->second->on_wakeup(make_observation(event.time), actions);
                dispatch_agent_actions(payload.agent_id, event.time, actions);
                schedule_event(event.time + it->second->wakeup_interval(), AgentWakeupEvent{payload.agent_id});
                event_log_.push_back(EventLogRow{
                    event.time, event.sequence, "AgentWakeupEvent", payload.agent_id, 0, "", 0, 0, "processed",
                });
            }
        },
        event.payload);
}

void Simulator::process_background_step(const SimTime time) {
    std::uniform_int_distribution<int> fair_step(config_.background.fair_value_step_min, config_.background.fair_value_step_max);
    fair_value_ += fair_step(rng_);
    schedule_event(time, FairValueUpdateEvent{fair_value_});

    const PriceTicks bid_px = fair_value_ - config_.background.base_spread_ticks;
    const PriceTicks ask_px = fair_value_ + config_.background.base_spread_ticks;

    schedule_event(time, AddLimitOrderEvent{
                            kBackgroundAgentId, make_background_order_id(), Side::Buy, bid_px,
                            config_.background.limit_order_quantity,
                        });
    schedule_event(time, AddLimitOrderEvent{
                            kBackgroundAgentId, make_background_order_id(), Side::Sell, ask_px,
                            config_.background.limit_order_quantity,
                        });

    std::uniform_int_distribution<int> coin(0, 99);
    if (coin(rng_) < 20) {
        const Side side = (coin(rng_) < 50) ? Side::Buy : Side::Sell;
        schedule_event(time, MarketOrderEvent{
                                kBackgroundAgentId, make_background_order_id(), side, config_.background.market_order_quantity,
                            });
    }

    if (!background_live_orders_.empty() && coin(rng_) < 20) {
        const OrderId cancel_id = background_live_orders_.back();
        background_live_orders_.pop_back();
        schedule_event(time, CancelOrderEvent{kBackgroundAgentId, cancel_id});
    }
}

void Simulator::dispatch_agent_actions(const AgentId agent_id, const SimTime time, const std::vector<AgentAction>& actions) {
    for (const auto& action : actions) {
        std::visit(
            [&](const auto& act) {
                using T = std::decay_t<decltype(act)>;
                if constexpr (std::is_same_v<T, SubmitLimitAction>) {
                    schedule_event(time, AddLimitOrderEvent{
                                            agent_id, compose_agent_order_id(agent_id, act.order_id), act.side, act.price, act.quantity,
                                        });
                } else if constexpr (std::is_same_v<T, SubmitMarketAction>) {
                    schedule_event(
                        time, MarketOrderEvent{agent_id, compose_agent_order_id(agent_id, act.order_id), act.side, act.quantity});
                } else if constexpr (std::is_same_v<T, CancelAction>) {
                    schedule_event(time, CancelOrderEvent{agent_id, compose_agent_order_id(agent_id, act.order_id)});
                }
            },
            action);
    }
}

MarketObservation Simulator::make_observation(const SimTime time) const {
    MarketObservation obs{};
    obs.time = time;
    obs.best_bid = engine_.best_bid();
    obs.best_ask = engine_.best_ask();
    obs.midpoint = current_midpoint();
    obs.last_trade_price = last_trade_price_;
    if (config_.expose_reference_value_to_agents) {
        obs.visible_reference_value = fair_value_;
    }
    return obs;
}

std::optional<PriceTicks> Simulator::current_midpoint() const {
    if (!engine_.best_bid().has_value() || !engine_.best_ask().has_value()) {
        return std::nullopt;
    }
    return (*engine_.best_bid() + *engine_.best_ask()) / 2;
}

std::optional<PriceTicks> Simulator::current_mark_price() const {
    if (last_trade_price_.has_value()) {
        return last_trade_price_;
    }
    if (auto mid = current_midpoint(); mid.has_value()) {
        return mid;
    }
    return fair_value_;
}

void Simulator::apply_trades_to_accounts(const SimTime time, const std::vector<Trade>& trades) {
    for (const auto& trade : trades) {
        last_trade_price_ = trade.execution_price;
        const auto buy_owner_it = order_owner_.find(trade.buy_order_id);
        if (buy_owner_it != order_owner_.end() && buy_owner_it->second != kBackgroundAgentId) {
            auto& acct = accounts_.at(buy_owner_it->second);
            acct.cash -= static_cast<std::int64_t>(trade.execution_price) * static_cast<std::int64_t>(trade.executed_quantity);
            acct.inventory += static_cast<std::int64_t>(trade.executed_quantity);
            FillRecord fill{time, buy_owner_it->second, trade.buy_order_id, Side::Buy, trade.execution_price, trade.executed_quantity,
                            trade.execution_sequence};
            acct.fills.push_back(fill);
            fill_log_.push_back(fill);
        }

        const auto sell_owner_it = order_owner_.find(trade.sell_order_id);
        if (sell_owner_it != order_owner_.end() && sell_owner_it->second != kBackgroundAgentId) {
            auto& acct = accounts_.at(sell_owner_it->second);
            acct.cash += static_cast<std::int64_t>(trade.execution_price) * static_cast<std::int64_t>(trade.executed_quantity);
            acct.inventory -= static_cast<std::int64_t>(trade.executed_quantity);
            FillRecord fill{
                time, sell_owner_it->second, trade.sell_order_id, Side::Sell, trade.execution_price, trade.executed_quantity,
                trade.execution_sequence,
            };
            acct.fills.push_back(fill);
            fill_log_.push_back(fill);
        }
    }
}

void Simulator::snapshot_accounts(const SimTime time) {
    const PriceTicks mark = *current_mark_price();
    for (auto& [agent_id, acct] : accounts_) {
        acct.mark_price = mark;
        const std::int64_t mtm = acct.cash + acct.inventory * static_cast<std::int64_t>(mark);
        acct.pnl_series.push_back(PnlSnapshot{time, agent_id, acct.cash, acct.inventory, mark, mtm});
    }
}

OrderId Simulator::make_background_order_id() {
    const OrderId id = next_background_order_id_++;
    background_live_orders_.push_back(id);
    return id;
}

OrderId Simulator::compose_agent_order_id(const AgentId agent_id, const OrderId local_order_id) const {
    return (agent_id << 32U) | (local_order_id & 0xFFFFFFFFULL);
}

void Simulator::export_csv(const std::string& output_dir) const {
    std::filesystem::create_directories(output_dir);
    write_event_log_csv(output_dir + "/simulation_event_log.csv");
    write_fill_log_csv(output_dir + "/fills.csv");
    write_pnl_csv(output_dir + "/agent_pnl_timeseries.csv");
}

void Simulator::write_event_log_csv(const std::string& path) const {
    std::ofstream out(path);
    out << "time,sequence,event_type,agent_id,order_id,side,price,quantity,status\n";
    for (const auto& row : event_log_) {
        out << row.time << "," << row.sequence << "," << row.event_type << "," << row.agent_id << "," << row.order_id << ","
            << row.side << "," << row.price << "," << row.quantity << "," << row.status << "\n";
    }
}

void Simulator::write_fill_log_csv(const std::string& path) const {
    std::ofstream out(path);
    out << "time,agent_id,order_id,side,price,quantity,trade_sequence\n";
    for (const auto& row : fill_log_) {
        out << row.time << "," << row.agent_id << "," << row.order_id << "," << (row.side == Side::Buy ? "Buy" : "Sell") << ","
            << row.price << "," << row.quantity << "," << row.trade_sequence << "\n";
    }
}

void Simulator::write_pnl_csv(const std::string& path) const {
    std::ofstream out(path);
    out << "time,agent_id,cash,inventory,mark_price,mtm_pnl\n";
    for (const auto& [_, acct] : accounts_) {
        for (const auto& row : acct.pnl_series) {
            out << row.time << "," << row.agent_id << "," << row.cash << "," << row.inventory << "," << row.mark_price << ","
                << row.mtm_pnl << "\n";
        }
    }
}

const std::vector<EventLogRow>& Simulator::event_log() const { return event_log_; }
const std::vector<FillRecord>& Simulator::fill_log() const { return fill_log_; }
const std::unordered_map<AgentId, AgentAccount>& Simulator::accounts() const { return accounts_; }
const MatchingEngine& Simulator::engine() const { return engine_; }

} // namespace quantabook::sim
