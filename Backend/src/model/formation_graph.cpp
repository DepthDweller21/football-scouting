#include "model/formation_graph.h"

#include "model/formation_templates.h"
#include "structures/MaxHeap.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>

namespace {

int player_composite(const PlayerRecord& p) {
    return p.shoot + p.tackle + p.speed + p.accuracy + p.awareness;
}

const PlayerRecord* find_player(const std::vector<PlayerRecord>& players, std::uint64_t id) {
    for (const PlayerRecord& p : players) {
        if (p.id == id) {
            return &p;
        }
    }
    return nullptr;
}

const PitchNode* find_node(const FormationGraph& graph, std::uint64_t id) {
    for (const PitchNode& n : graph.nodes) {
        if (n.player_id == id) {
            return &n;
        }
    }
    return nullptr;
}

int max_rank_on_pitch(const FormationGraph& graph) {
    int mx = 0;
    for (const PitchNode& n : graph.nodes) {
        mx = std::max(mx, n.rank);
    }
    return mx;
}

double distance_to_goal(const PitchNode& n, const OpponentGoal& goal) {
    const double dx = n.x_pct - goal.x_pct;
    const double dy = n.y_pct - goal.y_pct;
    return std::sqrt(dx * dx + dy * dy);
}

constexpr int kHomeGkRiskRaise = 25;
constexpr int kFrontLineRiskReduce = 25;
constexpr int kForwardPassBonus = 850;
constexpr int kBackwardPassPenalty = -520;
constexpr int kBackwardToGkExtra = -650;

int clamp_risk(int value) {
    if (value < 1) {
        return 1;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

/** Risk used for pass-target scoring: GK less attractive, front line more attractive. */
int effective_pass_target_risk(const PitchNode& receiver, int max_rank) {
    int eff = receiver.risk_factor;
    if (receiver.rank == 0) {
        eff += kHomeGkRiskRaise;
    }
    if (max_rank > 0 && receiver.rank == max_rank) {
        eff -= kFrontLineRiskReduce;
    }
    return clamp_risk(eff);
}

PassScoreBreakdown score_pass_candidate(const PitchNode& from, const PitchNode& to, int max_rank) {
    PassScoreBreakdown b{};
    const int eff_risk = effective_pass_target_risk(to, max_rank);
    b.safety_from_risk = static_cast<std::int64_t>(100 - eff_risk) * 350;
    b.composite_part = static_cast<std::int64_t>(to.composite) * 90;
    if (to.rank > from.rank) {
        b.forward_bonus = kForwardPassBonus;
    } else if (to.rank < from.rank) {
        b.forward_bonus = kBackwardPassPenalty;
        if (to.rank == 0) {
            b.forward_bonus += kBackwardToGkExtra;
        }
    }
    const double progress = from.y_pct - to.y_pct;
    if (progress > 0) {
        b.goal_proximity = static_cast<std::int64_t>(progress * 28.0);
    }
    b.total = b.safety_from_risk + b.composite_part + b.forward_bonus + b.goal_proximity;
    return b;
}

std::string format_breakdown(const PassScoreBreakdown& b) {
    std::ostringstream oss;
    oss << "safety(risk)=" << b.safety_from_risk << ", composite=" << b.composite_part;
    if (b.forward_bonus != 0) {
        oss << ", forward=" << b.forward_bonus;
    }
    if (b.goal_proximity != 0) {
        oss << ", towardGoal=" << b.goal_proximity;
    }
    oss << " => total " << b.total;
    return oss.str();
}

void snapshot_risk_factors(const FormationGraph& graph, std::vector<PlayerRiskSnapshot>& out) {
    out.clear();
    out.reserve(graph.nodes.size());
    for (const PitchNode& n : graph.nodes) {
        out.push_back(PlayerRiskSnapshot{n.player_id, n.risk_factor});
    }
}

std::string build_pass_reasoning(const PitchNode& from, const PitchNode& to,
                                 const PassCandidateEval& chosen,
                                 const std::vector<PassCandidateEval>& ranked, int pass_index, int max_rank) {
    std::ostringstream oss;
    if (pass_index > 0) {
        oss << "Risks shifted after previous pass. ";
    } else {
        oss << "Initial risk draw. ";
    }
    oss << "GK target +" << kHomeGkRiskRaise << " eff. risk, front line −" << kFrontLineRiskReduce
        << ". ";
    const int to_eff = effective_pass_target_risk(to, max_rank);
    oss << "Pass to " << to.name << " (risk " << to.risk_factor << "→eff " << to_eff
        << ", lower eff. is safer). Winner: " << format_breakdown(chosen.score);
    oss << ". From " << from.name << " (risk " << from.risk_factor << "). ";
    bool first_alt = true;
    for (const PassCandidateEval& c : ranked) {
        if (c.player_id == chosen.player_id) {
            continue;
        }
        if (!first_alt) {
            oss << "; ";
        }
        oss << "Rejected " << c.name << " (risk " << c.risk_factor << "→eff " << c.effective_risk
            << "): " << format_breakdown(c.score);
        first_alt = false;
    }
    return oss.str();
}

bool holder_can_shoot(const PitchNode& holder, const FormationGraph& graph, const OpponentGoal& goal) {
    const int mx = max_rank_on_pitch(graph);
    if (holder.rank < mx - 1) {
        return false;
    }
    return distance_to_goal(holder, goal) <= 16.0;
}

}  // namespace

std::vector<int> parse_formation_lines(const std::string& formation) {
    std::vector<int> lines;
    std::stringstream ss(formation);
    std::string part;
    while (std::getline(ss, part, '-')) {
        if (part.empty()) {
            continue;
        }
        int n = 0;
        for (char c : part) {
            if (std::isdigit(static_cast<unsigned char>(c))) {
                n = n * 10 + (c - '0');
            }
        }
        if (n > 0) {
            lines.push_back(n);
        }
    }
    if (lines.empty()) {
        lines = {4, 3, 3};
    }
    return lines;
}

int tactical_line_for_spot(PreferredSpot spot) {
    switch (spot) {
        case PreferredSpot::GK:
            return 0;
        case PreferredSpot::LB:
        case PreferredSpot::CB:
        case PreferredSpot::RB:
            return 1;
        case PreferredSpot::CDM:
        case PreferredSpot::CM:
        case PreferredSpot::CAM:
            return 2;
        case PreferredSpot::LW:
        case PreferredSpot::RW:
        case PreferredSpot::ST:
            return 3;
    }
    return 2;
}

bool placement_from_order_index(const std::vector<int>& line_counts, int order_index, int& rank_out,
                              int& col_out, int& width_out) {
    if (order_index < 0) {
        return false;
    }
    if (order_index == 0) {
        rank_out = 0;
        col_out = 0;
        width_out = 1;
        return true;
    }
    int cursor = 1;
    for (std::size_t line = 0; line < line_counts.size(); ++line) {
        const int w = line_counts[line];
        if (w <= 0) {
            continue;
        }
        if (order_index < cursor + w) {
            rank_out = static_cast<int>(line) + 1;
            col_out = order_index - cursor;
            width_out = w;
            return true;
        }
        cursor += w;
    }
    return false;
}

namespace {

double default_lane_for_spot(PreferredSpot spot) {
    switch (spot) {
        case PreferredSpot::LB:
        case PreferredSpot::LW:
            return 18.0;
        case PreferredSpot::RB:
        case PreferredSpot::RW:
            return 82.0;
        case PreferredSpot::GK:
        case PreferredSpot::CDM:
        case PreferredSpot::CAM:
        case PreferredSpot::CM:
        case PreferredSpot::CB:
        case PreferredSpot::ST:
            return 50.0;
    }
    return 50.0;
}

void set_node_coordinates(PitchNode& node, int rank, int col, int width, int total_lines, double x_lane) {
    node.rank = rank;
    node.col = col;
    node.line_width = width;
    if (x_lane >= 0.0 && x_lane <= 100.0) {
        node.x_pct = 12.0 + (x_lane / 100.0) * 76.0;
    } else if (width <= 1) {
        node.x_pct = 50.0;
    } else {
        node.x_pct = 12.0 + (static_cast<double>(col) / static_cast<double>(width - 1)) * 76.0;
    }
    if (total_lines <= 1) {
        node.y_pct = 50.0;
    } else {
        node.y_pct = 88.0 - (static_cast<double>(rank) / static_cast<double>(total_lines - 1)) * 76.0;
    }
}

}  // namespace

FormationGraph build_formation_graph(const std::string& formation,
                                     const std::vector<PlayerRecord>& players,
                                     const std::vector<TeamPlayerRecord>& roster) {
    FormationGraph graph{};
    graph.formation = formation;
    graph.line_counts = parse_formation_lines(formation);

    const int outfield_lines = static_cast<int>(graph.line_counts.size());
    int total_lines = 1 + outfield_lines;
    const int template_max_rank = formation_template_max_rank(formation);
    if (template_max_rank > 0) {
        total_lines = template_max_rank + 1;
    }

    struct Pending {
        TeamPlayerRecord tp{};
        const PlayerRecord* player{nullptr};
        int rank{0};
        int col{0};
        int width{1};
        double x_lane{-1.0};
        bool has_template_layout{false};
    };
    std::vector<Pending> pending;

    for (const TeamPlayerRecord& tp : roster) {
        const PlayerRecord* p = find_player(players, tp.player_id);
        if (!p) {
            continue;
        }
        Pending item{tp, p, 0, 0, 1, -1.0, false};
        if (formation_slot_layout(formation, tp.order_index, item.rank, item.x_lane)) {
            item.has_template_layout = true;
        } else if (placement_from_order_index(graph.line_counts, tp.order_index, item.rank, item.col,
                                              item.width)) {
            item.has_template_layout = true;
        } else {
            item.rank = tactical_line_for_spot(tp.assigned_spot);
            if (item.rank > outfield_lines) {
                item.rank = outfield_lines;
            }
            if (item.rank < 0) {
                item.rank = 0;
            }
            if (item.rank == 0) {
                item.width = 1;
            } else if (item.rank - 1 < outfield_lines) {
                item.width = graph.line_counts[static_cast<std::size_t>(item.rank - 1)];
            }
        }
        pending.push_back(item);
    }

    std::vector<int> col_used(total_lines, 0);
    std::vector<int> count_on_rank(total_lines, 0);
    for (const Pending& item : pending) {
        if (!item.has_template_layout && item.rank >= 0 && item.rank < total_lines) {
            ++count_on_rank[item.rank];
        }
    }

    std::sort(pending.begin(), pending.end(), [](const Pending& a, const Pending& b) {
        if (a.rank != b.rank) {
            return a.rank < b.rank;
        }
        if (a.tp.order_index != b.tp.order_index) {
            return a.tp.order_index < b.tp.order_index;
        }
        return a.player->id < b.player->id;
    });

    for (Pending& item : pending) {
        int rank = item.rank;
        int col = item.col;
        int width = item.width;
        double x_lane = item.x_lane;

        if (!item.has_template_layout) {
            if (rank == 0) {
                width = 1;
            } else if (rank - 1 < outfield_lines) {
                const int formation_width = graph.line_counts[static_cast<std::size_t>(rank - 1)];
                width = std::max(formation_width, count_on_rank[rank]);
            }
            col = col_used[rank]++;
            if (width > 1 && col >= width) {
                col = width - 1;
            }
        }

        if (x_lane < 0.0) {
            x_lane = default_lane_for_spot(item.tp.assigned_spot);
            if (width > 1) {
                x_lane = 18.0 + (static_cast<double>(col) / static_cast<double>(width - 1)) * 64.0;
            }
        }

        PitchNode node{};
        node.player_id = item.player->id;
        node.name = item.player->first_name + " " + item.player->last_name;
        node.spot = preferred_spot_to_string(item.tp.assigned_spot);
        node.composite = player_composite(*item.player);
        set_node_coordinates(node, rank, col, width, total_lines, x_lane);

        graph.nodes.push_back(std::move(node));
    }

    for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
        for (std::size_t j = i + 1; j < graph.nodes.size(); ++j) {
            if (can_pass_between(graph.nodes[i], graph.nodes[j])) {
                graph.edges.push_back(PitchEdge{graph.nodes[i].player_id, graph.nodes[j].player_id});
                graph.edges.push_back(PitchEdge{graph.nodes[j].player_id, graph.nodes[i].player_id});
            }
        }
    }

    return graph;
}

bool can_pass_between(const PitchNode& from, const PitchNode& to) {
    if (from.player_id == to.player_id) {
        return false;
    }
    const int dr = to.rank - from.rank;
    if (dr == 1 || dr == -1) {
        return true;
    }
    if (dr == 0 && std::abs(to.col - from.col) == 1) {
        return true;
    }
    return false;
}

std::vector<std::uint64_t> adjacent_player_ids(const FormationGraph& graph, std::uint64_t from_id) {
    std::vector<std::uint64_t> out;
    const PitchNode* from_node = find_node(graph, from_id);
    if (!from_node) {
        return out;
    }
    for (const PitchNode& n : graph.nodes) {
        if (n.player_id != from_id && can_pass_between(*from_node, n)) {
            out.push_back(n.player_id);
        }
    }
    return out;
}

void assign_risk_factors(FormationGraph& graph, unsigned seed) {
    std::srand(seed);
    for (PitchNode& n : graph.nodes) {
        n.risk_factor = 1 + (std::rand() % 100);
    }
}

void shift_risk_factors(FormationGraph& graph, unsigned seed) {
    std::srand(seed);
    for (PitchNode& n : graph.nodes) {
        const int delta = (std::rand() % 41) - 20;
        int next = n.risk_factor + delta;
        if (next < 1) {
            next = 1;
        }
        if (next > 100) {
            next = 100;
        }
        n.risk_factor = next;
    }
}

AttackSimulationResult simulate_attack_on_goal(FormationGraph& graph, std::uint64_t start_player_id, int max_steps,
                                               unsigned risk_seed) {
    AttackSimulationResult result{};
    result.goal = OpponentGoal{50.0, 6.0};
    result.risk_seed = risk_seed;

    if (max_steps <= 0 || graph.nodes.empty()) {
        result.outcome = "No players on pitch.";
        return result;
    }

    std::uint64_t holder = start_player_id;
    if (!find_node(graph, holder) && !graph.nodes.empty()) {
        for (const PitchNode& n : graph.nodes) {
            if (n.rank == 0) {
                holder = n.player_id;
                break;
            }
        }
        if (!find_node(graph, holder)) {
            holder = graph.nodes.front().player_id;
        }
    }

    const PitchNode* holder_node = find_node(graph, holder);
    if (!holder_node) {
        result.outcome = "Start player not on pitch.";
        return result;
    }

    if (holder_can_shoot(*holder_node, graph, result.goal)) {
        result.scored = true;
        result.outcome = holder_node->name + " is already in range — goal!";
        return result;
    }

    for (int s = 0; s < max_steps; ++s) {
        if (s == 0) {
            assign_risk_factors(graph, risk_seed);
        } else {
            shift_risk_factors(graph, risk_seed + static_cast<unsigned>(s) * 7919u);
        }

        holder_node = find_node(graph, holder);
        if (!holder_node) {
            break;
        }

        if (holder_can_shoot(*holder_node, graph, result.goal)) {
            result.scored = true;
            result.outcome = "Shot by " + holder_node->name + " after " + std::to_string(s) + " pass(es) — goal!";
            break;
        }

        const std::vector<std::uint64_t> neighbors = adjacent_player_ids(graph, holder);
        if (neighbors.empty()) {
            result.outcome = "Attack stalled: no legal pass from " + holder_node->name + ".";
            break;
        }

        const int max_rank = max_rank_on_pitch(graph);

        std::vector<PassCandidateEval> evals;
        MaxHeap heap;

        for (std::uint64_t nid : neighbors) {
            const PitchNode* to_node = find_node(graph, nid);
            if (!to_node) {
                continue;
            }
            PassCandidateEval ev{};
            ev.player_id = nid;
            ev.name = to_node->name;
            ev.risk_factor = to_node->risk_factor;
            ev.effective_risk = effective_pass_target_risk(*to_node, max_rank);
            ev.score = score_pass_candidate(*holder_node, *to_node, max_rank);
            evals.push_back(ev);
            heap.push(ev.score.total, nid);
        }

        std::sort(evals.begin(), evals.end(), [](const PassCandidateEval& a, const PassCandidateEval& b) {
            if (a.score.total != b.score.total) {
                return a.score.total > b.score.total;
            }
            return a.player_id < b.player_id;
        });

        MaxHeap::Entry best{};
        if (!heap.pop_max(best)) {
            result.outcome = "Attack stalled: could not rank pass options.";
            break;
        }

        const PitchNode* to_node = find_node(graph, best.id);
        if (!to_node) {
            break;
        }

        PassCandidateEval chosen{};
        for (const PassCandidateEval& ev : evals) {
            if (ev.player_id == best.id) {
                chosen = ev;
                break;
            }
        }

        PassStepDetail step{};
        step.from_player_id = holder;
        step.to_player_id = best.id;
        step.candidates = evals;
        snapshot_risk_factors(graph, step.risk_snapshot);
        step.reasoning = build_pass_reasoning(*holder_node, *to_node, chosen, evals, s, max_rank);
        result.passes.push_back(std::move(step));

        holder = best.id;
    }

    holder_node = find_node(graph, holder);
    if (!result.scored && holder_node) {
        if (holder_can_shoot(*holder_node, graph, result.goal)) {
            result.scored = true;
            result.outcome = "Final ball to " + holder_node->name + " — shot scores!";
        } else {
            const double d = distance_to_goal(*holder_node, result.goal);
            result.outcome = "Move ended at " + holder_node->name + " (" + std::to_string(static_cast<int>(d)) +
                             " units from goal). No shot.";
        }
    }

    return result;
}
