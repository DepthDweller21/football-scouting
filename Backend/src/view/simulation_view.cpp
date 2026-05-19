#include "view/simulation_view.h"

crow::json::wvalue attack_simulation_json(const FormationGraph& graph, const AttackSimulationResult& attack) {
    crow::json::wvalue::list nodes;
    for (const PitchNode& n : graph.nodes) {
        crow::json::wvalue j;
        j["playerId"] = n.player_id;
        j["name"] = n.name;
        j["spot"] = n.spot;
        j["rank"] = n.rank;
        j["col"] = n.col;
        j["lineWidth"] = n.line_width;
        j["xPct"] = n.x_pct;
        j["yPct"] = n.y_pct;
        j["composite"] = n.composite;
        j["riskFactor"] = n.risk_factor;
        nodes.push_back(std::move(j));
    }

    crow::json::wvalue::list edges;
    for (const PitchEdge& e : graph.edges) {
        crow::json::wvalue j;
        j["from"] = e.from;
        j["to"] = e.to;
        edges.push_back(std::move(j));
    }

    crow::json::wvalue::list steps;
    for (const PassStepDetail& p : attack.passes) {
        crow::json::wvalue j;
        j["from"] = p.from_player_id;
        j["to"] = p.to_player_id;
        j["reasoning"] = p.reasoning;

        crow::json::wvalue::list candidates;
        for (const PassCandidateEval& c : p.candidates) {
            crow::json::wvalue cj;
            cj["playerId"] = c.player_id;
            cj["name"] = c.name;
            cj["riskFactor"] = c.risk_factor;
            cj["effectiveRisk"] = c.effective_risk;
            cj["totalScore"] = c.score.total;
            cj["safetyFromRisk"] = c.score.safety_from_risk;
            cj["compositePart"] = c.score.composite_part;
            cj["forwardBonus"] = c.score.forward_bonus;
            cj["goalProximity"] = c.score.goal_proximity;
            cj["selected"] = (c.player_id == p.to_player_id);
            candidates.push_back(std::move(cj));
        }
        j["candidates"] = std::move(candidates);

        crow::json::wvalue::list risk_snap;
        for (const PlayerRiskSnapshot& r : p.risk_snapshot) {
            crow::json::wvalue rj;
            rj["playerId"] = r.player_id;
            rj["riskFactor"] = r.risk_factor;
            risk_snap.push_back(std::move(rj));
        }
        j["riskSnapshot"] = std::move(risk_snap);
        steps.push_back(std::move(j));
    }

    crow::json::wvalue data;
    data["formation"] = graph.formation;
    crow::json::wvalue::list line_counts;
    for (int c : graph.line_counts) {
        line_counts.push_back(c);
    }
    data["lineCounts"] = std::move(line_counts);
    data["nodes"] = std::move(nodes);
    data["edges"] = std::move(edges);
    data["passes"] = std::move(steps);
    data["scored"] = attack.scored;
    data["outcome"] = attack.outcome;
    data["riskSeed"] = attack.risk_seed;
    crow::json::wvalue goal;
    goal["xPct"] = attack.goal.x_pct;
    goal["yPct"] = attack.goal.y_pct;
    data["opponentGoal"] = std::move(goal);
    return data;
}
