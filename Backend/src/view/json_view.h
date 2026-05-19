#pragma once

/** JSON responses and resource serialization — View layer for this HTTP API. */

#include "model/domain.h"

#include <crow.h>

#include <string>

crow::response json_success(crow::json::wvalue data, const std::string& message = "");
crow::response json_error(int http_status, const std::string& code, const std::string& message);

crow::json::wvalue user_public_json(const UserRecord& u);
crow::json::wvalue school_json(const SchoolRecord& s);
crow::json::wvalue player_json(const PlayerRecord& p);
crow::json::wvalue visit_json(const VisitRecord& v);
crow::json::wvalue report_json(const ReportRecord& r);
crow::json::wvalue team_json(const TeamRecord& t);
crow::json::wvalue team_player_json(const TeamPlayerRecord& tp);

bool bearer_token(const crow::request& req, std::string& token_out);

bool parse_optional_u64(const crow::request& req, const char* param, std::uint64_t& out);
bool parse_optional_int(const crow::request& req, const char* param, int& out);
bool parse_optional_stat_range(const crow::request& req, const char* min_key, const char* max_key, bool& use_min,
                               int& min_v, bool& use_max, int& max_v);
