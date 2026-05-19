#pragma once

#include <crow.h>

class ScoutingService;

void registerSchoolRoutes(crow::SimpleApp& app, ScoutingService& svc);
