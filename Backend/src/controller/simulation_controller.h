#pragma once

#include <cstdint>
#include <crow.h>

class ScoutingService;

crow::response simulation_run(ScoutingService& svc, const crow::request& req, std::uint64_t team_id);
