#pragma once

#include "model/domain.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

/** Domain data persisted under data/*.csv (schools, players, teams, team_players). */
class DomainCsvStore {
public:
    static constexpr std::size_t kMaxRecords = 256;

    DomainCsvStore();

    bool load_schools(std::array<SchoolRecord, kMaxRecords>& out, std::array<bool, kMaxRecords>& occ);
    bool load_players(std::array<PlayerRecord, kMaxRecords>& out, std::array<bool, kMaxRecords>& occ);
    bool load_teams(std::array<TeamRecord, kMaxRecords>& out, std::array<bool, kMaxRecords>& occ);
    bool load_team_players(std::array<TeamPlayerRecord, kMaxRecords>& out, std::array<bool, kMaxRecords>& occ);

    bool save_schools(const std::array<SchoolRecord, kMaxRecords>& data, const std::array<bool, kMaxRecords>& occ);
    bool save_players(const std::array<PlayerRecord, kMaxRecords>& data, const std::array<bool, kMaxRecords>& occ);
    bool save_teams(const std::array<TeamRecord, kMaxRecords>& data, const std::array<bool, kMaxRecords>& occ);
    bool save_team_players(const std::array<TeamPlayerRecord, kMaxRecords>& data,
                           const std::array<bool, kMaxRecords>& occ);

    std::string last_error() const { return last_error_; }

private:
    std::string schools_path_;
    std::string players_path_;
    std::string teams_path_;
    std::string team_players_path_;
    std::string last_error_;
};
