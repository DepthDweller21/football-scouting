#pragma once

#include "model/formation_graph.h"
#include "model/formation_templates.h"
#include "model/scouting_repository.h"
#include "structures/AVLTree.h"
#include "structures/HashTable.h"
#include "structures/LinkedList.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

struct StatRangeFilter {
    bool use_min_shoot{false};
    int min_shoot{0};
    bool use_max_shoot{false};
    int max_shoot{0};
    bool use_min_tackle{false};
    int min_tackle{0};
    bool use_max_tackle{false};
    int max_tackle{0};
    bool use_min_speed{false};
    int min_speed{0};
    bool use_max_speed{false};
    int max_speed{0};
    bool use_min_accuracy{false};
    int min_accuracy{0};
    bool use_max_accuracy{false};
    int max_accuracy{0};
    bool use_min_awareness{false};
    int min_awareness{0};
    bool use_max_awareness{false};
    int max_awareness{0};
};

struct PlayerListQuery {
    int limit{50};
    bool use_school_filter{false};
    std::uint64_t school_id{0};
    bool use_spot_filter{false};
    PreferredSpot preferred_spot{PreferredSpot::CM};
    StatRangeFilter stats;
};

/**
 * Application logic and in-memory indexes — sits between HTTP controllers and ScoutingRepository.
 * (Often called the “service” or “use-case” layer in MVC-style APIs.)
 */
class ScoutingService {
public:
    explicit ScoutingService(ScoutingRepository& repo);

    bool hydrate();
    std::string last_repo_error() const { return repo_.last_error(); }

    struct AuthOutcome {
        bool ok{false};
        std::string error_code;
        std::string message;
        std::string token_plain;
        UserRecord user{};
    };

    AuthOutcome register_user(const std::string& full_name, const std::string& email, const std::string& password,
                              const std::string& role);
    AuthOutcome login(const std::string& email, const std::string& password);
    bool logout(const std::string& token_plain);
    bool user_from_token(const std::string& token_plain, UserRecord& out);

    static bool validate_stat_1_100(int v);
    static bool role_can_write_scouting(const std::string& role);
    static bool role_is_admin(const std::string& role);

    bool list_schools(LinkedList<SchoolRecord>& out);
    bool get_school(std::uint64_t id, SchoolRecord& out);
    bool create_school(const SchoolRecord& in, std::uint64_t& new_id, std::string& err);
    bool update_school(const SchoolRecord& in, std::string& err);
    bool delete_school(std::uint64_t id, std::string& err);

    bool create_player(const PlayerRecord& in, std::uint64_t& new_id, std::string& err);
    bool get_player(std::uint64_t id, PlayerRecord& out);
    bool players_for_school(std::uint64_t school_id, LinkedList<PlayerRecord>& out);

    bool ranked_top_players(const PlayerListQuery& q, LinkedList<PlayerRecord>& out);
    bool ranked_potential_players(const PlayerListQuery& q, LinkedList<PlayerRecord>& out);

    bool create_visit(const VisitRecord& in, std::uint64_t& new_id, std::string& err);
    bool create_report(const ReportRecord& in, std::uint64_t& new_id, std::string& err);
    bool reports_for_player(std::uint64_t player_id, LinkedList<ReportRecord>& out);

    bool create_team(const TeamRecord& in, std::uint64_t& new_id, std::string& err);
    bool update_team(const TeamRecord& in, std::uint64_t acting_user_id, const std::string& acting_role,
                     std::string& err);
    bool delete_team(std::uint64_t id, std::uint64_t acting_user_id, const std::string& acting_role, std::string& err);
    bool get_team(std::uint64_t id, TeamRecord& out);
    bool list_team_players(std::uint64_t team_id, LinkedList<TeamPlayerRecord>& out);
    bool team_add_player(std::uint64_t team_id, std::uint64_t player_id, PreferredSpot spot, int order_index,
                         std::uint64_t acting_user_id, const std::string& acting_role, std::string& err);
    bool team_remove_player(std::uint64_t team_id, std::uint64_t player_id, std::uint64_t acting_user_id,
                            const std::string& acting_role, std::string& err);
    bool suggest_lineup(const std::string& formation_id, bool use_school_filter, std::uint64_t school_id,
                        std::vector<LineupSlotPick>& out, std::string& err);
    bool team_set_lineup(std::uint64_t team_id, const std::string& formation_id,
                         const std::vector<LineupSlotPick>& picks, std::uint64_t acting_user_id,
                         const std::string& acting_role, std::string& err);
    bool team_analysis(std::uint64_t team_id, int& player_count, long long& sum_composite, double& avg_composite,
                       std::string& err);
    bool simulate_team_passes(std::uint64_t team_id, std::uint64_t start_player_id, int max_steps,
                              unsigned risk_seed, FormationGraph& graph_out, AttackSimulationResult& attack_out,
                              std::string& err);

private:
    ScoutingRepository& repo_;

    std::array<PlayerRecord, ScoutingRepository::kMaxRecords> player_store_{};
    std::size_t player_count_{0};
    HashTable<std::uint64_t, std::size_t> player_id_to_index_;

    HashTable<std::uint64_t, SchoolRecord> schools_by_id_;

    AVLTree avl_top_;
    AVLTree avl_potential_;

    struct RankKeys {
        std::int64_t top_key{0};
        std::int64_t pot_key{0};
        bool in_potential_avl{false};
    };
    HashTable<std::uint64_t, RankKeys> player_rank_keys_;

    LinkedList<VisitRecord> visit_log_;
    HashTable<std::uint64_t, LinkedList<ReportRecord>*> reports_by_player_;

    HashTable<std::uint64_t, TeamRecord> teams_by_id_;
    HashTable<std::uint64_t, LinkedList<TeamPlayerRecord>*> team_rosters_;

    void release_dynamic_lists();

    static std::int64_t composite_total(const PlayerRecord& p);
    static std::int64_t top_sort_key(const PlayerRecord& p);
    static std::int64_t pot_sort_key(std::uint64_t player_id, int potential_1_100);

    static bool passes_common_filters(const PlayerRecord& p, const PlayerListQuery& q);
    static bool passes_stat_filters(const PlayerRecord& p, const StatRangeFilter& f);

    bool fetch_player(std::uint64_t id, PlayerRecord& out);

    void rebuild_team_roster_unlocked(std::uint64_t team_id);
};
