#include "routes/teams_routes.h"

#include "controller/teams_controller.h"

void registerTeamRoutes(crow::SimpleApp& app, ScoutingService& svc) {
    CROW_ROUTE(app, "/teams").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return teams_create(svc, req);
    });

    CROW_ROUTE(app, "/teams/<uint>/lineup").methods(crow::HTTPMethod::Post)(
        [&svc](const crow::request& req, std::uint64_t team_id) { return teams_apply_lineup(svc, req, team_id); });

    CROW_ROUTE(app, "/teams/<uint>/players").methods(crow::HTTPMethod::Post)(
        [&svc](const crow::request& req, std::uint64_t team_id) { return teams_add_player(svc, req, team_id); });

    CROW_ROUTE(app, "/teams/<uint>/players/<uint>")
        .methods(crow::HTTPMethod::Delete)([&svc](const crow::request& req, std::uint64_t team_id,
                                                   std::uint64_t player_id) {
            return teams_remove_player(svc, req, team_id, player_id);
        });

    CROW_ROUTE(app, "/teams/<uint>/analysis").methods(crow::HTTPMethod::Get)(
        [&svc](std::uint64_t team_id) { return teams_analysis(svc, team_id); });

    CROW_ROUTE(app, "/teams/<uint>").methods(crow::HTTPMethod::Get)(
        [&svc](std::uint64_t team_id) { return teams_get(svc, team_id); });

    CROW_ROUTE(app, "/teams/<uint>").methods(crow::HTTPMethod::Put)(
        [&svc](const crow::request& req, std::uint64_t team_id) { return teams_update(svc, req, team_id); });

    CROW_ROUTE(app, "/teams/<uint>").methods(crow::HTTPMethod::Delete)(
        [&svc](const crow::request& req, std::uint64_t team_id) { return teams_remove(svc, req, team_id); });
}
