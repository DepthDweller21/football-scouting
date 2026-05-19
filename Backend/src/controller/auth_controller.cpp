#include "controller/auth_controller.h"

#include "model/domain.h"
#include "service/scouting_service.h"
#include "view/json_view.h"

#include <string>

crow::response auth_register(ScoutingService& svc, const crow::request& req) {
    const auto body = crow::json::load(req.body);
    if (!body) {
        return json_error(400, "BAD_JSON", "invalid JSON body");
    }
    const std::string full_name = body.has("fullName") ? std::string(body["fullName"].s()) : "";
    const std::string email = body.has("email") ? std::string(body["email"].s()) : "";
    const std::string password = body.has("password") ? std::string(body["password"].s()) : "";
    const std::string role = body.has("role") ? std::string(body["role"].s()) : "viewer";

    const auto out = svc.register_user(full_name, email, password, role);
    if (!out.ok) {
        const int code = out.error_code == "REGISTER_FAILED" ? 409 : 400;
        return json_error(code, out.error_code, out.message);
    }
    crow::json::wvalue data;
    data["user"] = user_public_json(out.user);
    return json_success(data, out.message);
}

crow::response auth_login(ScoutingService& svc, const crow::request& req) {
    const auto body = crow::json::load(req.body);
    if (!body) {
        return json_error(400, "BAD_JSON", "invalid JSON body");
    }
    const std::string email = body.has("email") ? std::string(body["email"].s()) : "";
    const std::string password = body.has("password") ? std::string(body["password"].s()) : "";
    const auto out = svc.login(email, password);
    if (!out.ok) {
        const int code = out.error_code == "INVALID_CREDENTIALS" ? 401 : 400;
        return json_error(code, out.error_code, out.message);
    }
    crow::json::wvalue data;
    data["token"] = out.token_plain;
    data["user"] = user_public_json(out.user);
    return json_success(data, out.message);
}

crow::response auth_logout(ScoutingService& svc, const crow::request& req) {
    std::string tok;
    if (!bearer_token(req, tok)) {
        return json_error(401, "UNAUTHORIZED", "Bearer token required");
    }
    if (!svc.logout(tok)) {
        return json_error(400, "LOGOUT_FAILED", "could not revoke session");
    }
    crow::json::wvalue::object omap;
    return json_success(crow::json::wvalue(std::move(omap)), "logged out");
}

crow::response auth_me(ScoutingService& svc, const crow::request& req) {
    std::string tok;
    if (!bearer_token(req, tok)) {
        return json_error(401, "UNAUTHORIZED", "Bearer token required");
    }
    UserRecord user;
    if (!svc.user_from_token(tok, user)) {
        return json_error(401, "UNAUTHORIZED", "invalid or expired session");
    }
    crow::json::wvalue data;
    data["user"] = user_public_json(user);
    return json_success(data);
}
