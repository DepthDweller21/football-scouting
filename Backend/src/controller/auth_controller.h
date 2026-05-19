#pragma once

#include <crow.h>

class ScoutingService;

crow::response auth_register(ScoutingService& svc, const crow::request& req);
crow::response auth_login(ScoutingService& svc, const crow::request& req);
crow::response auth_logout(ScoutingService& svc, const crow::request& req);
crow::response auth_me(ScoutingService& svc, const crow::request& req);
