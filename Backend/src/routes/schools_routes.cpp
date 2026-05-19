#include "routes/schools_routes.h"

#include "controller/schools_controller.h"

void registerSchoolRoutes(crow::SimpleApp& app, ScoutingService& svc) {
    CROW_ROUTE(app, "/schools").methods(crow::HTTPMethod::Get)([&svc]() { return schools_list(svc); });

    CROW_ROUTE(app, "/schools/<uint>").methods(crow::HTTPMethod::Get)(
        [&svc](std::uint64_t school_id) { return schools_get(svc, school_id); });

    CROW_ROUTE(app, "/schools/<uint>/players").methods(crow::HTTPMethod::Get)(
        [&svc](std::uint64_t school_id) { return schools_players(svc, school_id); });

    CROW_ROUTE(app, "/schools").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return schools_create(svc, req);
    });

    CROW_ROUTE(app, "/schools/<uint>").methods(crow::HTTPMethod::Put)(
        [&svc](const crow::request& req, std::uint64_t school_id) {
            return schools_update(svc, req, school_id);
        });

    CROW_ROUTE(app, "/schools/<uint>").methods(crow::HTTPMethod::Delete)(
        [&svc](const crow::request& req, std::uint64_t school_id) {
            return schools_remove(svc, req, school_id);
        });

    CROW_ROUTE(app, "/visits").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return visits_create(svc, req);
    });
}
