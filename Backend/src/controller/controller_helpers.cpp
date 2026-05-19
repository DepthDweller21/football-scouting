#include "controller/controller_helpers.h"

#include <string>

PlayerListQuery player_query_from_request(const crow::request& req) {
    PlayerListQuery q{};
    int lim = 50;
    if (parse_optional_int(req, "limit", lim)) {
        q.limit = lim;
    }
    std::uint64_t sid = 0;
    if (parse_optional_u64(req, "schoolId", sid)) {
        q.use_school_filter = true;
        q.school_id = sid;
    }
    const char* spot_ptr = req.url_params.get("preferredSpot");
    if (spot_ptr) {
        bool ok = false;
        PreferredSpot ps = preferred_spot_from_string(std::string(spot_ptr), ok);
        if (ok) {
            q.use_spot_filter = true;
            q.preferred_spot = ps;
        }
    }

    parse_optional_stat_range(req, "shootMin", "shootMax", q.stats.use_min_shoot, q.stats.min_shoot,
                             q.stats.use_max_shoot, q.stats.max_shoot);
    parse_optional_stat_range(req, "tackleMin", "tackleMax", q.stats.use_min_tackle, q.stats.min_tackle,
                             q.stats.use_max_tackle, q.stats.max_tackle);
    parse_optional_stat_range(req, "speedMin", "speedMax", q.stats.use_min_speed, q.stats.min_speed,
                             q.stats.use_max_speed, q.stats.max_speed);
    parse_optional_stat_range(req, "accuracyMin", "accuracyMax", q.stats.use_min_accuracy, q.stats.min_accuracy,
                             q.stats.use_max_accuracy, q.stats.max_accuracy);
    parse_optional_stat_range(req, "awarenessMin", "awarenessMax", q.stats.use_min_awareness, q.stats.min_awareness,
                             q.stats.use_max_awareness, q.stats.max_awareness);

    return q;
}

bool parse_player_body(const crow::json::rvalue& body, PlayerRecord& out, std::string& err) {
    out = {};
    if (!body.has("schoolId") || !body.has("firstName") || !body.has("lastName") || !body.has("age") ||
        !body.has("preferredSpot")) {
        err = "schoolId, firstName, lastName, age, preferredSpot required";
        return false;
    }
    out.school_id = static_cast<std::uint64_t>(body["schoolId"].u());
    out.first_name = std::string(body["firstName"].s());
    out.last_name = std::string(body["lastName"].s());
    out.age = static_cast<int>(body["age"].i());
    bool ok_spot = false;
    out.preferred_spot = preferred_spot_from_string(std::string(body["preferredSpot"].s()), ok_spot);
    if (!ok_spot) {
        err = "invalid preferredSpot";
        return false;
    }
    if (body.has("potentialScore") && body["potentialScore"].t() != crow::json::type::Null) {
        out.potential_score = static_cast<int>(body["potentialScore"].i());
    } else {
        out.potential_score = -1;
    }
    const auto need_stat = [&](const char* key, int& dst) {
        if (!body.has(key)) {
            err = std::string("missing stat ") + key;
            return false;
        }
        dst = static_cast<int>(body[key].i());
        if (!ScoutingService::validate_stat_1_100(dst)) {
            err = std::string("stat out of range ") + key;
            return false;
        }
        return true;
    };
    if (!need_stat("shoot", out.shoot) || !need_stat("tackle", out.tackle) || !need_stat("speed", out.speed) ||
        !need_stat("accuracy", out.accuracy) || !need_stat("awareness", out.awareness)) {
        return false;
    }
    if (out.potential_score != -1 && !ScoutingService::validate_stat_1_100(out.potential_score)) {
        err = "potentialScore must be 1-100 or omit/null";
        return false;
    }
    return true;
}

crow::json::wvalue json_array_from_schools(const LinkedList<SchoolRecord>& list) {
    crow::json::wvalue items;
    unsigned idx = 0;
    list.for_each([&](const SchoolRecord& s) { items[idx++] = school_json(s); });
    return items;
}

crow::json::wvalue json_array_from_players(const LinkedList<PlayerRecord>& list) {
    crow::json::wvalue items;
    unsigned idx = 0;
    list.for_each([&](const PlayerRecord& p) { items[idx++] = player_json(p); });
    return items;
}

crow::json::wvalue json_array_from_reports(const LinkedList<ReportRecord>& list) {
    crow::json::wvalue items;
    unsigned idx = 0;
    list.for_each([&](const ReportRecord& r) { items[idx++] = report_json(r); });
    return items;
}

crow::json::wvalue json_array_from_team_players(const LinkedList<TeamPlayerRecord>& list) {
    crow::json::wvalue items;
    unsigned idx = 0;
    list.for_each([&](const TeamPlayerRecord& tp) { items[idx++] = team_player_json(tp); });
    return items;
}

bool auth_user_from_request(const crow::request& req, ScoutingService& svc, UserRecord& user_out,
                            crow::response& err_response) {
    std::string tok;
    if (!bearer_token(req, tok)) {
        err_response = json_error(401, "UNAUTHORIZED", "Bearer token required");
        return false;
    }
    if (!svc.user_from_token(tok, user_out)) {
        err_response = json_error(401, "UNAUTHORIZED", "invalid session");
        return false;
    }
    return true;
}

bool auth_user_can_write_scouting(const crow::request& req, ScoutingService& svc, UserRecord& user_out,
                                  crow::response& err_response) {
    if (!auth_user_from_request(req, svc, user_out, err_response)) {
        return false;
    }
    if (!ScoutingService::role_can_write_scouting(user_out.role)) {
        err_response = json_error(403, "FORBIDDEN", "scout or admin required");
        return false;
    }
    return true;
}

bool auth_user_is_admin(const crow::request& req, ScoutingService& svc, UserRecord& user_out,
                        crow::response& err_response) {
    if (!auth_user_from_request(req, svc, user_out, err_response)) {
        return false;
    }
    if (!ScoutingService::role_is_admin(user_out.role)) {
        err_response = json_error(403, "FORBIDDEN", "admin role required");
        return false;
    }
    return true;
}
