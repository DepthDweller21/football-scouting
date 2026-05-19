#include "routes/players_routes.h"

#include "controller/players_controller.h"

void registerPlayerRoutes(crow::SimpleApp& app, ScoutingService& svc) {
    CROW_ROUTE(app, "/players/top").methods(crow::HTTPMethod::Get)([&svc](const crow::request& req) {
        return players_top(svc, req);
    });

    CROW_ROUTE(app, "/players/potential").methods(crow::HTTPMethod::Get)([&svc](const crow::request& req) {
        return players_potential(svc, req);
    });

    CROW_ROUTE(app, "/players/<uint>").methods(crow::HTTPMethod::Get)(
        [&svc](std::uint64_t player_id) { return players_get(svc, player_id); });

    CROW_ROUTE(app, "/players").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return players_create(svc, req);
    });

    CROW_ROUTE(app, "/reports").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return reports_create(svc, req);
    });

    CROW_ROUTE(app, "/reports/player/<uint>").methods(crow::HTTPMethod::Get)(
        [&svc](std::uint64_t player_id) { return reports_for_player(svc, player_id); });
}
