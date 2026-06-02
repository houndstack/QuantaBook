#include "quantabook/sim/simulator.hpp"
#include "quantabook/sim/strategies/baseline_mm.hpp"
#include "quantabook/sim/strategies/random_taker.hpp"

#include <iostream>
#include <memory>

int main() {
    using namespace quantabook::sim;

    SimulationConfig config{};
    config.seed = 12345;
    config.start_time = 0;
    config.end_time = 50;
    config.initial_fair_value = 10000;
    config.expose_reference_value_to_agents = true;
    config.mark_price_policy = MarkPricePolicy::LastTradeThenMidpointThenFairValue;
    config.background.enabled = true;
    config.background.interval = 1;

    BaselineMmConfig mm_config{};
    mm_config.wakeup_interval = 2;
    mm_config.quote_offset_ticks = 2;
    mm_config.quote_quantity = 1;

    Simulator simulator(config);
    simulator.add_agent(std::make_unique<SymmetricBaselineMm>(1, mm_config));
    simulator.add_agent(std::make_unique<RandomTaker>(2, RandomTakerConfig{1, 1, 77}, config.seed));
    simulator.run();

    const std::string out_dir = "output/sim_demo_seed_12345";
    simulator.export_csv(out_dir);

    std::cout << "Simulation complete.\n";
    std::cout << "Event log rows: " << simulator.event_log().size() << "\n";
    std::cout << "Fill log rows: " << simulator.fill_log().size() << "\n";
    std::cout << "Summary rows: " << simulator.summaries().size() << "\n";
    std::cout << "CSV output directory: " << out_dir << "\n";

    return 0;
}
