#pragma once

#include <crow.h>

class ScoutingService;

void registerTeamRoutes(crow::SimpleApp& app, ScoutingService& svc);
