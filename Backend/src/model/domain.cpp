#include "model/domain.h"

#include <cctype>

PreferredSpot preferred_spot_from_string(const std::string& s, bool& ok) {
    ok = true;
    std::string u = s;
    for (char& c : u) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (u == "GK") {
        return PreferredSpot::GK;
    }
    if (u == "RB") {
        return PreferredSpot::RB;
    }
    if (u == "CB") {
        return PreferredSpot::CB;
    }
    if (u == "LB") {
        return PreferredSpot::LB;
    }
    if (u == "CDM") {
        return PreferredSpot::CDM;
    }
    if (u == "CM") {
        return PreferredSpot::CM;
    }
    if (u == "CAM") {
        return PreferredSpot::CAM;
    }
    if (u == "RW") {
        return PreferredSpot::RW;
    }
    if (u == "LW") {
        return PreferredSpot::LW;
    }
    if (u == "ST") {
        return PreferredSpot::ST;
    }
    ok = false;
    return PreferredSpot::CM;
}

std::string preferred_spot_to_string(PreferredSpot p) {
    switch (p) {
        case PreferredSpot::GK:
            return "GK";
        case PreferredSpot::RB:
            return "RB";
        case PreferredSpot::CB:
            return "CB";
        case PreferredSpot::LB:
            return "LB";
        case PreferredSpot::CDM:
            return "CDM";
        case PreferredSpot::CM:
            return "CM";
        case PreferredSpot::CAM:
            return "CAM";
        case PreferredSpot::RW:
            return "RW";
        case PreferredSpot::LW:
            return "LW";
        case PreferredSpot::ST:
            return "ST";
    }
    return "CM";
}
