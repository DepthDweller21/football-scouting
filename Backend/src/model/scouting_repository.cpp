#include "model/scouting_repository.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>

namespace {

std::string format_timestamp_now() {
    const std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

}  // namespace

bool ScoutingRepository::slot_for_id(std::uint64_t id, std::size_t& idx) {
    if (id == 0 || id > kMaxRecords) {
        return false;
    }
    idx = static_cast<std::size_t>(id - 1);
    return true;
}

bool ScoutingRepository::take_free_slot(std::array<bool, kMaxRecords>& occ, std::size_t& idx) {
    last_error_.clear();
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (!occ[i]) {
            idx = i;
            return true;
        }
    }
    last_error_ = "capacity reached";
    return false;
}

void ScoutingRepository::purge_team_players_for_team(std::uint64_t team_id) {
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (team_player_occ_[i] && team_players_[i].team_id == team_id) {
            team_player_occ_[i] = false;
        }
    }
}

ScoutingRepository::ScoutingRepository() {
    if (!domain_csv_.load_schools(schools_, school_occ_)) {
        last_error_ = domain_csv_.last_error();
    }
    if (!domain_csv_.load_players(players_, player_occ_)) {
        last_error_ = domain_csv_.last_error();
    }
    if (!domain_csv_.load_teams(teams_, team_occ_)) {
        last_error_ = domain_csv_.last_error();
    }
    if (!domain_csv_.load_team_players(team_players_, team_player_occ_)) {
        last_error_ = domain_csv_.last_error();
    }
}

bool ScoutingRepository::load_schools(std::array<SchoolRecord, kMaxRecords>& out, std::size_t& count) {
    last_error_.clear();
    count = 0;
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (school_occ_[i]) {
            out[count++] = schools_[i];
        }
    }
    return true;
}

bool ScoutingRepository::insert_school(const SchoolRecord& s, std::uint64_t& new_id) {
    std::size_t idx = 0;
    if (!take_free_slot(school_occ_, idx)) {
        return false;
    }
    schools_[idx] = s;
    schools_[idx].id = static_cast<std::uint64_t>(idx + 1);
    school_occ_[idx] = true;
    new_id = schools_[idx].id;
    if (!domain_csv_.save_schools(schools_, school_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::update_school(const SchoolRecord& s) {
    last_error_.clear();
    std::size_t idx = 0;
    if (!slot_for_id(s.id, idx) || !school_occ_[idx]) {
        last_error_ = "school not found";
        return false;
    }
    schools_[idx] = s;
    schools_[idx].id = s.id;
    if (!domain_csv_.save_schools(schools_, school_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::delete_school(std::uint64_t id) {
    last_error_.clear();
    std::size_t idx = 0;
    if (!slot_for_id(id, idx) || !school_occ_[idx]) {
        last_error_ = "school not found";
        return false;
    }
    school_occ_[idx] = false;
    if (!domain_csv_.save_schools(schools_, school_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::load_players(std::array<PlayerRecord, kMaxRecords>& out, std::size_t& count) {
    last_error_.clear();
    count = 0;
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (player_occ_[i]) {
            out[count++] = players_[i];
        }
    }
    return true;
}

bool ScoutingRepository::insert_player_with_stats(const PlayerRecord& p, std::uint64_t& new_id) {
    std::size_t idx = 0;
    if (!take_free_slot(player_occ_, idx)) {
        return false;
    }
    players_[idx] = p;
    players_[idx].id = static_cast<std::uint64_t>(idx + 1);
    player_occ_[idx] = true;
    new_id = players_[idx].id;
    if (!domain_csv_.save_players(players_, player_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::load_visits(std::array<VisitRecord, kMaxRecords>& out, std::size_t& count) {
    last_error_.clear();
    count = 0;
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (visit_occ_[i]) {
            out[count++] = visits_[i];
        }
    }
    return true;
}

bool ScoutingRepository::insert_visit(const VisitRecord& v, std::uint64_t& new_id) {
    std::size_t idx = 0;
    if (!take_free_slot(visit_occ_, idx)) {
        return false;
    }
    visits_[idx] = v;
    visits_[idx].id = static_cast<std::uint64_t>(idx + 1);
    visit_occ_[idx] = true;
    new_id = visits_[idx].id;
    return true;
}

bool ScoutingRepository::load_reports(std::array<ReportRecord, kMaxRecords>& out, std::size_t& count) {
    last_error_.clear();
    count = 0;
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (report_occ_[i]) {
            out[count++] = reports_[i];
        }
    }
    return true;
}

bool ScoutingRepository::insert_report(const ReportRecord& r, std::uint64_t& new_id) {
    std::size_t idx = 0;
    if (!take_free_slot(report_occ_, idx)) {
        return false;
    }
    reports_[idx] = r;
    reports_[idx].id = static_cast<std::uint64_t>(idx + 1);
    if (reports_[idx].created_at.empty()) {
        reports_[idx].created_at = format_timestamp_now();
    }
    report_occ_[idx] = true;
    new_id = reports_[idx].id;
    return true;
}

bool ScoutingRepository::load_teams(std::array<TeamRecord, kMaxRecords>& out, std::size_t& count) {
    last_error_.clear();
    count = 0;
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (team_occ_[i]) {
            out[count++] = teams_[i];
        }
    }
    return true;
}

bool ScoutingRepository::load_team_players_for_team(std::uint64_t team_id,
                                                    std::array<TeamPlayerRecord, kMaxRecords>& out,
                                                    std::size_t& count) {
    last_error_.clear();
    count = 0;
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (team_player_occ_[i] && team_players_[i].team_id == team_id) {
            out[count++] = team_players_[i];
        }
    }
    std::sort(out.begin(), out.begin() + count, [](const TeamPlayerRecord& a, const TeamPlayerRecord& b) {
        if (a.order_index != b.order_index) {
            return a.order_index < b.order_index;
        }
        return a.id < b.id;
    });
    return true;
}

bool ScoutingRepository::load_team_players(std::array<TeamPlayerRecord, kMaxRecords>& out, std::size_t& count) {
    last_error_.clear();
    count = 0;
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (team_player_occ_[i]) {
            out[count++] = team_players_[i];
        }
    }
    std::sort(out.begin(), out.begin() + count, [](const TeamPlayerRecord& a, const TeamPlayerRecord& b) {
        if (a.team_id != b.team_id) {
            return a.team_id < b.team_id;
        }
        if (a.order_index != b.order_index) {
            return a.order_index < b.order_index;
        }
        return a.id < b.id;
    });
    return true;
}

bool ScoutingRepository::insert_team(const TeamRecord& t, std::uint64_t& new_id) {
    std::size_t idx = 0;
    if (!take_free_slot(team_occ_, idx)) {
        return false;
    }
    teams_[idx] = t;
    teams_[idx].id = static_cast<std::uint64_t>(idx + 1);
    team_occ_[idx] = true;
    new_id = teams_[idx].id;
    if (!domain_csv_.save_teams(teams_, team_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::update_team(const TeamRecord& t) {
    last_error_.clear();
    std::size_t idx = 0;
    if (!slot_for_id(t.id, idx) || !team_occ_[idx]) {
        last_error_ = "team not found";
        return false;
    }
    teams_[idx] = t;
    teams_[idx].id = t.id;
    if (!domain_csv_.save_teams(teams_, team_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::delete_team(std::uint64_t id) {
    last_error_.clear();
    std::size_t idx = 0;
    if (!slot_for_id(id, idx) || !team_occ_[idx]) {
        last_error_ = "team not found";
        return false;
    }
    purge_team_players_for_team(id);
    team_occ_[idx] = false;
    if (!domain_csv_.save_teams(teams_, team_occ_) || !domain_csv_.save_team_players(team_players_, team_player_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::insert_team_player(const TeamPlayerRecord& tp, std::uint64_t& new_row_id) {
    std::size_t idx = 0;
    if (!take_free_slot(team_player_occ_, idx)) {
        return false;
    }
    team_players_[idx] = tp;
    team_players_[idx].id = static_cast<std::uint64_t>(idx + 1);
    team_player_occ_[idx] = true;
    new_row_id = team_players_[idx].id;
    if (!domain_csv_.save_team_players(team_players_, team_player_occ_)) {
        last_error_ = domain_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::remove_team_player(std::uint64_t team_id, std::uint64_t player_id) {
    last_error_.clear();
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (team_player_occ_[i] && team_players_[i].team_id == team_id && team_players_[i].player_id == player_id) {
            team_player_occ_[i] = false;
            if (!domain_csv_.save_team_players(team_players_, team_player_occ_)) {
                last_error_ = domain_csv_.last_error();
                return false;
            }
            return true;
        }
    }
    return true;
}

bool ScoutingRepository::insert_user(const UserRecord& user, std::uint64_t& new_id) {
    if (!auth_csv_.insert_user(user, new_id)) {
        last_error_ = auth_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::find_user_by_email(const std::string& email, UserRecord& out) {
    last_error_.clear();
    return auth_csv_.find_user_by_email(email, out);
}

bool ScoutingRepository::find_user_by_id(std::uint64_t id, UserRecord& out) {
    last_error_.clear();
    return auth_csv_.find_user_by_id(id, out);
}

bool ScoutingRepository::insert_session(std::uint64_t user_id, const std::string& token, std::uint64_t& new_id) {
    if (!auth_csv_.insert_session(user_id, token, new_id)) {
        last_error_ = auth_csv_.last_error();
        return false;
    }
    return true;
}

bool ScoutingRepository::revoke_session(const std::string& token) {
    last_error_.clear();
    return auth_csv_.revoke_session(token);
}

bool ScoutingRepository::find_user_for_valid_token(const std::string& token, UserRecord& out) {
    last_error_.clear();
    return auth_csv_.find_user_for_valid_token(token, out);
}
