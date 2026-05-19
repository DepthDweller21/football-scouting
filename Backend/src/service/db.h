#pragma once

#include <string>

struct DbHealthResult {
    bool ok;
    std::string message;
    std::string host;
    std::string database;
};

inline DbHealthResult checkDatabaseHealth() {
    return {true,
            std::string("schools/players/teams in data/*.csv; visits in-memory; auth in data/users.csv + "
                        "data/sessions.csv"),
            "",
            ""};
}
