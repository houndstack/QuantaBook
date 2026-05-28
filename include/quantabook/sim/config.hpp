#pragma once

#include "quantabook/sim/types.hpp"

namespace quantabook::sim {

struct BackgroundFlowConfig {
    bool enabled{true};
    SimTime interval{1};
    Quantity limit_order_quantity{5};
    Quantity market_order_quantity{3};
    PriceTicks base_spread_ticks{2};
    int fair_value_step_min{-1};
    int fair_value_step_max{1};
};

struct BaselineMmConfig {
    SimTime wakeup_interval{1};
    PriceTicks quote_offset_ticks{2};
    Quantity quote_quantity{1};
};

struct RandomTakerConfig {
    SimTime wakeup_interval{1};
    Quantity order_quantity{1};
    std::uint64_t seed_offset{1001};
};

struct InventoryAwareMmConfig {
    SimTime wakeup_interval{1};
    PriceTicks base_quote_offset_ticks{2};
    PriceTicks inventory_skew_per_unit_ticks{1};
    Quantity quote_quantity{1};
    std::int64_t max_inventory_abs{10};
};

struct SimulationConfig {
    std::uint64_t seed{42};
    SimTime start_time{0};
    SimTime end_time{100};
    PriceTicks initial_fair_value{10000};
    bool expose_reference_value_to_agents{false};
    BackgroundFlowConfig background{};
};

} // namespace quantabook::sim
