#pragma once

#include <cstdint>
#include <string>

enum class PreferredSpot : int {
    GK = 0,
    RB,
    CB,
    LB,
    CDM,
    CM,
    CAM,
    RW,
    LW,
    ST,
};

PreferredSpot preferred_spot_from_string(const std::string& s, bool& ok);
std::string preferred_spot_to_string(PreferredSpot p);

struct UserRecord {
    std::uint64_t id{0};
    std::string full_name;
    std::string email;
    std::string password;  // stored plaintext per project scope (not for production)
    std::string role;      // admin | scout | viewer
};

struct SessionRecord {
    std::uint64_t id{0};
    std::uint64_t user_id{0};
    std::string token;
    std::string expires_at;
};

struct SchoolRecord {
    std::uint64_t id{0};
    std::string name;
    std::string location;
    std::string contact_email;
};

struct PlayerRecord {
    std::uint64_t id{0};
    std::uint64_t school_id{0};
    std::string first_name;
    std::string last_name;
    int age{0};
    PreferredSpot preferred_spot{PreferredSpot::CM};
    int potential_score{-1};  // -1 = NULL
    int shoot{0};
    int tackle{0};
    int speed{0};
    int accuracy{0};
    int awareness{0};
};

struct VisitRecord {
    std::uint64_t id{0};
    std::uint64_t scout_user_id{0};
    std::uint64_t school_id{0};
    std::string visit_date;
    std::string notes;
};

struct ReportRecord {
    std::uint64_t id{0};
    std::uint64_t player_id{0};
    std::uint64_t scout_user_id{0};
    std::uint64_t visit_id{0};  // 0 = none
    std::string recommendation;
    std::string strengths;
    std::string weaknesses;
    std::string created_at;
};

struct TeamRecord {
    std::uint64_t id{0};
    std::uint64_t owner_user_id{0};
    std::string name;
    std::string formation;
};

struct TeamPlayerRecord {
    std::uint64_t id{0};
    std::uint64_t team_id{0};
    std::uint64_t player_id{0};
    PreferredSpot assigned_spot{PreferredSpot::CM};
    int order_index{0};
};
