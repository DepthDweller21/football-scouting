#pragma once

#include "model/domain.h"

#include <cstdint>
#include <string>
#include <vector>

/** One player on the tactical grid (rank = line, col = slot on that line). */
struct PitchNode {
    std::uint64_t player_id{0};
    std::string name;
    std::string spot;
    int rank{0};
    int col{0};
    int line_width{1};
    double x_pct{50.0};
    double y_pct{50.0};
    int composite{0};
    int risk_factor{50};  // 1–100, assigned per attack run; lower = safer pass target
};

struct PitchEdge {
    std::uint64_t from{0};
    std::uint64_t to{0};
};

struct PassStep {
    std::uint64_t from_player_id{0};
    std::uint64_t to_player_id{0};
};

struct OpponentGoal {
    double x_pct{50.0};
    double y_pct{6.0};
};

struct PassScoreBreakdown {
    std::int64_t safety_from_risk{0};
    std::int64_t composite_part{0};
    std::int64_t forward_bonus{0};
    std::int64_t goal_proximity{0};
    std::int64_t total{0};
};

struct PassCandidateEval {
    std::uint64_t player_id{0};
    std::string name;
    int risk_factor{0};
    int effective_risk{0};
    PassScoreBreakdown score{};
};

struct PlayerRiskSnapshot {
    std::uint64_t player_id{0};
    int risk_factor{0};
};

struct PassStepDetail {
    std::uint64_t from_player_id{0};
    std::uint64_t to_player_id{0};
    std::string reasoning;
    std::vector<PassCandidateEval> candidates;
    /** All players' risk values used for this pass decision (reshuffled each step). */
    std::vector<PlayerRiskSnapshot> risk_snapshot;
};

struct AttackSimulationResult {
    OpponentGoal goal{};
    std::vector<PassStepDetail> passes;
    bool scored{false};
    std::string outcome;
    unsigned risk_seed{0};
};

struct FormationGraph {
    std::string formation;
    std::vector<int> line_counts;
    std::vector<PitchNode> nodes;
    std::vector<PitchEdge> edges;
};

std::vector<int> parse_formation_lines(const std::string& formation);

int tactical_line_for_spot(PreferredSpot spot);

/** Map slot order (0 = GK) to pitch rank/col using formation line counts (e.g. 3-5-2 → 3,5,2). */
bool placement_from_order_index(const std::vector<int>& line_counts, int order_index, int& rank_out,
                                int& col_out, int& width_out);

FormationGraph build_formation_graph(const std::string& formation,
                                     const std::vector<PlayerRecord>& players,
                                     const std::vector<TeamPlayerRecord>& roster);

bool can_pass_between(const PitchNode& from, const PitchNode& to);

std::vector<std::uint64_t> adjacent_player_ids(const FormationGraph& graph, std::uint64_t from_id);

void assign_risk_factors(FormationGraph& graph, unsigned seed);

/** Nudge each player's risk by a seeded delta (clamped 1–100). */
void shift_risk_factors(FormationGraph& graph, unsigned seed);

/** Build-up toward opponent goal; max-heap picks pass targets using risk (safety), skill, and progress. */
AttackSimulationResult simulate_attack_on_goal(FormationGraph& graph, std::uint64_t start_player_id, int max_steps,
                                               unsigned risk_seed);
