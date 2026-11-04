// RoleType names from dump/dump.cs, TypeDefIndex 1537.
#pragma once
#include "Offsets.h"
#include <cstring>

inline const char* GetInternalRoleName(RoleType role) {
    static constexpr const char* names[] = {
        "None", // 0
        "Goose", // 1
        "Duck", // 2
        "Dodo", // 3
        "Bounty", // 4
        "Mechanic", // 5
        "Technician", // 6
        "Medium", // 7
        "Vigilante", // 8
        "Cannibal", // 9
        "Morphling", // 10
        "Sheriff", // 11
        "Silencer", // 12
        "Canadian", // 13
        "LoverDuck", // 14
        "LoverGoose", // 15
        "Vulture", // 16
        "Professional", // 17
        "Spy", // 18
        "Mimic", // 19
        "Detective", // 20
        "Pigeon", // 21
        "Birdwatcher", // 22
        "Assassin", // 23
        "Falcon", // 24
        "Hitman", // 25
        "Bodyguard", // 26
        "Snitch", // 27
        "Politician", // 28
        "Locksmith", // 29
        "Mortician", // 30
        "Celebrity", // 31
        "Party", // 32
        "Demolitionist", // 33
        "DuelingDodo", // 34
        "GHGoose", // 35
        "GHDuck", // 36
        "GHBounty", // 37
        "DNDDuck", // 38
        "DNDFalcon", // 39
        "DNDVulture", // 40
        "DNDMorphling", // 41
        "FPGoose", // 42
        "ExploreGoose", // 43
        "TTVampire", // 44
        "TTPeasant", // 45
        "TTThrall", // 46
        "Spectator", // 47
        "IdentityThief", // 48
        "Adventurer", // 49
        "Avenger", // 50
        "Ninja", // 51
        "Undertaker", // 52
        "Snoop", // 53
        "Esper", // 54
        "Invisibility", // 55
        "Astral", // 56
        "Pelican", // 57
        "TTEThrall", // 58
        "TTMummy", // 59
        "SerialKiller", // 60
        "Engineer", // 61
        "Warlock", // 62
        "StreetUrchin", // 63
        "Tracker", // 64
        "EsperDuck", // 65
        "Stalker", // 66
        "Preacher", // 67
        "BHCInquisitor", // 68
        "BHCSaint", // 69
        "BHCHighPriest", // 70
        "BHCDemonHunter", // 71
        "BHCInitiate", // 72
        "BHCSeamstress", // 73
        "BHCCrow", // 74
        "BHCSinEater", // 75
        "TFGoose", // 76
        "TLCChicken", // 77
        "TLCBodyguard", // 78
        "TLCIdentityThief", // 79
        "TLCUndertaker", // 80
        "TLCCamoDuck", // 81
        "Cupid", // 82
        "Survivalist", // 83
        "Carrier", // 84
        "Parasite", // 85
        "Drone", // 86
        "Scientist", // 87
        "HNSRole", // 88
        "HNSOwl", // 89
        "HNSSpotter", // 90
        "HNSSniper", // 91
        "Lobbyist", // 92
        "LostDuckling", // 93
        "FortuneTeller", // 94
        "Mime", // 95
        "Raven", // 96
        "Rabbit", // 97
        "LucidDreamer", // 98
        "Clown", // 99
        "Soldier", // 100
        "Coroner", // 101
        "Sensor", // 102
        "Looter", // 103
        "Sniper", // 104
        "Delusional", // 105
        "Hawk", // 106
        "AI", // 107
        "TTDrTurducken", // 108
        "TTTurduckensMonster", // 109
        "WitchDoctor", // 110
        "Cuckoo", // 111
        "Swordsman", // 112
        "Magpie", // 113
        "TTWereduck", // 114
    };
    const int value = static_cast<int>(role);
    if (value <= 0 || value >= static_cast<int>(sizeof(names) / sizeof(names[0])))
        return "Unknown";
    return names[value];
}

inline RoleInfo GetInternalRoleInfo(RoleType role) {
    if (role == RoleType::Party) return {"Party", TeamClassification::Impostor};
    auto info = GetRoleInfo(role);
    if (strcmp(info.name, "Unknown") != 0) return info;
    // Modern Classic roles: Goose/Duck inheritance and neutral role definitions in dump.cs.
    switch (static_cast<int>(role)) {
        case 49: case 50: case 53: case 54: case 56: case 61: case 63: case 64: case 66:
        case 83: case 87: case 92: case 94: case 98: case 100: case 101: case 102:
            return {GetInternalRoleName(role), TeamClassification::Crewmate};
        case 48: case 51: case 52: case 55: case 60: case 62: case 65: case 67: case 82:
        case 84: case 85: case 86: case 93: case 95: case 97: case 99: case 103: case 104:
        case 105: case 110: case 112:
            return {GetInternalRoleName(role), TeamClassification::Impostor};
        case 57: case 96: case 106: case 111: case 113:
            return {GetInternalRoleName(role), TeamClassification::Neutral};
        default: return info;
    }
}
