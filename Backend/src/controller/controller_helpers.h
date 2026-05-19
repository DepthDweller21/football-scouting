#pragma once

#include "model/domain.h"
#include "service/scouting_service.h"
#include "structures/LinkedList.h"
#include "view/json_view.h"

#include <crow.h>
#include <string>

PlayerListQuery player_query_from_request(const crow::request& req);

bool parse_player_body(const crow::json::rvalue& body, PlayerRecord& out, std::string& err);

crow::json::wvalue json_array_from_schools(const LinkedList<SchoolRecord>& list);
crow::json::wvalue json_array_from_players(const LinkedList<PlayerRecord>& list);
crow::json::wvalue json_array_from_reports(const LinkedList<ReportRecord>& list);
crow::json::wvalue json_array_from_team_players(const LinkedList<TeamPlayerRecord>& list);

/** Returns false and sets err_response when auth fails. */
bool auth_user_from_request(const crow::request& req, ScoutingService& svc, UserRecord& user_out,
                            crow::response& err_response);

bool auth_user_can_write_scouting(const crow::request& req, ScoutingService& svc, UserRecord& user_out,
                                  crow::response& err_response);

bool auth_user_is_admin(const crow::request& req, ScoutingService& svc, UserRecord& user_out,
                        crow::response& err_response);
