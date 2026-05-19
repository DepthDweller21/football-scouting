#include "model/formation_templates.h"

#include "model/formation_graph.h"

#include <algorithm>
#include <unordered_set>

namespace {

struct SlotDef {
    PreferredSpot spot;
    double x_lane;
    int rank;
};

FormationTemplate make_template(std::string id, std::string label, std::initializer_list<SlotDef> slots) {
    FormationTemplate t{};
    t.id = id;
    t.label = label;
    t.formation = id;
    int order = 0;
    for (const SlotDef& def : slots) {
        t.slots.push_back(FormationSlot{def.spot, order++, def.x_lane, def.rank});
    }
    return t;
}

const std::vector<FormationTemplate>& catalog() {
    static const std::vector<FormationTemplate> kTemplates = {
        make_template("4-3-3", "4-3-3 (balanced attack)",
                      {{PreferredSpot::GK, 50, 0},
                       {PreferredSpot::LB, 18, 1},
                       {PreferredSpot::CB, 38, 1},
                       {PreferredSpot::CB, 62, 1},
                       {PreferredSpot::RB, 82, 1},
                       {PreferredSpot::CM, 32, 2},
                       {PreferredSpot::CDM, 50, 2},
                       {PreferredSpot::CAM, 68, 2},
                       {PreferredSpot::LW, 18, 3},
                       {PreferredSpot::ST, 50, 3},
                       {PreferredSpot::RW, 82, 3}}),
        make_template("4-4-2", "4-4-2 (compact midfield)",
                      {{PreferredSpot::GK, 50, 0},
                       {PreferredSpot::LB, 18, 1},
                       {PreferredSpot::CB, 38, 1},
                       {PreferredSpot::CB, 62, 1},
                       {PreferredSpot::RB, 82, 1},
                       {PreferredSpot::LW, 28, 2},
                       {PreferredSpot::CM, 42, 2},
                       {PreferredSpot::CM, 58, 2},
                       {PreferredSpot::RW, 72, 2},
                       {PreferredSpot::ST, 38, 3},
                       {PreferredSpot::ST, 62, 3}}),
        make_template("4-2-3-1", "4-2-3-1 (double pivot)",
                      {{PreferredSpot::GK, 50, 0},
                       {PreferredSpot::LB, 18, 1},
                       {PreferredSpot::CB, 38, 1},
                       {PreferredSpot::CB, 62, 1},
                       {PreferredSpot::RB, 82, 1},
                       {PreferredSpot::CDM, 38, 2},
                       {PreferredSpot::CDM, 62, 2},
                       {PreferredSpot::LW, 20, 3},
                       {PreferredSpot::CAM, 50, 3},
                       {PreferredSpot::RW, 80, 3},
                       {PreferredSpot::ST, 50, 4}}),
        make_template("3-5-2", "3-5-2 (wing play)",
                      {{PreferredSpot::GK, 50, 0},
                       {PreferredSpot::CB, 25, 1},
                       {PreferredSpot::CB, 50, 1},
                       {PreferredSpot::CB, 75, 1},
                       {PreferredSpot::CM, 35, 2},
                       {PreferredSpot::CDM, 50, 2},
                       {PreferredSpot::CM, 65, 2},
                       {PreferredSpot::LW, 15, 3},
                       {PreferredSpot::RW, 85, 3},
                       {PreferredSpot::ST, 42, 4},
                       {PreferredSpot::ST, 58, 4}}),
        make_template("5-3-2", "5-3-2 (defensive block)",
                      {{PreferredSpot::GK, 50, 0},
                       {PreferredSpot::LB, 15, 1},
                       {PreferredSpot::CB, 32, 1},
                       {PreferredSpot::CB, 50, 1},
                       {PreferredSpot::CB, 68, 1},
                       {PreferredSpot::RB, 85, 1},
                       {PreferredSpot::CM, 30, 2},
                       {PreferredSpot::CM, 50, 2},
                       {PreferredSpot::CM, 70, 2},
                       {PreferredSpot::ST, 42, 3},
                       {PreferredSpot::ST, 58, 3}}),
        make_template("4-5-1", "4-5-1 (midfield control)",
                      {{PreferredSpot::GK, 50, 0},
                       {PreferredSpot::LB, 18, 1},
                       {PreferredSpot::CB, 38, 1},
                       {PreferredSpot::CB, 62, 1},
                       {PreferredSpot::RB, 82, 1},
                       {PreferredSpot::CDM, 50, 2},
                       {PreferredSpot::CM, 35, 3},
                       {PreferredSpot::CAM, 65, 3},
                       {PreferredSpot::LW, 15, 4},
                       {PreferredSpot::RW, 85, 4},
                       {PreferredSpot::ST, 50, 5}}),
    };
    return kTemplates;
}

bool spot_is_wide(PreferredSpot s) {
    return s == PreferredSpot::RW || s == PreferredSpot::LW || s == PreferredSpot::RB || s == PreferredSpot::LB;
}

int stat_profile_score(const PlayerRecord& p, PreferredSpot slot) {
    switch (slot) {
        case PreferredSpot::GK:
            return p.awareness * 3 + p.tackle * 2 + p.accuracy;
        case PreferredSpot::CB:
            return p.tackle * 3 + p.awareness * 2 + p.speed;
        case PreferredSpot::RB:
        case PreferredSpot::LB:
            return p.tackle * 2 + p.speed * 2 + p.awareness + p.accuracy;
        case PreferredSpot::CDM:
            return p.tackle * 2 + p.awareness * 2 + p.accuracy + p.speed;
        case PreferredSpot::CM:
            return p.accuracy * 2 + p.awareness * 2 + p.tackle + p.speed;
        case PreferredSpot::CAM:
            return p.shoot * 2 + p.accuracy * 2 + p.awareness + p.speed;
        case PreferredSpot::RW:
        case PreferredSpot::LW:
            return p.speed * 3 + p.shoot * 2 + p.accuracy;
        case PreferredSpot::ST:
            return p.shoot * 3 + p.speed * 2 + p.accuracy;
    }
    return p.shoot + p.tackle + p.speed + p.accuracy + p.awareness;
}

}  // namespace

const std::vector<FormationTemplate>& all_formation_templates() {
    return catalog();
}

bool formation_template_by_id(const std::string& formation_id, FormationTemplate& out) {
    for (const FormationTemplate& t : catalog()) {
        if (t.id == formation_id) {
            out = t;
            return true;
        }
    }
    return false;
}

bool is_supported_formation(const std::string& formation_id) {
    FormationTemplate tmp{};
    return formation_template_by_id(formation_id, tmp);
}

bool formation_slot_layout(const std::string& formation_id, int order_index, int& rank_out, double& x_lane_out) {
    FormationTemplate tmpl{};
    if (!formation_template_by_id(formation_id, tmpl)) {
        return false;
    }
    for (const FormationSlot& s : tmpl.slots) {
        if (s.order_index == order_index) {
            rank_out = s.rank;
            x_lane_out = s.x_lane;
            return true;
        }
    }
    return false;
}

int formation_template_max_rank(const std::string& formation_id) {
    FormationTemplate tmpl{};
    if (!formation_template_by_id(formation_id, tmpl)) {
        return 0;
    }
    int max_rank = 0;
    for (const FormationSlot& s : tmpl.slots) {
        max_rank = std::max(max_rank, s.rank);
    }
    return max_rank;
}

int player_fit_score_for_slot(const PlayerRecord& player, PreferredSpot slot) {
    int score = stat_profile_score(player, slot);

    if (player.preferred_spot == slot) {
        score += 5000;
    } else if (tactical_line_for_spot(player.preferred_spot) == tactical_line_for_spot(slot)) {
        score += 900;
    }

    if (spot_is_wide(player.preferred_spot) && spot_is_wide(slot)) {
        score += 400;
    }

    if (player.potential_score >= 1 && player.potential_score <= 100) {
        score += player.potential_score * 8;
    }

    return score;
}

std::vector<LineupSlotPick> compute_optimal_lineup(const std::string& formation_id,
                                                   const std::vector<PlayerRecord>& pool) {
    FormationTemplate tmpl{};
    if (!formation_template_by_id(formation_id, tmpl)) {
        return {};
    }
    if (pool.size() < tmpl.slots.size()) {
        return {};
    }

    std::unordered_set<std::uint64_t> used;
    std::vector<LineupSlotPick> picks;
    picks.reserve(tmpl.slots.size());

    for (const FormationSlot& slot : tmpl.slots) {
        int best_score = -1;
        const PlayerRecord* best = nullptr;
        for (const PlayerRecord& p : pool) {
            if (p.id == 0 || used.count(p.id) != 0) {
                continue;
            }
            const int fit = player_fit_score_for_slot(p, slot.spot);
            if (fit > best_score) {
                best_score = fit;
                best = &p;
            }
        }
        if (!best || best->id == 0) {
            return {};
        }
        used.insert(best->id);
        LineupSlotPick pick{};
        pick.assigned_spot = slot.spot;
        pick.order_index = slot.order_index;
        pick.player_id = best->id;
        pick.fit_score = best_score;
        picks.push_back(pick);
    }

    if (used.size() != picks.size()) {
        return {};
    }

    return picks;
}
