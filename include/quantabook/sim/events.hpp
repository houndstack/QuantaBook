#pragma once

#include "quantabook/order.hpp"
#include "quantabook/sim/types.hpp"

#include <variant>

namespace quantabook::sim {

struct AddLimitOrderEvent {
    AgentId agent_id{};
    OrderId order_id{};
    Side side{};
    PriceTicks price{};
    Quantity quantity{};
};

struct MarketOrderEvent {
    AgentId agent_id{};
    OrderId order_id{};
    Side side{};
    Quantity quantity{};
};

struct CancelOrderEvent {
    AgentId agent_id{};
    OrderId order_id{};
};

struct FairValueUpdateEvent {
    PriceTicks new_fair_value{};
};

struct AgentWakeupEvent {
    AgentId agent_id{};
};

using EventPayload = std::variant<AddLimitOrderEvent, MarketOrderEvent, CancelOrderEvent, FairValueUpdateEvent, AgentWakeupEvent>;

struct ScheduledEvent {
    SimTime time{};
    EventSequence sequence{};
    EventPayload payload{};
};

} // namespace quantabook::sim

