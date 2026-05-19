#pragma once

#include <cstdint>
#include <crow.h>

class ScoutingService;

crow::response teams_create(ScoutingService& svc, const crow::request& req);
crow::response teams_get(ScoutingService& svc, std::uint64_t team_id);
crow::response teams_update(ScoutingService& svc, const crow::request& req, std::uint64_t team_id);
crow::response teams_remove(ScoutingService& svc, const crow::request& req, std::uint64_t team_id);
crow::response teams_analysis(ScoutingService& svc, std::uint64_t team_id);
crow::response teams_add_player(ScoutingService& svc, const crow::request& req, std::uint64_t team_id);
crow::response teams_remove_player(ScoutingService& svc, const crow::request& req, std::uint64_t team_id,
                                   std::uint64_t player_id);
crow::response teams_apply_lineup(ScoutingService& svc, const crow::request& req, std::uint64_t team_id);
