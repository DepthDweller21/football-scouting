#pragma once

#include <cstdint>
#include <crow.h>

class ScoutingService;

crow::response schools_list(ScoutingService& svc);
crow::response schools_get(ScoutingService& svc, std::uint64_t school_id);
crow::response schools_players(ScoutingService& svc, std::uint64_t school_id);
crow::response schools_create(ScoutingService& svc, const crow::request& req);
crow::response schools_update(ScoutingService& svc, const crow::request& req, std::uint64_t school_id);
crow::response schools_remove(ScoutingService& svc, const crow::request& req, std::uint64_t school_id);
crow::response visits_create(ScoutingService& svc, const crow::request& req);
