#pragma once

#include <crow.h>

class ScoutingService;

crow::response formations_list();
crow::response formations_suggest_lineup(ScoutingService& svc, const crow::request& req, const std::string& formation_id);
