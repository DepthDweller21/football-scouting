#include "view/json_view.h"

#include <cstdlib>
#include <utility>

crow::response json_success(crow::json::wvalue data, const std::string& message) {
    crow::json::wvalue envelope;
    envelope["success"] = true;
    envelope["data"] = std::move(data);
    envelope["message"] = message;
    crow::response r(200, envelope);
    r.add_header("Content-Type", "application/json");
    return r;
}

crow::response json_error(int http_status, const std::string& code, const std::string& message) {
    crow::json::wvalue err;
    err["code"] = code;
    err["message"] = message;
    crow::json::wvalue envelope;
    envelope["success"] = false;
    envelope["error"] = std::move(err);
    crow::response r(http_status, envelope);
    r.add_header("Content-Type", "application/json");
    return r;
}

crow::json::wvalue user_public_json(const UserRecord& u) {
    crow::json::wvalue w;
    w["id"] = u.id;
    w["fullName"] = u.full_name;
    w["email"] = u.email;
    w["role"] = u.role;
    return w;
}

crow::json::wvalue school_json(const SchoolRecord& s) {
    crow::json::wvalue w;
    w["id"] = s.id;
    w["name"] = s.name;
    w["location"] = s.location;
    w["contactEmail"] = s.contact_email;
    return w;
}

crow::json::wvalue player_json(const PlayerRecord& p) {
    crow::json::wvalue w;
    w["id"] = p.id;
    w["schoolId"] = p.school_id;
    w["firstName"] = p.first_name;
    w["lastName"] = p.last_name;
    w["age"] = p.age;
    w["preferredSpot"] = preferred_spot_to_string(p.preferred_spot);
    if (p.potential_score >= 1 && p.potential_score <= 100) {
        w["potentialScore"] = p.potential_score;
    } else {
        w["potentialScore"] = crow::json::wvalue(nullptr);
    }
    w["shoot"] = p.shoot;
    w["tackle"] = p.tackle;
    w["speed"] = p.speed;
    w["accuracy"] = p.accuracy;
    w["awareness"] = p.awareness;
    return w;
}

crow::json::wvalue visit_json(const VisitRecord& v) {
    crow::json::wvalue w;
    w["id"] = v.id;
    w["scoutUserId"] = v.scout_user_id;
    w["schoolId"] = v.school_id;
    w["visitDate"] = v.visit_date;
    w["notes"] = v.notes;
    return w;
}

crow::json::wvalue report_json(const ReportRecord& r) {
    crow::json::wvalue w;
    w["id"] = r.id;
    w["playerId"] = r.player_id;
    w["scoutUserId"] = r.scout_user_id;
    if (r.visit_id != 0) {
        w["visitId"] = r.visit_id;
    } else {
        w["visitId"] = crow::json::wvalue(nullptr);
    }
    w["recommendation"] = r.recommendation;
    w["strengths"] = r.strengths;
    w["weaknesses"] = r.weaknesses;
    w["createdAt"] = r.created_at;
    return w;
}

crow::json::wvalue team_json(const TeamRecord& t) {
    crow::json::wvalue w;
    w["id"] = t.id;
    w["ownerUserId"] = t.owner_user_id;
    w["name"] = t.name;
    w["formation"] = t.formation;
    return w;
}

crow::json::wvalue team_player_json(const TeamPlayerRecord& tp) {
    crow::json::wvalue w;
    w["id"] = tp.id;
    w["teamId"] = tp.team_id;
    w["playerId"] = tp.player_id;
    w["assignedSpot"] = preferred_spot_to_string(tp.assigned_spot);
    w["orderIndex"] = tp.order_index;
    return w;
}

bool bearer_token(const crow::request& req, std::string& token_out) {
    token_out.clear();
    const std::string auth = req.get_header_value("Authorization");
    if (auth.empty()) {
        return false;
    }
    constexpr char prefix[] = "Bearer ";
    if (auth.rfind(prefix, 0) != 0) {
        return false;
    }
    std::string tok = auth.substr(sizeof(prefix) - 1);
    while (!tok.empty() && (tok.front() == ' ' || tok.front() == '\t')) {
        tok.erase(tok.begin());
    }
    if (tok.empty()) {
        return false;
    }
    token_out = tok;
    return true;
}

bool parse_optional_u64(const crow::request& req, const char* param, std::uint64_t& out) {
    const auto* v = req.url_params.get(param);
    if (!v) {
        return false;
    }
    char* end = nullptr;
    unsigned long long x = std::strtoull(v, &end, 10);
    if (end == v) {
        return false;
    }
    out = static_cast<std::uint64_t>(x);
    return true;
}

bool parse_optional_int(const crow::request& req, const char* param, int& out) {
    const auto* v = req.url_params.get(param);
    if (!v) {
        return false;
    }
    char* end = nullptr;
    long x = std::strtol(v, &end, 10);
    if (end == v) {
        return false;
    }
    out = static_cast<int>(x);
    return true;
}

bool parse_optional_stat_range(const crow::request& req, const char* min_key, const char* max_key, bool& use_min, int& min_v, bool& use_max, int& max_v) {
    bool ok = false;
    int tmp = 0;
    if (parse_optional_int(req, min_key, tmp)) {
        use_min = true;
        min_v = tmp;
        ok = true;
    }
    if (parse_optional_int(req, max_key, tmp)) {
        use_max = true;
        max_v = tmp;
        ok = true;
    }
    return ok;
}
