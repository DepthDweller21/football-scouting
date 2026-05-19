#include "routes/formations_routes.h"

#include "controller/formations_controller.h"

void registerFormationRoutes(crow::SimpleApp& app, ScoutingService& svc) {
    CROW_ROUTE(app, "/formations").methods(crow::HTTPMethod::Get)([]() { return formations_list(); });

    CROW_ROUTE(app, "/formations/<string>/suggest-lineup").methods(crow::HTTPMethod::Get)(
        [&svc](const crow::request& req, const std::string& formation_id) {
            return formations_suggest_lineup(svc, req, formation_id);
        });
}
