#include <crow.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "model/scouting_repository.h"
#include "routes/router.h"
#include "service/scouting_service.h"

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    ScoutingRepository repo;
    ScoutingService service(repo);
    if (!service.hydrate()) {
        std::cerr << "Hydrate failed: " << service.last_repo_error() << "\n";
        return 1;
    }

    crow::SimpleApp app;

    registerRoutes(app, service);

    app.port(8080).run();
    return 0;
}
