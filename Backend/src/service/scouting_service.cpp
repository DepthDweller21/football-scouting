#include "service/scouting_service.h"

#include "model/formation_graph.h"
#include "model/formation_templates.h"

#include <algorithm>
#include <unordered_set>
#include <vector>
#include <array>
#include <cctype>
#include <ctime>
#include <string>

namespace {

std::string random_session_token() {
    std::string t;
    for (int i = 0; i < 32; ++i) {
        t += static_cast<char>('a' + (rand() % 26));
    }
    return t;
}

std::string normalize_role(std::string r) {
    for (char& c : r) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (r == "admin" || r == "scout" || r == "viewer") {
        return r;
    }
    return "viewer";
}

bool team_manage_allowed(const TeamRecord& t, std::uint64_t uid, const std::string& role) {
    return ScoutingService::role_is_admin(role) || t.owner_user_id == uid;
}

}  // namespace

ScoutingService::ScoutingService(ScoutingRepository& repo) : repo_(repo) {}

void ScoutingService::release_dynamic_lists() {
    reports_by_player_.for_each([](std::uint64_t, LinkedList<ReportRecord>* lst) {
        delete lst;
    });
    reports_by_player_.clear();
    team_rosters_.for_each([](std::uint64_t, LinkedList<TeamPlayerRecord>* lst) {
        delete lst;
    });
    team_rosters_.clear();
}

bool ScoutingService::hydrate() {
    release_dynamic_lists();

    player_store_.fill(PlayerRecord{});
    player_count_ = 0;
    player_id_to_index_.clear();
    schools_by_id_.clear();
    visit_log_.clear();
    avl_top_.clear();
    avl_potential_.clear();
    player_rank_keys_.clear();
    teams_by_id_.clear();

    std::array<SchoolRecord, ScoutingRepository::kMaxRecords> schools{};
    std::size_t n_schools = 0;
    std::array<PlayerRecord, ScoutingRepository::kMaxRecords> players{};
    std::size_t n_players = 0;
    std::array<VisitRecord, ScoutingRepository::kMaxRecords> visits{};
    std::size_t n_visits = 0;
    std::array<ReportRecord, ScoutingRepository::kMaxRecords> reports{};
    std::size_t n_reports = 0;
    std::array<TeamRecord, ScoutingRepository::kMaxRecords> teams{};
    std::size_t n_teams = 0;
    std::array<TeamPlayerRecord, ScoutingRepository::kMaxRecords> team_players{};
    std::size_t n_team_players = 0;

    if (!repo_.load_schools(schools, n_schools)) {
        return false;
    }
    if (!repo_.load_players(players, n_players)) {
        return false;
    }
    if (!repo_.load_visits(visits, n_visits)) {
        return false;
    }
    if (!repo_.load_reports(reports, n_reports)) {
        return false;
    }
    if (!repo_.load_teams(teams, n_teams)) {
        return false;
    }
    if (!repo_.load_team_players(team_players, n_team_players)) {
        return false;
    }

    for (std::size_t i = 0; i < n_schools; ++i) {
        schools_by_id_.insert_or_assign(schools[i].id, std::move(schools[i]));
    }

    for (std::size_t i = 0; i < n_players; ++i) {
        if (player_count_ >= ScoutingRepository::kMaxRecords) {
            return false;
        }
        const std::size_t idx = player_count_;
        player_store_[idx] = std::move(players[i]);
        const PlayerRecord& pr = player_store_[idx];
        player_id_to_index_.insert_or_assign(pr.id, idx);

        RankKeys rk{};
        rk.top_key = top_sort_key(pr);
        avl_top_.insert(rk.top_key, pr.id);
        if (pr.potential_score >= 1 && pr.potential_score <= 100) {
            rk.pot_key = pot_sort_key(pr.id, pr.potential_score);
            rk.in_potential_avl = true;
            avl_potential_.insert(rk.pot_key, pr.id);
        }
        player_rank_keys_.insert_or_assign(pr.id, rk);
        ++player_count_;
    }

    for (std::size_t i = 0; i < n_visits; ++i) {
        visit_log_.push_back(std::move(visits[i]));
    }

    for (std::size_t i = 0; i < n_reports; ++i) {
        ReportRecord r = std::move(reports[i]);
        auto** slot = reports_by_player_.find_mut(r.player_id);
        if (!slot) {
            reports_by_player_.insert_or_assign(r.player_id, new LinkedList<ReportRecord>());
            slot = reports_by_player_.find_mut(r.player_id);
        }
        if (slot && *slot) {
            (*slot)->push_back(std::move(r));
        }
    }

    for (std::size_t i = 0; i < n_teams; ++i) {
        teams_by_id_.insert_or_assign(teams[i].id, std::move(teams[i]));
    }

    for (std::size_t i = 0; i < n_team_players; ++i) {
        TeamPlayerRecord tp = std::move(team_players[i]);
        auto** slot = team_rosters_.find_mut(tp.team_id);
        if (!slot) {
            team_rosters_.insert_or_assign(tp.team_id, new LinkedList<TeamPlayerRecord>());
            slot = team_rosters_.find_mut(tp.team_id);
        }
        if (slot && *slot) {
            (*slot)->push_back(std::move(tp));
        }
    }

    return true;
}

bool ScoutingService::validate_stat_1_100(int v) {
    return v >= 1 && v <= 100;
}

bool ScoutingService::role_can_write_scouting(const std::string& role) {
    return role == "admin" || role == "scout";
}

bool ScoutingService::role_is_admin(const std::string& role) {
    return role == "admin";
}

std::int64_t ScoutingService::composite_total(const PlayerRecord& p) {
    return static_cast<std::int64_t>(p.shoot) + p.tackle + p.speed + p.accuracy + p.awareness;
}

std::int64_t ScoutingService::top_sort_key(const PlayerRecord& p) {
    const std::int64_t c = composite_total(p);
    constexpr std::int64_t kMul = 1000000000000LL;
    return (500LL - c) * kMul + static_cast<std::int64_t>(p.id);
}

std::int64_t ScoutingService::pot_sort_key(std::uint64_t player_id, int potential_1_100) {
    constexpr std::int64_t kMul = 1000000000000LL;
    return (101LL - static_cast<std::int64_t>(potential_1_100)) * kMul + static_cast<std::int64_t>(player_id);
}

bool ScoutingService::fetch_player(std::uint64_t id, PlayerRecord& out) {
    std::size_t idx = 0;
    if (!player_id_to_index_.try_get(id, idx)) {
        return false;
    }
    if (idx >= player_count_) {
        return false;
    }
    out = player_store_[idx];
    return true;
}

bool ScoutingService::passes_common_filters(const PlayerRecord& p, const PlayerListQuery& q) {
    if (q.use_school_filter && p.school_id != q.school_id) {
        return false;
    }
    if (q.use_spot_filter && p.preferred_spot != q.preferred_spot) {
        return false;
    }
    return true;
}

static bool in_opt_range(int v, bool use_lo, int lo, bool use_hi, int hi) {
    if (use_lo && v < lo) {
        return false;
    }
    if (use_hi && v > hi) {
        return false;
    }
    return true;
}

bool ScoutingService::passes_stat_filters(const PlayerRecord& p, const StatRangeFilter& f) {
    return in_opt_range(p.shoot, f.use_min_shoot, f.min_shoot, f.use_max_shoot, f.max_shoot) &&
           in_opt_range(p.tackle, f.use_min_tackle, f.min_tackle, f.use_max_tackle, f.max_tackle) &&
           in_opt_range(p.speed, f.use_min_speed, f.min_speed, f.use_max_speed, f.max_speed) &&
           in_opt_range(p.accuracy, f.use_min_accuracy, f.min_accuracy, f.use_max_accuracy, f.max_accuracy) &&
           in_opt_range(p.awareness, f.use_min_awareness, f.min_awareness, f.use_max_awareness, f.max_awareness);
}

ScoutingService::AuthOutcome ScoutingService::register_user(const std::string& full_name, const std::string& email,
                                                             const std::string& password, const std::string& role) {
    AuthOutcome o;
    if (full_name.empty() || email.empty() || password.empty()) {
        o.ok = false;
        o.error_code = "VALIDATION_ERROR";
        o.message = "full_name, email, password required";
        return o;
    }
    UserRecord u{};
    u.full_name = full_name;
    u.email = email;
    u.password = password;
    u.role = normalize_role(role);
    std::uint64_t new_id = 0;
    if (!repo_.insert_user(u, new_id)) {
        o.ok = false;
        o.error_code = "REGISTER_FAILED";
        o.message = repo_.last_error();
        return o;
    }
    u.id = new_id;
    o.ok = true;
    o.user = u;
    o.message = "registered";
    return o;
}

ScoutingService::AuthOutcome ScoutingService::login(const std::string& email, const std::string& password) {
    AuthOutcome o;
    UserRecord u{};
    if (!repo_.find_user_by_email(email, u) || u.password != password) {
        o.ok = false;
        o.error_code = "INVALID_CREDENTIALS";
        o.message = "invalid email or password";
        return o;
    }
    const std::string token_plain = random_session_token();
    std::uint64_t sid = 0;
    if (!repo_.insert_session(u.id, token_plain, sid)) {
        o.ok = false;
        o.error_code = "LOGIN_FAILED";
        o.message = repo_.last_error();
        return o;
    }
    (void)sid;
    o.ok = true;
    o.token_plain = token_plain;
    o.user = u;
    o.message = "logged in";
    return o;
}

bool ScoutingService::logout(const std::string& token_plain) {
    if (token_plain.empty()) {
        return false;
    }
    return repo_.revoke_session(token_plain);
}

bool ScoutingService::user_from_token(const std::string& token_plain, UserRecord& out) {
    if (token_plain.empty()) {
        return false;
    }
    return repo_.find_user_for_valid_token(token_plain, out);
}

bool ScoutingService::list_schools(LinkedList<SchoolRecord>& out) {
    out.clear();
    schools_by_id_.for_each([&](std::uint64_t, const SchoolRecord& s) {
        out.push_back(s);
    });
    return true;
}

bool ScoutingService::get_school(std::uint64_t id, SchoolRecord& out) {
    return schools_by_id_.try_get(id, out);
}

bool ScoutingService::create_school(const SchoolRecord& in, std::uint64_t& new_id, std::string& err) {
    SchoolRecord copy = in;
    copy.id = 0;
    if (!repo_.insert_school(copy, new_id)) {
        err = repo_.last_error();
        return false;
    }
    copy.id = new_id;
    schools_by_id_.insert_or_assign(new_id, copy);
    return true;
}

bool ScoutingService::update_school(const SchoolRecord& in, std::string& err) {
    if (!schools_by_id_.contains(in.id)) {
        err = "school not found";
        return false;
    }
    if (!repo_.update_school(in)) {
        err = repo_.last_error();
        return false;
    }
    schools_by_id_.insert_or_assign(in.id, in);
    return true;
}

bool ScoutingService::delete_school(std::uint64_t id, std::string& err) {
    if (!schools_by_id_.contains(id)) {
        err = "school not found";
        return false;
    }
    if (!repo_.delete_school(id)) {
        err = repo_.last_error();
        return false;
    }
    schools_by_id_.erase(id);
    return true;
}

bool ScoutingService::create_player(const PlayerRecord& in, std::uint64_t& new_id, std::string& err) {
    if (!schools_by_id_.contains(in.school_id)) {
        err = "school not found";
        return false;
    }
    if (player_count_ >= ScoutingRepository::kMaxRecords) {
        err = "capacity reached";
        return false;
    }
    if (!repo_.insert_player_with_stats(in, new_id)) {
        err = repo_.last_error();
        return false;
    }
    PlayerRecord loaded = in;
    loaded.id = new_id;
    const std::size_t idx = player_count_;
    player_store_[idx] = std::move(loaded);
    const PlayerRecord& pr = player_store_[idx];
    player_id_to_index_.insert_or_assign(pr.id, idx);

    RankKeys rk{};
    rk.top_key = top_sort_key(pr);
    avl_top_.insert(rk.top_key, pr.id);
    if (pr.potential_score >= 1 && pr.potential_score <= 100) {
        rk.pot_key = pot_sort_key(pr.id, pr.potential_score);
        rk.in_potential_avl = true;
        avl_potential_.insert(rk.pot_key, pr.id);
    }
    player_rank_keys_.insert_or_assign(pr.id, rk);
    ++player_count_;
    return true;
}

bool ScoutingService::get_player(std::uint64_t id, PlayerRecord& out) {
    return fetch_player(id, out);
}

bool ScoutingService::players_for_school(std::uint64_t school_id, LinkedList<PlayerRecord>& out) {
    out.clear();
    if (!schools_by_id_.contains(school_id)) {
        return false;
    }
    for (std::size_t i = 0; i < player_count_; ++i) {
        const PlayerRecord& p = player_store_[i];
        if (p.school_id == school_id) {
            out.push_back(p);
        }
    }
    return true;
}

bool ScoutingService::ranked_top_players(const PlayerListQuery& q, LinkedList<PlayerRecord>& out) {
    out.clear();
    const int limit = std::max(1, std::min(q.limit, 500));
    int taken = 0;
    avl_top_.walk_in_order([&](int64_t, std::uint64_t pid) {
        if (taken >= limit) {
            return;
        }
        PlayerRecord p{};
        if (!fetch_player(pid, p)) {
            return;
        }
        if (!passes_common_filters(p, q)) {
            return;
        }
        if (!passes_stat_filters(p, q.stats)) {
            return;
        }
        out.push_back(p);
        ++taken;
    });
    return true;
}

bool ScoutingService::ranked_potential_players(const PlayerListQuery& q, LinkedList<PlayerRecord>& out) {
    out.clear();
    const int limit = std::max(1, std::min(q.limit, 500));
    int taken = 0;
    avl_potential_.walk_in_order([&](int64_t, std::uint64_t pid) {
        if (taken >= limit) {
            return;
        }
        PlayerRecord p{};
        if (!fetch_player(pid, p)) {
            return;
        }
        if (p.potential_score < 1 || p.potential_score > 100) {
            return;
        }
        if (!passes_common_filters(p, q)) {
            return;
        }
        if (!passes_stat_filters(p, q.stats)) {
            return;
        }
        out.push_back(p);
        ++taken;
    });
    return true;
}

bool ScoutingService::create_visit(const VisitRecord& in, std::uint64_t& new_id, std::string& err) {
    if (!schools_by_id_.contains(in.school_id)) {
        err = "school not found";
        return false;
    }
    if (!repo_.insert_visit(in, new_id)) {
        err = repo_.last_error();
        return false;
    }
    VisitRecord v = in;
    v.id = new_id;
    visit_log_.push_back(std::move(v));
    return true;
}

bool ScoutingService::create_report(const ReportRecord& in, std::uint64_t& new_id, std::string& err) {
    if (!player_id_to_index_.contains(in.player_id)) {
        err = "player not found";
        return false;
    }
    if (!repo_.insert_report(in, new_id)) {
        err = repo_.last_error();
        return false;
    }
    ReportRecord r = in;
    r.id = new_id;
    auto** slot = reports_by_player_.find_mut(r.player_id);
    if (!slot) {
        reports_by_player_.insert_or_assign(r.player_id, new LinkedList<ReportRecord>());
        slot = reports_by_player_.find_mut(r.player_id);
    }
    if (slot && *slot) {
        (*slot)->push_back(std::move(r));
    }
    return true;
}

bool ScoutingService::reports_for_player(std::uint64_t player_id, LinkedList<ReportRecord>& out) {
    out.clear();
    if (!player_id_to_index_.contains(player_id)) {
        return false;
    }
    auto** slot = reports_by_player_.find_mut(player_id);
    if (!slot || !*slot) {
        return true;
    }
    (*slot)->for_each([&](const ReportRecord& r) {
        out.push_back(r);
    });
    return true;
}

void ScoutingService::rebuild_team_roster_unlocked(std::uint64_t team_id) {
    std::array<TeamPlayerRecord, ScoutingRepository::kMaxRecords> rows{};
    std::size_t n = 0;
    if (!repo_.load_team_players_for_team(team_id, rows, n)) {
        return;
    }
    auto** slot = team_rosters_.find_mut(team_id);
    if (slot && *slot) {
        delete *slot;
        *slot = nullptr;
    }
    auto* nl = new LinkedList<TeamPlayerRecord>();
    for (std::size_t i = 0; i < n; ++i) {
        nl->push_back(std::move(rows[i]));
    }
    team_rosters_.insert_or_assign(team_id, nl);
}

bool ScoutingService::create_team(const TeamRecord& in, std::uint64_t& new_id, std::string& err) {
    if (!is_supported_formation(in.formation)) {
        err = "unsupported formation; use a preset from GET /formations";
        return false;
    }
    TeamRecord copy = in;
    copy.id = 0;
    if (!repo_.insert_team(copy, new_id)) {
        err = repo_.last_error();
        return false;
    }
    copy.id = new_id;
    teams_by_id_.insert_or_assign(new_id, copy);
    team_rosters_.insert_or_assign(new_id, new LinkedList<TeamPlayerRecord>());
    return true;
}

bool ScoutingService::update_team(const TeamRecord& in, std::uint64_t acting_user_id, const std::string& acting_role,
                                  std::string& err) {
    if (!is_supported_formation(in.formation)) {
        err = "unsupported formation; use a preset from GET /formations";
        return false;
    }
    TeamRecord existing{};
    if (!teams_by_id_.try_get(in.id, existing)) {
        err = "team not found";
        return false;
    }
    if (!team_manage_allowed(existing, acting_user_id, acting_role)) {
        err = "forbidden";
        return false;
    }
    TeamRecord updated = in;
    updated.owner_user_id = existing.owner_user_id;
    if (!repo_.update_team(updated)) {
        err = repo_.last_error();
        return false;
    }
    teams_by_id_.insert_or_assign(in.id, updated);
    return true;
}

bool ScoutingService::delete_team(std::uint64_t id, std::uint64_t acting_user_id, const std::string& acting_role,
                                  std::string& err) {
    TeamRecord existing{};
    if (!teams_by_id_.try_get(id, existing)) {
        err = "team not found";
        return false;
    }
    if (!team_manage_allowed(existing, acting_user_id, acting_role)) {
        err = "forbidden";
        return false;
    }
    if (!repo_.delete_team(id)) {
        err = repo_.last_error();
        return false;
    }
    teams_by_id_.erase(id);
    auto** slot = team_rosters_.find_mut(id);
    if (slot && *slot) {
        delete *slot;
        team_rosters_.erase(id);
    }
    return true;
}

bool ScoutingService::get_team(std::uint64_t id, TeamRecord& out) {
    return teams_by_id_.try_get(id, out);
}

bool ScoutingService::list_team_players(std::uint64_t team_id, LinkedList<TeamPlayerRecord>& out) {
    out.clear();
    if (!teams_by_id_.contains(team_id)) {
        return false;
    }
    auto** slot = team_rosters_.find_mut(team_id);
    if (!slot || !*slot) {
        rebuild_team_roster_unlocked(team_id);
        slot = team_rosters_.find_mut(team_id);
    }
    if (!slot || !*slot) {
        return true;
    }
    (*slot)->for_each([&](const TeamPlayerRecord& tp) {
        out.push_back(tp);
    });
    return true;
}

bool ScoutingService::team_add_player(std::uint64_t team_id, std::uint64_t player_id, PreferredSpot spot,
                                      int order_index, std::uint64_t acting_user_id, const std::string& acting_role,
                                      std::string& err) {
    TeamRecord team{};
    if (!teams_by_id_.try_get(team_id, team)) {
        err = "team not found";
        return false;
    }
    if (!team_manage_allowed(team, acting_user_id, acting_role)) {
        err = "forbidden";
        return false;
    }
    if (!player_id_to_index_.contains(player_id)) {
        err = "player not found";
        return false;
    }
    auto** slot = team_rosters_.find_mut(team_id);
    if (!slot || !*slot) {
        rebuild_team_roster_unlocked(team_id);
        slot = team_rosters_.find_mut(team_id);
    }
    if (slot && *slot) {
        bool dup = false;
        (*slot)->for_each([&](const TeamPlayerRecord& tp) {
            if (tp.player_id == player_id) {
                dup = true;
            }
        });
        if (dup) {
            err = "player already on team";
            return false;
        }
    }
    TeamPlayerRecord tp{};
    tp.team_id = team_id;
    tp.player_id = player_id;
    tp.assigned_spot = spot;
    tp.order_index = order_index;
    std::uint64_t row_id = 0;
    if (!repo_.insert_team_player(tp, row_id)) {
        err = repo_.last_error();
        return false;
    }
    tp.id = row_id;
    if (!slot) {
        team_rosters_.insert_or_assign(team_id, new LinkedList<TeamPlayerRecord>());
        slot = team_rosters_.find_mut(team_id);
    }
    if (slot && *slot) {
        (*slot)->push_back(tp);
    }
    return true;
}

bool ScoutingService::suggest_lineup(const std::string& formation_id, bool use_school_filter,
                                     std::uint64_t school_id, std::vector<LineupSlotPick>& out, std::string& err) {
    out.clear();
    if (!is_supported_formation(formation_id)) {
        err = "unsupported formation";
        return false;
    }
    std::vector<PlayerRecord> pool;
    pool.reserve(player_count_);
    for (std::size_t i = 0; i < player_count_; ++i) {
        const PlayerRecord& p = player_store_[i];
        if (use_school_filter && p.school_id != school_id) {
            continue;
        }
        pool.push_back(p);
    }
    FormationTemplate tmpl{};
    formation_template_by_id(formation_id, tmpl);
    if (pool.size() < tmpl.slots.size()) {
        err = "not enough players in pool for this formation";
        return false;
    }
    out = compute_optimal_lineup(formation_id, pool);
    if (out.empty()) {
        err = "could not build lineup";
        return false;
    }
    std::unordered_set<std::uint64_t> unique_ids;
    for (const LineupSlotPick& pick : out) {
        if (pick.player_id == 0 || !unique_ids.insert(pick.player_id).second) {
            out.clear();
            err = "could not build lineup without duplicate players";
            return false;
        }
    }
    return true;
}

bool ScoutingService::team_set_lineup(std::uint64_t team_id, const std::string& formation_id,
                                      const std::vector<LineupSlotPick>& picks, std::uint64_t acting_user_id,
                                      const std::string& acting_role, std::string& err) {
    TeamRecord team{};
    if (!teams_by_id_.try_get(team_id, team)) {
        err = "team not found";
        return false;
    }
    if (!team_manage_allowed(team, acting_user_id, acting_role)) {
        err = "forbidden";
        return false;
    }
    if (!is_supported_formation(formation_id)) {
        err = "unsupported formation";
        return false;
    }
    FormationTemplate tmpl{};
    formation_template_by_id(formation_id, tmpl);
    if (picks.size() != tmpl.slots.size()) {
        err = "lineup must have exactly 11 players for this formation";
        return false;
    }

    std::unordered_set<std::uint64_t> seen_players;
    for (const LineupSlotPick& pick : picks) {
        if (pick.player_id == 0 || !player_id_to_index_.contains(pick.player_id)) {
            err = "invalid player in lineup";
            return false;
        }
        if (!seen_players.insert(pick.player_id).second) {
            err = "duplicate player in lineup";
            return false;
        }
    }

    LinkedList<TeamPlayerRecord> current;
    list_team_players(team_id, current);
    current.for_each([&](const TeamPlayerRecord& tp) {
        repo_.remove_team_player(team_id, tp.player_id);
    });
    rebuild_team_roster_unlocked(team_id);

    if (team.formation != formation_id) {
        team.formation = formation_id;
        if (!repo_.update_team(team)) {
            err = repo_.last_error();
            return false;
        }
        teams_by_id_.insert_or_assign(team_id, team);
    }

    for (const LineupSlotPick& pick : picks) {
        TeamPlayerRecord tp{};
        tp.team_id = team_id;
        tp.player_id = pick.player_id;
        tp.assigned_spot = pick.assigned_spot;
        tp.order_index = pick.order_index;
        std::uint64_t row_id = 0;
        if (!repo_.insert_team_player(tp, row_id)) {
            err = repo_.last_error();
            rebuild_team_roster_unlocked(team_id);
            return false;
        }
    }
    rebuild_team_roster_unlocked(team_id);
    return true;
}

bool ScoutingService::team_remove_player(std::uint64_t team_id, std::uint64_t player_id,
                                         std::uint64_t acting_user_id, const std::string& acting_role,
                                         std::string& err) {
    TeamRecord team{};
    if (!teams_by_id_.try_get(team_id, team)) {
        err = "team not found";
        return false;
    }
    if (!team_manage_allowed(team, acting_user_id, acting_role)) {
        err = "forbidden";
        return false;
    }
    if (!repo_.remove_team_player(team_id, player_id)) {
        err = repo_.last_error();
        return false;
    }
    rebuild_team_roster_unlocked(team_id);
    return true;
}

bool ScoutingService::team_analysis(std::uint64_t team_id, int& player_count, long long& sum_composite,
                                    double& avg_composite, std::string& err) {
    if (!teams_by_id_.contains(team_id)) {
        err = "team not found";
        return false;
    }
    auto** slot = team_rosters_.find_mut(team_id);
    if (!slot || !*slot) {
        rebuild_team_roster_unlocked(team_id);
        slot = team_rosters_.find_mut(team_id);
    }
    player_count = 0;
    sum_composite = 0;
    avg_composite = 0;
    if (!slot || !*slot) {
        return true;
    }
    (*slot)->for_each([&](const TeamPlayerRecord& tp) {
        PlayerRecord p{};
        if (fetch_player(tp.player_id, p)) {
            ++player_count;
            sum_composite += composite_total(p);
        }
    });
    if (player_count > 0) {
        avg_composite = static_cast<double>(sum_composite) / static_cast<double>(player_count);
    }
    return true;
}

bool ScoutingService::simulate_team_passes(std::uint64_t team_id, std::uint64_t start_player_id, int max_steps,
                                           unsigned risk_seed, FormationGraph& graph_out,
                                           AttackSimulationResult& attack_out, std::string& err) {
    TeamRecord team{};
    if (!teams_by_id_.try_get(team_id, team)) {
        err = "team not found";
        return false;
    }
    auto** slot = team_rosters_.find_mut(team_id);
    if (!slot || !*slot) {
        rebuild_team_roster_unlocked(team_id);
        slot = team_rosters_.find_mut(team_id);
    }
    if (!slot || !*slot) {
        err = "team has no players";
        return false;
    }

    std::vector<TeamPlayerRecord> roster;
    (*slot)->for_each([&](const TeamPlayerRecord& tp) { roster.push_back(tp); });
    if (roster.empty()) {
        err = "team has no players";
        return false;
    }

    std::vector<PlayerRecord> players;
    for (const TeamPlayerRecord& tp : roster) {
        PlayerRecord p{};
        if (fetch_player(tp.player_id, p)) {
            players.push_back(p);
        }
    }
    if (players.empty()) {
        err = "no roster players found in store";
        return false;
    }

    if (max_steps < 1) {
        max_steps = 8;
    }
    if (max_steps > 32) {
        max_steps = 32;
    }

    graph_out = build_formation_graph(team.formation, players, roster);
    if (graph_out.nodes.empty()) {
        err = "could not place players on formation grid";
        return false;
    }

    std::uint64_t start = start_player_id;
    if (start == 0) {
        for (const PitchNode& n : graph_out.nodes) {
            if (n.rank == 0) {
                start = n.player_id;
                break;
            }
        }
        if (start == 0) {
            start = graph_out.nodes.front().player_id;
        }
    }

    unsigned seed = risk_seed;
    if (seed == 0) {
        seed = static_cast<unsigned>(team_id * 131u + static_cast<unsigned>(std::time(nullptr) % 100000u));
    }
    attack_out = simulate_attack_on_goal(graph_out, start, max_steps, seed);
    return true;
}
