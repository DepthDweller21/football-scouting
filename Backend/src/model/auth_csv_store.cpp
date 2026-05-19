#include "model/auth_csv_store.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

constexpr std::time_t kSessionTtlSecs = 7 * 24 * 3600;

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

}  // namespace

namespace {

std::string resolve_auth_path(const char* filename) {
    const std::string parent = std::string("../data/") + filename;
    const std::string local = std::string("data/") + filename;
    if (std::filesystem::exists(parent)) {
        return parent;
    }
    return local;
}

}  // namespace

AuthCsvStore::AuthCsvStore() {
    users_path_ = resolve_auth_path("users.csv");
    sessions_path_ = resolve_auth_path("sessions.csv");

    std::error_code ec;
    const auto users_dir = std::filesystem::path(users_path_).parent_path();
    std::filesystem::create_directories(users_dir, ec);

    load_users();
    load_sessions();
}

bool AuthCsvStore::load_users() {
    last_error_.clear();
    users_.clear();

    std::ifstream in(users_path_);
    if (!in) {
        std::ofstream create(users_path_);
        create << "id,full_name,email,password,role\n";
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
        UserRecord u{};
        u.id = parse_u64(cols[0]);
        u.full_name = cols[1];
        u.email = cols[2];
        u.password = cols[3];
        u.role = cols[4];
        if (u.id > 0) {
            users_.push_back(std::move(u));
        }
    }
    return true;
}

bool AuthCsvStore::load_sessions() {
    last_error_.clear();
    sessions_.clear();

    std::ifstream in(sessions_path_);
    if (!in) {
        std::ofstream create(sessions_path_);
        create << "id,user_id,token,revoked,expires_at\n";
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
        SessionRow row{};
        row.id = parse_u64(cols[0]);
        row.user_id = parse_u64(cols[1]);
        row.token = cols[2];
        row.revoked = (cols[3] == "1" || cols[3] == "true");
        row.expires_at = static_cast<std::time_t>(parse_u64(cols[4]));
        if (row.id > 0) {
            sessions_.push_back(std::move(row));
        }
    }
    return true;
}

bool AuthCsvStore::save_users() {
    std::ofstream out(users_path_, std::ios::trunc);
    if (!out) {
        last_error_ = "cannot write users.csv";
        return false;
    }
    out << "id,full_name,email,password,role\n";
    for (const UserRecord& u : users_) {
        out << u.id << ',' << csv_escape(u.full_name) << ',' << csv_escape(u.email) << ','
            << csv_escape(u.password) << ',' << csv_escape(u.role) << '\n';
    }
    return true;
}

bool AuthCsvStore::save_sessions() {
    std::ofstream out(sessions_path_, std::ios::trunc);
    if (!out) {
        last_error_ = "cannot write sessions.csv";
        return false;
    }
    out << "id,user_id,token,revoked,expires_at\n";
    for (const SessionRow& s : sessions_) {
        out << s.id << ',' << s.user_id << ',' << csv_escape(s.token) << ','
            << (s.revoked ? 1 : 0) << ',' << static_cast<unsigned long long>(s.expires_at) << '\n';
    }
    return true;
}

std::uint64_t AuthCsvStore::next_user_id(const std::vector<UserRecord>& users) {
    std::uint64_t max_id = 0;
    for (const UserRecord& u : users) {
        max_id = std::max(max_id, u.id);
    }
    return max_id + 1;
}

std::uint64_t AuthCsvStore::next_session_id(const std::vector<SessionRow>& sessions) {
    std::uint64_t max_id = 0;
    for (const SessionRow& s : sessions) {
        max_id = std::max(max_id, s.id);
    }
    return max_id + 1;
}

bool AuthCsvStore::insert_user(const UserRecord& user, std::uint64_t& new_id) {
    last_error_.clear();
    for (const UserRecord& u : users_) {
        if (u.email == user.email) {
            last_error_ = "email already registered";
            return false;
        }
    }
    if (users_.size() >= kMaxRecords) {
        last_error_ = "capacity reached";
        return false;
    }
    UserRecord copy = user;
    copy.id = next_user_id(users_);
    users_.push_back(copy);
    new_id = copy.id;
    return save_users();
}

bool AuthCsvStore::find_user_by_email(const std::string& email, UserRecord& out) {
    last_error_.clear();
    for (const UserRecord& u : users_) {
        if (u.email == email) {
            out = u;
            return true;
        }
    }
    return false;
}

bool AuthCsvStore::find_user_by_id(std::uint64_t id, UserRecord& out) {
    last_error_.clear();
    for (const UserRecord& u : users_) {
        if (u.id == id) {
            out = u;
            return true;
        }
    }
    return false;
}

bool AuthCsvStore::insert_session(std::uint64_t user_id, const std::string& token, std::uint64_t& new_id) {
    last_error_.clear();
    UserRecord tmp{};
    if (!find_user_by_id(user_id, tmp)) {
        last_error_ = "user not found";
        return false;
    }

    const std::time_t now = std::time(nullptr);

    auto it = sessions_.end();
    for (auto i = sessions_.begin(); i != sessions_.end(); ++i) {
        if (i->revoked || now >= i->expires_at) {
            it = i;
            break;
        }
    }
    if (it == sessions_.end()) {
        if (sessions_.size() >= kMaxRecords) {
            last_error_ = "capacity reached";
            return false;
        }
        SessionRow row{};
        row.id = next_session_id(sessions_);
        row.user_id = user_id;
        row.token = token;
        row.revoked = false;
        row.expires_at = now + kSessionTtlSecs;
        sessions_.push_back(row);
        new_id = row.id;
        return save_sessions();
    }

    it->user_id = user_id;
    it->token = token;
    it->revoked = false;
    it->expires_at = now + kSessionTtlSecs;
    new_id = it->id;
    return save_sessions();
}

bool AuthCsvStore::revoke_session(const std::string& token) {
    last_error_.clear();
    for (SessionRow& s : sessions_) {
        if (s.token == token) {
            s.revoked = true;
            return save_sessions();
        }
    }
    return save_sessions();
}

bool AuthCsvStore::find_user_for_valid_token(const std::string& token, UserRecord& out) {
    last_error_.clear();
    const std::time_t now = std::time(nullptr);
    for (const SessionRow& s : sessions_) {
        if (s.revoked || s.token != token) {
            continue;
        }
        if (now >= s.expires_at) {
            continue;
        }
        return find_user_by_id(s.user_id, out);
    }
    return false;
}
