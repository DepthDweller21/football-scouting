#pragma once

#include <cstdint>
#include <crow.h>

class ScoutingService;

crow::response players_top(ScoutingService& svc, const crow::request& req);
crow::response players_potential(ScoutingService& svc, const crow::request& req);
crow::response players_get(ScoutingService& svc, std::uint64_t player_id);
crow::response players_create(ScoutingService& svc, const crow::request& req);
crow::response reports_create(ScoutingService& svc, const crow::request& req);
crow::response reports_for_player(ScoutingService& svc, std::uint64_t player_id);
