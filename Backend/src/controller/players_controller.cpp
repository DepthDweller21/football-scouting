#include "controller/players_controller.h"

#include "controller/controller_helpers.h"
#include "view/json_view.h"
#include "service/scouting_service.h"

crow::response players_top(ScoutingService& svc, const crow::request& req) {
    const PlayerListQuery q = player_query_from_request(req);
    LinkedList<PlayerRecord> rows;
    svc.ranked_top_players(q, rows);
    crow::json::wvalue data;
    data["players"] = json_array_from_players(rows);
    return json_success(data);
}

crow::response players_potential(ScoutingService& svc, const crow::request& req) {
    const PlayerListQuery q = player_query_from_request(req);
    LinkedList<PlayerRecord> rows;
    svc.ranked_potential_players(q, rows);
    crow::json::wvalue data;
    data["players"] = json_array_from_players(rows);
    return json_success(data);
}

crow::response players_get(ScoutingService& svc, std::uint64_t player_id) {
    PlayerRecord p{};
    if (!svc.get_player(player_id, p)) {
        return json_error(404, "NOT_FOUND", "player not found");
    }
    crow::json::wvalue data;
    data["player"] = player_json(p);
    return json_success(data);
}

crow::response players_create(ScoutingService& svc, const crow::request& req) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_can_write_scouting(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body) {
        return json_error(400, "BAD_JSON", "invalid JSON body");
    }
    PlayerRecord p{};
    std::string verr;
    if (!parse_player_body(body, p, verr)) {
        return json_error(400, "VALIDATION_ERROR", verr);
    }
    std::uint64_t new_id = 0;
    std::string svc_err;
    if (!svc.create_player(p, new_id, svc_err)) {
        return json_error(400, "CREATE_FAILED", svc_err);
    }
    crow::json::wvalue data;
    data["id"] = new_id;
    return json_success(data, "created");
}

crow::response reports_create(ScoutingService& svc, const crow::request& req) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_can_write_scouting(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("playerId")) {
        return json_error(400, "VALIDATION_ERROR", "playerId required");
    }
    ReportRecord r{};
    r.player_id = static_cast<std::uint64_t>(body["playerId"].u());
    r.scout_user_id = u.id;
    if (body.has("visitId") && body["visitId"].t() != crow::json::type::Null) {
        r.visit_id = static_cast<std::uint64_t>(body["visitId"].u());
    }
    if (body.has("recommendation")) {
        r.recommendation = std::string(body["recommendation"].s());
    }
    if (body.has("strengths")) {
        r.strengths = std::string(body["strengths"].s());
    }
    if (body.has("weaknesses")) {
        r.weaknesses = std::string(body["weaknesses"].s());
    }
    std::uint64_t new_id = 0;
    std::string svc_err;
    if (!svc.create_report(r, new_id, svc_err)) {
        return json_error(400, "CREATE_FAILED", svc_err);
    }
    r.id = new_id;
    crow::json::wvalue data;
    data["report"] = report_json(r);
    return json_success(data, "created");
}

crow::response reports_for_player(ScoutingService& svc, std::uint64_t player_id) {
    LinkedList<ReportRecord> chain;
    if (!svc.reports_for_player(player_id, chain)) {
        return json_error(404, "NOT_FOUND", "player not found");
    }
    crow::json::wvalue data;
    data["reports"] = json_array_from_reports(chain);
    return json_success(data);
}
