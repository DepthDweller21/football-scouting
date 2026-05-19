#pragma once

#include "model/domain.h"

#include <string>
#include <vector>

struct FormationSlot {
    PreferredSpot spot{PreferredSpot::CM};
    int order_index{0};
    /** Horizontal position on pitch: 0 = left touchline, 100 = right. */
    double x_lane{50.0};
    /** Vertical band: 0 = GK, higher = closer to opponent goal. */
    int rank{0};
};

struct FormationTemplate {
    std::string id;
    std::string label;
    std::string formation;
    std::vector<FormationSlot> slots;
};

struct LineupSlotPick {
    PreferredSpot assigned_spot{PreferredSpot::CM};
    int order_index{0};
    std::uint64_t player_id{0};
    int fit_score{0};
};

/** All supported formations (fixed catalog). */
const std::vector<FormationTemplate>& all_formation_templates();

bool formation_template_by_id(const std::string& formation_id, FormationTemplate& out);
bool is_supported_formation(const std::string& formation_id);

/** Pitch layout for a template slot, or false if unknown. */
bool formation_slot_layout(const std::string& formation_id, int order_index, int& rank_out, double& x_lane_out);

int formation_template_max_rank(const std::string& formation_id);

/** Role-weighted fit score for assigning a player to a slot. */
int player_fit_score_for_slot(const PlayerRecord& player, PreferredSpot slot);

/**
 * Greedy optimal XI: each slot gets the best unused player from the pool.
 * Returns empty if the template is unknown or the pool is too small.
 */
std::vector<LineupSlotPick> compute_optimal_lineup(const std::string& formation_id,
                                                   const std::vector<PlayerRecord>& pool);
