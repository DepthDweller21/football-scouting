#include "controller/simulation_controller.h"

#include "model/formation_graph.h"
#include "view/json_view.h"
#include "view/simulation_view.h"
#include "service/scouting_service.h"

crow::response simulation_run(ScoutingService& svc, const crow::request& req, std::uint64_t team_id) {
    std::uint64_t start_id = 0;
    parse_optional_u64(req, "startPlayerId", start_id);
    int max_steps = 12;
    parse_optional_int(req, "maxSteps", max_steps);
    unsigned risk_seed = 0;
    int seed_tmp = 0;
    if (parse_optional_int(req, "riskSeed", seed_tmp) && seed_tmp > 0) {
        risk_seed = static_cast<unsigned>(seed_tmp);
    }

    FormationGraph graph{};
    AttackSimulationResult attack{};
    std::string err;
    if (!svc.simulate_team_passes(team_id, start_id, max_steps, risk_seed, graph, attack, err)) {
        const int code = err == "team not found" ? 404 : 400;
        return json_error(code, code == 404 ? "NOT_FOUND" : "SIMULATE_FAILED", err);
    }

    return json_success(attack_simulation_json(graph, attack));
}