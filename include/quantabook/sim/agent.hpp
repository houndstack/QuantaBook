#pragma once

#include "quantabook/order.hpp"
#include "quantabook/sim/config.hpp"
#include "quantabook/sim/events.hpp"
#include "quantabook/sim/types.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace quantabook::sim {

struct MarketObservation {
    SimTime time{};
    std::optional<PriceTicks> best_bid{};
    std::optional<PriceTicks> best_ask{};
    std::optional<PriceTicks> midpoint{};
    std::optional<PriceTicks> visible_reference_value{};
    std::optional<PriceTicks> last_trade_price{};
    std::int64_t own_cash{};
    std::int64_t own_inventory{};
};

struct SubmitLimitAction {
    OrderId order_id{};
    Side side{};
    PriceTicks price{};
    Quantity quantity{};
};

struct SubmitMarketAction {
    OrderId order_id{};
    Side side{};
    Quantity quantity{};
};

struct CancelAction {
    OrderId order_id{};
};

using AgentAction = std::variant<SubmitLimitAction, SubmitMarketAction, CancelAction>;

class IAgent {
  public:
    virtual ~IAgent() = default;
    virtual AgentId id() const = 0;
    virtual std::string name() const = 0;
    virtual SimTime wakeup_interval() const = 0;
    virtual void on_sim_start(const MarketObservation& observation, std::vector<AgentAction>& out_actions) = 0;
    virtual void on_wakeup(const MarketObservation& observation, std::vector<AgentAction>& out_actions) = 0;
};

} // namespace quantabook::sim
