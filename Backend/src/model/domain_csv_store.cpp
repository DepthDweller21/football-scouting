#include "model/domain_csv_store.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

std::string csv_escape(const std::string& s) {
    if (s.find_first_of(",\"\n\r") == std::string::npos) {
        return s;
    }
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool in_quotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur += '"';
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                cur += c;
            }
        } else if (c == '"') {
            in_quotes = true;
        } else if (c == ',') {
            fields.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    fields.push_back(cur);
    return fields;
}

std::uint64_t parse_u64(const std::string& s) {
    if (s.empty()) {
        return 0;
    }
    return std::strtoull(s.c_str(), nullptr, 10);
}

int parse_int(const std::string& s) {
    if (s.empty()) {
        return 0;
    }
    return static_cast<int>(std::strtol(s.c_str(), nullptr, 10));
}

std::string resolve_data_path(const char* filename) {
    const std::string parent = std::string("../data/") + filename;
    const std::string local = std::string("data/") + filename;
    if (std::filesystem::exists(parent)) {
        return parent;
    }
    return local;
}

bool place_by_id(std::uint64_t id, std::size_t& idx) {
    if (id == 0 || id > DomainCsvStore::kMaxRecords) {
        return false;
    }
    idx = static_cast<std::size_t>(id - 1);
    return true;
}

}  // namespace

DomainCsvStore::DomainCsvStore() {
    schools_path_ = resolve_data_path("schools.csv");
    players_path_ = resolve_data_path("players.csv");
    teams_path_ = resolve_data_path("teams.csv");
    team_players_path_ = resolve_data_path("team_players.csv");

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(schools_path_).parent_path(), ec);
}

bool DomainCsvStore::load_schools(std::array<SchoolRecord, kMaxRecords>& out,
                                  std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    out.fill(SchoolRecord{});
    occ.fill(false);

    std::ifstream in(schools_path_);
    if (!in) {
        return true;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return true;
    }

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto cols = parse_csv_line(line);
        if (cols.size() < 3) {
            continue;
        }
        std::size_t idx = 0;
        const std::uint64_t id = parse_u64(cols[0]);
        if (!place_by_id(id, idx)) {
            continue;
        }
        SchoolRecord s{};
        s.id = id;
        s.name = cols[1];
        s.location = cols[2];
        if (cols.size() > 3) {
            s.contact_email = cols[3];
        }
        out[idx] = s;
        occ[idx] = true;
    }
    return true;
}

bool DomainCsvStore::load_players(std::array<PlayerRecord, kMaxRecords>& out,
                                  std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    out.fill(PlayerRecord{});
    occ.fill(false);

    std::ifstream in(players_path_);
    if (!in) {
        return true;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return true;
    }

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto cols = parse_csv_line(line);
        if (cols.size() < 12) {
            continue;
        }
        std::size_t idx = 0;
        const std::uint64_t id = parse_u64(cols[0]);
        if (!place_by_id(id, idx)) {
            continue;
        }
        bool ok_spot = false;
        const PreferredSpot spot = preferred_spot_from_string(cols[5], ok_spot);
        if (!ok_spot) {
            continue;
        }

        PlayerRecord p{};
        p.id = id;
        p.school_id = parse_u64(cols[1]);
        p.first_name = cols[2];
        p.last_name = cols[3];
        p.age = parse_int(cols[4]);
        p.preferred_spot = spot;
        if (!cols[6].empty()) {
            p.potential_score = parse_int(cols[6]);
        } else {
            p.potential_score = -1;
        }
        p.shoot = parse_int(cols[7]);
        p.tackle = parse_int(cols[8]);
        p.speed = parse_int(cols[9]);
        p.accuracy = parse_int(cols[10]);
        p.awareness = parse_int(cols[11]);

        out[idx] = p;
        occ[idx] = true;
    }
    return true;
}

bool DomainCsvStore::save_schools(const std::array<SchoolRecord, kMaxRecords>& data,
                                  const std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    std::ofstream out(schools_path_, std::ios::trunc);
    if (!out) {
        last_error_ = "cannot write schools.csv";
        return false;
    }
    out << "id,name,location,contact_email\n";
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (!occ[i]) {
            continue;
        }
        const SchoolRecord& s = data[i];
        out << s.id << ',' << csv_escape(s.name) << ',' << csv_escape(s.location) << ','
            << csv_escape(s.contact_email) << '\n';
    }
    return true;
}

bool DomainCsvStore::save_players(const std::array<PlayerRecord, kMaxRecords>& data,
                                  const std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    std::ofstream out(players_path_, std::ios::trunc);
    if (!out) {
        last_error_ = "cannot write players.csv";
        return false;
    }
    out << "id,school_id,first_name,last_name,age,preferred_spot,potential_score,shoot,tackle,speed,accuracy,"
           "awareness\n";
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (!occ[i]) {
            continue;
        }
        const PlayerRecord& p = data[i];
        out << p.id << ',' << p.school_id << ',' << csv_escape(p.first_name) << ',' << csv_escape(p.last_name)
            << ',' << p.age << ',' << preferred_spot_to_string(p.preferred_spot) << ',';
        if (p.potential_score >= 1 && p.potential_score <= 100) {
            out << p.potential_score;
        }
        out << ',' << p.shoot << ',' << p.tackle << ',' << p.speed << ',' << p.accuracy << ',' << p.awareness
            << '\n';
    }
    return true;
}

bool DomainCsvStore::load_teams(std::array<TeamRecord, kMaxRecords>& out, std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    out.fill(TeamRecord{});
    occ.fill(false);

    std::ifstream in(teams_path_);
    if (!in) {
        return true;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return true;
    }

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto cols = parse_csv_line(line);
        if (cols.size() < 4) {
            continue;
        }
        std::size_t idx = 0;
        const std::uint64_t id = parse_u64(cols[0]);
        if (!place_by_id(id, idx)) {
            continue;
        }
        TeamRecord t{};
        t.id = id;
        t.owner_user_id = parse_u64(cols[1]);
        t.name = cols[2];
        t.formation = cols[3];
        out[idx] = t;
        occ[idx] = true;
    }
    return true;
}

bool DomainCsvStore::load_team_players(std::array<TeamPlayerRecord, kMaxRecords>& out,
                                       std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    out.fill(TeamPlayerRecord{});
    occ.fill(false);

    std::ifstream in(team_players_path_);
    if (!in) {
        return true;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return true;
    }

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto cols = parse_csv_line(line);
        if (cols.size() < 5) {
            continue;
        }
        std::size_t idx = 0;
        const std::uint64_t id = parse_u64(cols[0]);
        if (!place_by_id(id, idx)) {
            continue;
        }
        bool ok_spot = false;
        const PreferredSpot spot = preferred_spot_from_string(cols[3], ok_spot);
        if (!ok_spot) {
            continue;
        }
        TeamPlayerRecord tp{};
        tp.id = id;
        tp.team_id = parse_u64(cols[1]);
        tp.player_id = parse_u64(cols[2]);
        tp.assigned_spot = spot;
        tp.order_index = parse_int(cols[4]);
        out[idx] = tp;
        occ[idx] = true;
    }
    return true;
}

bool DomainCsvStore::save_teams(const std::array<TeamRecord, kMaxRecords>& data,
                                const std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    std::ofstream out(teams_path_, std::ios::trunc);
    if (!out) {
        last_error_ = "cannot write teams.csv";
        return false;
    }
    out << "id,owner_user_id,name,formation\n";
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (!occ[i]) {
            continue;
        }
        const TeamRecord& t = data[i];
        out << t.id << ',' << t.owner_user_id << ',' << csv_escape(t.name) << ',' << csv_escape(t.formation)
            << '\n';
    }
    return true;
}

bool DomainCsvStore::save_team_players(const std::array<TeamPlayerRecord, kMaxRecords>& data,
                                       const std::array<bool, kMaxRecords>& occ) {
    last_error_.clear();
    std::ofstream out(team_players_path_, std::ios::trunc);
    if (!out) {
        last_error_ = "cannot write team_players.csv";
        return false;
    }
    out << "id,team_id,player_id,assigned_spot,order_index\n";
    for (std::size_t i = 0; i < kMaxRecords; ++i) {
        if (!occ[i]) {
            continue;
        }
        const TeamPlayerRecord& tp = data[i];
        out << tp.id << ',' << tp.team_id << ',' << tp.player_id << ','
            << preferred_spot_to_string(tp.assigned_spot) << ',' << tp.order_index << '\n';
    }
    return true;
}
