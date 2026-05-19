#pragma once

#include <crow.h>

class ScoutingService;

void registerSimulationRoutes(crow::SimpleApp& app, ScoutingService& svc);
