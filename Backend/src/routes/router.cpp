#include "routes/router.h"

#include "controller/health_controller.h"
#include "routes/auth_routes.h"
#include "routes/players_routes.h"
#include "routes/schools_routes.h"
#include "routes/simulation_routes.h"
#include "routes/formations_routes.h"
#include "routes/teams_routes.h"
#include "service/scouting_service.h"

void registerRoutes(crow::SimpleApp& app, ScoutingService& svc) {
    CROW_ROUTE(app, "/")([]() { return rootHandler(); });

    CROW_ROUTE(app, "/health")([]() { return healthHandler(); });

    CROW_ROUTE(app, "/db-health")([]() { return dbHealthHandler(); });

    registerAuthRoutes(app, svc);
    registerSchoolRoutes(app, svc);
    registerPlayerRoutes(app, svc);
    registerFormationRoutes(app, svc);
    registerTeamRoutes(app, svc);
    registerSimulationRoutes(app, svc);
}
