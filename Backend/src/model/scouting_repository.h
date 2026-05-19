#pragma once

#include "model/auth_csv_store.h"
#include "model/domain.h"
#include "model/domain_csv_store.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>

/** Fixed-capacity (256 slots per table) in-memory persistence — Model layer. */
class ScoutingRepository {
public:
    static constexpr std::size_t kMaxRecords = 256;

    ScoutingRepository();

    bool load_schools(std::array<SchoolRecord, kMaxRecords>& out, std::size_t& count);
    bool insert_school(const SchoolRecord& s, std::uint64_t& new_id);
    bool update_school(const SchoolRecord& s);
    bool delete_school(std::uint64_t id);

    bool load_players(std::array<PlayerRecord, kMaxRecords>& out, std::size_t& count);
    bool insert_player_with_stats(const PlayerRecord& p, std::uint64_t& new_id);

    bool load_visits(std::array<VisitRecord, kMaxRecords>& out, std::size_t& count);
    bool insert_visit(const VisitRecord& v, std::uint64_t& new_id);

    bool load_reports(std::array<ReportRecord, kMaxRecords>& out, std::size_t& count);
    bool insert_report(const ReportRecord& r, std::uint64_t& new_id);

    bool load_teams(std::array<TeamRecord, kMaxRecords>& out, std::size_t& count);
    bool load_team_players(std::array<TeamPlayerRecord, kMaxRecords>& out, std::size_t& count);
    bool load_team_players_for_team(std::uint64_t team_id, std::array<TeamPlayerRecord, kMaxRecords>& out,
                                    std::size_t& count);
    bool insert_team(const TeamRecord& t, std::uint64_t& new_id);
    bool update_team(const TeamRecord& t);
    bool delete_team(std::uint64_t id);
    bool insert_team_player(const TeamPlayerRecord& tp, std::uint64_t& new_row_id);
    bool remove_team_player(std::uint64_t team_id, std::uint64_t player_id);

    bool insert_user(const UserRecord& user, std::uint64_t& new_id);
    bool find_user_by_email(const std::string& email, UserRecord& out);
    bool find_user_by_id(std::uint64_t id, UserRecord& out);

    bool insert_session(std::uint64_t user_id, const std::string& token, std::uint64_t& new_id);
    bool revoke_session(const std::string& token);
    bool find_user_for_valid_token(const std::string& token, UserRecord& out);

    std::string last_error() const { return last_error_; }

private:
    std::string last_error_;
    AuthCsvStore auth_csv_;
    DomainCsvStore domain_csv_;

    std::array<bool, kMaxRecords> school_occ_{};
    std::array<SchoolRecord, kMaxRecords> schools_{};

    std::array<bool, kMaxRecords> player_occ_{};
    std::array<PlayerRecord, kMaxRecords> players_{};

    std::array<bool, kMaxRecords> visit_occ_{};
    std::array<VisitRecord, kMaxRecords> visits_{};

    std::array<bool, kMaxRecords> report_occ_{};
    std::array<ReportRecord, kMaxRecords> reports_{};

    std::array<bool, kMaxRecords> team_occ_{};
    std::array<TeamRecord, kMaxRecords> teams_{};

    std::array<bool, kMaxRecords> team_player_occ_{};
    std::array<TeamPlayerRecord, kMaxRecords> team_players_{};

    static bool slot_for_id(std::uint64_t id, std::size_t& idx);
    bool take_free_slot(std::array<bool, kMaxRecords>& occ, std::size_t& idx);
    void purge_team_players_for_team(std::uint64_t team_id);
};
