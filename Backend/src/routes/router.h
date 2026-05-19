#pragma once

#include <crow.h>

class ScoutingService;

/** Registers all HTTP routes (health + API). Composition root for controllers. */
void registerRoutes(crow::SimpleApp& app, ScoutingService& svc);
