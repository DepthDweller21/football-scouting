#include "controller/formations_controller.h"

#include "controller/controller_helpers.h"
#include "model/formation_templates.h"
#include "service/scouting_service.h"
#include "view/json_view.h"

#include <cstdlib>

crow::response formations_list() {
    const auto& templates = all_formation_templates();
    crow::json::wvalue::list items;
    items.resize(templates.size());
    std::size_t idx = 0;
    for (const FormationTemplate& t : templates) {
        crow::json::wvalue w;
        w["id"] = t.id;
        w["label"] = t.label;
        w["formation"] = t.formation;
        crow::json::wvalue::list slots;
        slots.resize(t.slots.size());
        for (std::size_t i = 0; i < t.slots.size(); ++i) {
            crow::json::wvalue s;
            s["assignedSpot"] = preferred_spot_to_string(t.slots[i].spot);
            s["orderIndex"] = t.slots[i].order_index;
            s["xLane"] = t.slots[i].x_lane;
            s["rank"] = t.slots[i].rank;
            slots[i] = std::move(s);
        }
        w["slots"] = std::move(slots);
        items[idx++] = std::move(w);
    }
    crow::json::wvalue data;
    data["formations"] = std::move(items);
    return json_success(data);
}

crow::response formations_suggest_lineup(ScoutingService& svc, const crow::request& req,
                                           const std::string& formation_id) {
    bool use_school = false;
    std::uint64_t school_id = 0;
    if (const char* sid = req.url_params.get("schoolId")) {
        use_school = true;
        school_id = static_cast<std::uint64_t>(std::strtoull(sid, nullptr, 10));
    }
    std::vector<LineupSlotPick> picks;
    std::string err;
    if (!svc.suggest_lineup(formation_id, use_school, school_id, picks, err)) {
        return json_error(400, "LINEUP_FAILED", err);
    }
    crow::json::wvalue::list items;
    items.resize(picks.size());
    for (std::size_t i = 0; i < picks.size(); ++i) {
        crow::json::wvalue w;
        w["playerId"] = picks[i].player_id;
        w["assignedSpot"] = preferred_spot_to_string(picks[i].assigned_spot);
        w["orderIndex"] = picks[i].order_index;
        w["fitScore"] = picks[i].fit_score;
        items[i] = std::move(w);
    }
    crow::json::wvalue data;
    data["formation"] = formation_id;
    data["lineup"] = std::move(items);
    return json_success(data);
}
