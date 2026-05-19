#pragma once

#include <crow.h>

class ScoutingService;

void registerAuthRoutes(crow::SimpleApp& app, ScoutingService& svc);
