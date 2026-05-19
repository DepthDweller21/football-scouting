#pragma once

#include <crow.h>

class ScoutingService;

void registerPlayerRoutes(crow::SimpleApp& app, ScoutingService& svc);
