#include "routes/auth_routes.h"

#include "controller/auth_controller.h"

void registerAuthRoutes(crow::SimpleApp& app, ScoutingService& svc) {
    CROW_ROUTE(app, "/auth/register").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return auth_register(svc, req);
    });

    CROW_ROUTE(app, "/auth/login").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return auth_login(svc, req);
    });

    CROW_ROUTE(app, "/auth/logout").methods(crow::HTTPMethod::Post)([&svc](const crow::request& req) {
        return auth_logout(svc, req);
    });

    CROW_ROUTE(app, "/auth/me").methods(crow::HTTPMethod::Get)([&svc](const crow::request& req) {
        return auth_me(svc, req);
    });
}
