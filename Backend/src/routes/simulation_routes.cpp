#include "routes/simulation_routes.h"

#include "controller/simulation_controller.h"

void registerSimulationRoutes(crow::SimpleApp& app, ScoutingService& svc) {
    CROW_ROUTE(app, "/teams/<uint>/simulate").methods(crow::HTTPMethod::Get)(
        [&svc](const crow::request& req, std::uint64_t team_id) { return simulation_run(svc, req, team_id); });
}
