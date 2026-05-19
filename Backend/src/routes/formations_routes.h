#pragma once

#include <crow.h>

class ScoutingService;

void registerFormationRoutes(crow::SimpleApp& app, ScoutingService& svc);
