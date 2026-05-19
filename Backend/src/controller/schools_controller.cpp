#include "controller/schools_controller.h"

#include "controller/controller_helpers.h"
#include "view/json_view.h"
#include "service/scouting_service.h"

crow::response schools_list(ScoutingService& svc) {
    LinkedList<SchoolRecord> schools;
    svc.list_schools(schools);
    crow::json::wvalue data;
    data["schools"] = json_array_from_schools(schools);
    return json_success(data);
}

crow::response schools_get(ScoutingService& svc, std::uint64_t school_id) {
    SchoolRecord s{};
    if (!svc.get_school(school_id, s)) {
        return json_error(404, "NOT_FOUND", "school not found");
    }
    crow::json::wvalue data;
    data["school"] = school_json(s);
    return json_success(data);
}

crow::response schools_players(ScoutingService& svc, std::uint64_t school_id) {
    LinkedList<PlayerRecord> players;
    if (!svc.players_for_school(school_id, players)) {
        return json_error(404, "NOT_FOUND", "school not found");
    }
    crow::json::wvalue data;
    data["players"] = json_array_from_players(players);
    return json_success(data);
}

crow::response schools_create(ScoutingService& svc, const crow::request& req) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_can_write_scouting(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("name") || !body.has("location")) {
        return json_error(400, "VALIDATION_ERROR", "name and location required");
    }
    SchoolRecord s{};
    s.name = std::string(body["name"].s());
    s.location = std::string(body["location"].s());
    if (body.has("contactEmail")) {
        s.contact_email = std::string(body["contactEmail"].s());
    }
    std::uint64_t new_id = 0;
    std::string svc_err;
    if (!svc.create_school(s, new_id, svc_err)) {
        return json_error(400, "CREATE_FAILED", svc_err);
    }
    crow::json::wvalue data;
    data["id"] = new_id;
    return json_success(data, "created");
}

crow::response schools_update(ScoutingService& svc, const crow::request& req, std::uint64_t school_id) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_can_write_scouting(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("name") || !body.has("location")) {
        return json_error(400, "VALIDATION_ERROR", "name and location required");
    }
    SchoolRecord s{};
    s.id = school_id;
    s.name = std::string(body["name"].s());
    s.location = std::string(body["location"].s());
    if (body.has("contactEmail")) {
        s.contact_email = std::string(body["contactEmail"].s());
    }
    std::string svc_err;
    if (!svc.update_school(s, svc_err)) {
        return json_error(400, "UPDATE_FAILED", svc_err);
    }
    return json_success(school_json(s), "updated");
}

crow::response schools_remove(ScoutingService& svc, const crow::request& req, std::uint64_t school_id) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_is_admin(req, svc, u, err)) {
        return err;
    }
    std::string svc_err;
    if (!svc.delete_school(school_id, svc_err)) {
        return json_error(400, "DELETE_FAILED", svc_err);
    }
    crow::json::wvalue::object omap;
    return json_success(crow::json::wvalue(std::move(omap)), "deleted");
}

crow::response visits_create(ScoutingService& svc, const crow::request& req) {
    UserRecord u{};
    crow::response err;
    if (!auth_user_can_write_scouting(req, svc, u, err)) {
        return err;
    }
    const auto body = crow::json::load(req.body);
    if (!body || !body.has("schoolId") || !body.has("visitDate")) {
        return json_error(400, "VALIDATION_ERROR", "schoolId and visitDate required");
    }
    VisitRecord v{};
    v.scout_user_id = u.id;
    v.school_id = body["schoolId"].u();
    v.visit_date = std::string(body["visitDate"].s());
    if (body.has("notes")) {
        v.notes = std::string(body["notes"].s());
    }
    std::uint64_t new_id = 0;
    std::string svc_err;
    if (!svc.create_visit(v, new_id, svc_err)) {
        return json_error(400, "CREATE_FAILED", svc_err);
    }
    crow::json::wvalue data;
    data["visit"] = visit_json(v);
    data["visit"]["id"] = new_id;
    return json_success(data, "created");
}
