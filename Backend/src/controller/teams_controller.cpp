#include "controller/teams_controller.h"

#include "controller/controller_helpers.h"
#include "model/formation_templates.h"
#include "view/json_view.h"
#include "service/scouting_service.h"

#include <vector>

crow::response teams_create(ScoutingService& svc, const crow::request& req) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_can_write_scouting(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("name") || !body.has("formation")) {
        return json_error(400, "VALIDATION_ERROR", "name and formation required");
    }
    TeamRecord t{};
    t.owner_user_id = u.id;
    t.name = std::string(body["name"].s());
    t.formation = std::string(body["formation"].s());
    std::uint64_t new_id = 0;
    std::string svc_err;
    if (!svc.create_team(t, new_id, svc_err)) {
        return json_error(400, "CREATE_FAILED", svc_err);
    }
    t.id = new_id;
    crow::json::wvalue data;
    data["team"] = team_json(t);
    return json_success(data, "created");
}

crow::response teams_get(ScoutingService& svc, std::uint64_t team_id) {
    TeamRecord t{};
    if (!svc.get_team(team_id, t)) {
        return json_error(404, "NOT_FOUND", "team not found");
    }
    LinkedList<TeamPlayerRecord> roster;
    svc.list_team_players(team_id, roster);
    crow::json::wvalue data;
    data["team"] = team_json(t);
    data["players"] = json_array_from_team_players(roster);
    return json_success(data);
}

crow::response teams_update(ScoutingService& svc, const crow::request& req, std::uint64_t team_id) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_from_request(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("name") || !body.has("formation")) {
        return json_error(400, "VALIDATION_ERROR", "name and formation required");
    }
    TeamRecord t{};
    t.id = team_id;
    t.name = std::string(body["name"].s());
    t.formation = std::string(body["formation"].s());
    std::string svc_err;
    if (!svc.update_team(t, u.id, u.role, svc_err)) {
        const int code = svc_err == "forbidden" ? 403 : (svc_err == "team not found" ? 404 : 400);
        return json_error(code, svc_err == "forbidden" ? "FORBIDDEN" : "UPDATE_FAILED", svc_err);
    }
    TeamRecord fresh{};
    svc.get_team(team_id, fresh);
    return json_success(team_json(fresh), "updated");
}

crow::response teams_remove(ScoutingService& svc, const crow::request& req, std::uint64_t team_id) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_from_request(req, svc, u, err)) {
        return err;
    }
    std::string svc_err;
    if (!svc.delete_team(team_id, u.id, u.role, svc_err)) {
        const int code = svc_err == "forbidden" ? 403 : (svc_err == "team not found" ? 404 : 400);
        return json_error(code, svc_err == "forbidden" ? "FORBIDDEN" : "DELETE_FAILED", svc_err);
    }
    crow::json::wvalue::object omap;
    return json_success(crow::json::wvalue(std::move(omap)), "deleted");
}

crow::response teams_analysis(ScoutingService& svc, std::uint64_t team_id) {
    int pc = 0;
    long long sum = 0;
    double avg = 0;
    std::string svc_err;
    if (!svc.team_analysis(team_id, pc, sum, avg, svc_err)) {
        return json_error(404, "NOT_FOUND", svc_err);
    }
    crow::json::wvalue data;
    data["playerCount"] = pc;
    data["sumCompositeFiveStats"] = sum;
    data["averageCompositeFiveStats"] = avg;
    return json_success(data);
}

crow::response teams_add_player(ScoutingService& svc, const crow::request& req, std::uint64_t team_id) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_from_request(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("playerId") || !body.has("assignedSpot")) {
        return json_error(400, "VALIDATION_ERROR", "playerId and assignedSpot required");
    }
    const std::uint64_t player_id = static_cast<std::uint64_t>(body["playerId"].u());
    bool ok_spot = false;
    const PreferredSpot spot = preferred_spot_from_string(std::string(body["assignedSpot"].s()), ok_spot);
    if (!ok_spot) {
        return json_error(400, "VALIDATION_ERROR", "invalid assignedSpot");
    }
    int order_index = 0;
    if (body.has("orderIndex")) {
        order_index = static_cast<int>(body["orderIndex"].i());
    }
    std::string svc_err;
    if (!svc.team_add_player(team_id, player_id, spot, order_index, u.id, u.role, svc_err)) {
        const int code = svc_err == "forbidden" ? 403 : 400;
        return json_error(code, svc_err == "forbidden" ? "FORBIDDEN" : "TEAM_PLAYER_FAILED", svc_err);
    }
    crow::json::wvalue::object omap;
    return json_success(crow::json::wvalue(std::move(omap)), "added");
}

crow::response teams_apply_lineup(ScoutingService& svc, const crow::request& req, std::uint64_t team_id) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_from_request(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("formation")) {
        return json_error(400, "VALIDATION_ERROR", "formation required");
    }
    const std::string formation_id = std::string(body["formation"].s());
    std::vector<LineupSlotPick> picks;

    if (body.has("lineup") && body["lineup"].t() == crow::json::type::List) {
        const auto& list = body["lineup"];
        picks.reserve(list.size());
        for (std::size_t i = 0; i < list.size(); ++i) {
            const auto& item = list[i];
            if (!item.has("playerId") || !item.has("assignedSpot")) {
                return json_error(400, "VALIDATION_ERROR", "each lineup entry needs playerId and assignedSpot");
            }
            bool ok_spot = false;
            LineupSlotPick pick{};
            pick.player_id = static_cast<std::uint64_t>(item["playerId"].u());
            pick.assigned_spot = preferred_spot_from_string(std::string(item["assignedSpot"].s()), ok_spot);
            if (!ok_spot) {
                return json_error(400, "VALIDATION_ERROR", "invalid assignedSpot in lineup");
            }
            pick.order_index = item.has("orderIndex") ? static_cast<int>(item["orderIndex"].i()) : static_cast<int>(i);
            picks.push_back(pick);
        }
    } else {
        bool use_school = false;
        std::uint64_t school_id = 0;
        if (body.has("schoolId")) {
            use_school = true;
            school_id = static_cast<std::uint64_t>(body["schoolId"].u());
        }
        std::string svc_err;
        if (!svc.suggest_lineup(formation_id, use_school, school_id, picks, svc_err)) {
            return json_error(400, "LINEUP_FAILED", svc_err);
        }
    }

    std::string svc_err;
    if (!svc.team_set_lineup(team_id, formation_id, picks, u.id, u.role, svc_err)) {
        const int code = svc_err == "forbidden" ? 403 : (svc_err == "team not found" ? 404 : 400);
        return json_error(code, svc_err == "forbidden" ? "FORBIDDEN" : "LINEUP_FAILED", svc_err);
    }
    return teams_get(svc, team_id);
}

crow::response teams_remove_player(ScoutingService& svc, const crow::request& req, std::uint64_t team_id,
                                   std::uint64_t player_id) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_from_request(req, svc, u, err)) {
        return err;
    }
    std::string svc_err;
    if (!svc.team_remove_player(team_id, player_id, u.id, u.role, svc_err)) {
        const int code = svc_err == "forbidden" ? 403 : 400;
        return json_error(code, svc_err == "forbidden" ? "FORBIDDEN" : "TEAM_PLAYER_FAILED", svc_err);
    }
    crow::json::wvalue::object omap;
    return json_success(crow::json::wvalue(std::move(omap)), "removed");
}
