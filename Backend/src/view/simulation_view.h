#pragma once

#include "model/formation_graph.h"

#include <crow.h>

crow::json::wvalue attack_simulation_json(const FormationGraph& graph, const AttackSimulationResult& attack);
