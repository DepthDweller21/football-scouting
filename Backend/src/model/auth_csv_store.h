#pragma once

#include "model/domain.h"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

/** Sign-up / login persistence in data/users.csv and data/sessions.csv. */
class AuthCsvStore {
public:
    static constexpr std::size_t kMaxRecords = 256;

    AuthCsvStore();

    bool insert_user(const UserRecord& user, std::uint64_t& new_id);
    bool find_user_by_email(const std::string& email, UserRecord& out);
    bool find_user_by_id(std::uint64_t id, UserRecord& out);

    bool insert_session(std::uint64_t user_id, const std::string& token, std::uint64_t& new_id);
    bool revoke_session(const std::string& token);
    bool find_user_for_valid_token(const std::string& token, UserRecord& out);

    std::string last_error() const { return last_error_; }

private:
    struct SessionRow {
        std::uint64_t id{0};
        std::uint64_t user_id{0};
        std::string token;
        bool revoked{false};
        std::time_t expires_at{0};
    };

    std::string last_error_;
    std::string users_path_;
    std::string sessions_path_;

    std::vector<UserRecord> users_;
    std::vector<SessionRow> sessions_;

    bool load_users();
    bool load_sessions();
    bool save_users();
    bool save_sessions();

    static std::uint64_t next_user_id(const std::vector<UserRecord>& users);
    static std::uint64_t next_session_id(const std::vector<SessionRow>& sessions);
};
