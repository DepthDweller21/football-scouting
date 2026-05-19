#include "controller/health_controller.h"

#include "service/db.h"

crow::response rootHandler() {
    return crow::response(200, "Hello from Crow!");
}

crow::response healthHandler() {
    crow::json::wvalue response;
    response["status"] = "ok";
    return crow::response(200, response);
}

crow::response dbHealthHandler() {
    const DbHealthResult dbResult = checkDatabaseHealth();

    crow::json::wvalue response;
    response["status"] = dbResult.ok ? "ok" : "error";
    response["message"] = dbResult.message;
    response["host"] = dbResult.host;
    response["database"] = dbResult.database;
    return crow::response(dbResult.ok ? 200 : 500, response);
}
