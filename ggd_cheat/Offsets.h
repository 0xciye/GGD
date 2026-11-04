// C++ / Offsets.h / Native / x64 / Windows 10/11 / Goose Goose Duck (Unity il2cpp v31)

#pragma once
#include <cstdint>
#include <string>

namespace Offsets {
    // -------------------------------------------------------------
    // Base Module & Metadata RVAs (GameAssembly.dll)
    // -------------------------------------------------------------
    constexpr uintptr_t LocalPlayer_TypeInfo       = 0x50B6E58; // Handlers.GameHandlers.PlayerHandlers.LocalPlayer_TypeInfo
    constexpr uintptr_t PlayerController_TypeInfo  = 0x505DFE0; // Handlers.GameHandlers.PlayerHandlers.PlayerController_TypeInfo
    constexpr uintptr_t PlayableEntity_TypeInfo    = 0x505CD60; // Handlers.GameHandlers.PlayerHandlers.PlayableEntity_TypeInfo
    constexpr uintptr_t MapManager_TypeInfo        = 0x50C0200; // MapManager_TypeInfo
    constexpr uintptr_t Camera_TypeInfo            = 0x5089F68; // UnityEngine.Camera_TypeInfo

    // -------------------------------------------------------------
    // Camera & Visual Engine RVAs (GameAssembly.dll)
    // -------------------------------------------------------------
    constexpr uintptr_t LocalPlayer_OverrideOrthoSize = 0xDA9FF0; // LocalPlayer.OverrideOrthographicSize(float)
    constexpr uintptr_t LocalPlayer_ResetOrthoSize    = 0xDABB50; // LocalPlayer.ResetOrthographicSize()
    constexpr uintptr_t Camera_set_orthographicSize   = 0x3B35510; // UnityEngine.Camera.set_orthographicSize(float)
    constexpr uintptr_t Camera_get_orthographicSize   = 0x3B36FB0; // UnityEngine.Camera.get_orthographicSize()

    // -------------------------------------------------------------
    // FogOfWar RVAs & Layout (GameAssembly.dll)
    // -------------------------------------------------------------
    constexpr uintptr_t FogOfWar_Disable              = 0xD438A0; // FogOfWar.Disable()
    constexpr uintptr_t FogOfWar_Enable               = 0xD43B10; // FogOfWar.Enable()
    constexpr uintptr_t FogOfWar_SetBlindMode         = 0xD47F10; // FogOfWar.SetBlindMode(bool)

    namespace FogOfWarLayout {
        constexpr uint32_t shader                     = 0x68;  // GameObject*
        constexpr uint32_t meshRenderer               = 0x70;  // MeshRenderer*
        constexpr uint32_t disabled                   = 0x170; // bool
        constexpr uint32_t isBlind                    = 0x180; // bool
        constexpr uint32_t currentBlindness           = 0x184; // float
        constexpr uint32_t targetBlindness            = 0x188; // float
        constexpr uint32_t mayanLightsSabotaged       = 0x18C; // bool
    }

    // -------------------------------------------------------------
    // EAC / Anti-Cheat Function RVAs (GameAssembly.dll)
    // -------------------------------------------------------------
    constexpr uintptr_t EAC_Dispatcher             = 0x25743C0; // Central Dispatcher RVA
    constexpr uintptr_t EAC_ShowErrorWindow        = 0xB14140;  // CriticalError reporter (Error 19 CHARLIE-03 suppressor)
    constexpr uintptr_t EAC_TryCheckForLateTamper   = 0xB145D0;  // Late tamper verification routine
    constexpr uintptr_t EAC_get_IsAntiCheatOpen    = 0xB15C10;  // Anti-cheat state probe

    // -------------------------------------------------------------
    // Il2CppClass Internal Offsets (x64)
    // -------------------------------------------------------------
    constexpr uint32_t Il2CppClass_StaticFields    = 0xB8;      // klass->static_fields

    // -------------------------------------------------------------
    // LocalPlayer Static & Instance Offsets
    // -------------------------------------------------------------
    namespace LocalPlayer {
        // Static Fields (klass->static_fields + offset)
        constexpr uint32_t DEFAULT_LENS_ORTHO_SIZE = 0x00;      // float
        constexpr uint32_t MIN_LENS_ORTHO_SIZE     = 0x04;      // float
        constexpr uint32_t MAX_LENS_ORTHO_SIZE     = 0x08;      // float
        constexpr uint32_t Instance                = 0x10;      // LocalPlayer*
        constexpr uint32_t Player                  = 0x18;      // PlayerController* (Local Player)

        // Instance Fields (LocalPlayer instance + offset)
        constexpr uint32_t fogOfWar                = 0x20;      // FogOfWar*
        constexpr uint32_t roomUpdater             = 0x30;      // RoomUpdater*
        constexpr uint32_t mainCamera              = 0x78;      // UnityEngine.Camera*
        constexpr uint32_t stateCamera             = 0x80;      // CinemachineStateDrivenCamera*
        constexpr uint32_t scriptableState         = 0x88;      // CinemachineVirtualCamera*
        constexpr uint32_t isInDark                = 0xB8;      // bool
        constexpr uint32_t isInSandstorm           = 0xB9;      // bool
        constexpr uint32_t canSeeGhosts            = 0x148;     // bool
    }

    // -------------------------------------------------------------
    // PlayerController Static Fields
    // -------------------------------------------------------------
    namespace PlayerControllerClass {
        constexpr uint32_t PlayerControllers       = 0x00;      // Dictionary<string, PlayerController>*
        constexpr uint32_t cachedSpectatorInfo     = 0x08;      // System.String*
    }

    // -------------------------------------------------------------
    // PlayableEntity Static Fields
    // -------------------------------------------------------------
    namespace PlayableEntityClass {
        constexpr uint32_t localPlayerSet          = 0x18;      // bool
        constexpr uint32_t PlayableEntities        = 0x28;      // Dictionary<string, PlayableEntity>*
        constexpr uint32_t PlayableEntitiesAtStart = 0x38;      // Dictionary<string, PlayableEntityInfo>*
    }

    // -------------------------------------------------------------
    // PlayableEntityInfo Layout
    // -------------------------------------------------------------
    namespace PlayableEntityInfoLayout {
        constexpr uint32_t playerId                = 0x10;      // System.String*
        constexpr uint32_t playerName              = 0x18;      // System.String*
        constexpr uint32_t playerRole              = 0x20;      // GGDRole*
        constexpr uint32_t isGhost                 = 0x28;      // bool
        constexpr uint32_t isLocal                 = 0x29;      // bool
    }

    // -------------------------------------------------------------
    // CinemachineVirtualCamera Live State Offsets
    // -------------------------------------------------------------
    namespace CinemachineOffsets {
        constexpr uint32_t m_State                 = 0x108;     // CameraState
        constexpr uint32_t OrthoSize               = 0x10C;     // m_State + 0x04 (LensSettings.OrthographicSize)
        constexpr uint32_t RawPosition             = 0x154;     // m_State + 0x4C (Vector3 RawPosition)
        constexpr uint32_t PositionCorrection      = 0x180;     // m_State + 0x78 (Vector3 PositionCorrection)
    }

    // -------------------------------------------------------------
    // Dictionary<TKey, TValue> Layout (x64 il2cpp)
    // -------------------------------------------------------------
    namespace Dictionary {
        constexpr uint32_t Buckets                 = 0x10;      // int[]
        constexpr uint32_t Entries                 = 0x18;      // Il2CppArray of Entry
        constexpr uint32_t Count                   = 0x20;      // int32_t

        // Il2CppArray Header
        constexpr uint32_t ArrayLength             = 0x18;      // uintptr_t max_length
        constexpr uint32_t ArrayData               = 0x20;      // first element starts at 0x20

        // Dictionary.Entry<string, PlayerController>
        constexpr uint32_t EntrySize               = 24;        // 0x18 bytes per element
        constexpr uint32_t EntryKey                = 0x08;      // System.String*
        constexpr uint32_t EntryValue              = 0x10;      // PlayerController*
    }

    // -------------------------------------------------------------
    // PlayerController / PlayableEntity Instance Fields
    // -------------------------------------------------------------
    namespace Entity {
        constexpr uint32_t m_CachedPtr             = 0x10;      // native Component / Transform pointer
        constexpr uint32_t nickname                = 0x90;      // System.String*
        constexpr uint32_t isLocal                 = 0x98;      // bool
        constexpr uint32_t playerRole              = 0xA0;      // GGDRole*
        constexpr uint32_t isPlayerRoleSet         = 0xA8;      // bool
        constexpr uint32_t facingRight             = 0xA9;      // bool
        constexpr uint32_t hasXRayVision           = 0xCC;      // bool
        constexpr uint32_t hasBeenRevealed         = 0xCD;      // bool
        constexpr uint32_t hasBeenSpyRevealed      = 0xCE;      // bool
        constexpr uint32_t teamId                  = 0xDC;      // int32_t
        constexpr uint32_t previousPosition        = 0xE4;      // Vector3 (float x, y, z)
        constexpr uint32_t fogOfWarEnabled         = 0xF4;      // bool
        constexpr uint32_t isGhost                 = 0x178;     // bool
        constexpr uint32_t isSilenced              = 0x17A;     // bool
        constexpr uint32_t isInvisible             = 0x189;     // bool
        constexpr uint32_t oldTransformPosition    = 0x38C;     // Vector3 (float x, y, z)
        constexpr uint32_t actorNumber             = 0x384;     // int32_t
    }

    // -------------------------------------------------------------
    // GGDRole Layout
    // -------------------------------------------------------------
    namespace GGDRole {
        constexpr uint32_t RoleTypeOffset          = 0x10;      // int16_t (enum RoleType)
    }

    // -------------------------------------------------------------
    // System.String Layout (il2cpp)
    // -------------------------------------------------------------
    namespace Il2CppString {
        constexpr uint32_t Length                  = 0x10;      // int32_t
        constexpr uint32_t Chars                   = 0x14;      // wchar_t[]
    }
}

// -------------------------------------------------------------
// Goose Goose Duck Role Definitions
// -------------------------------------------------------------
enum class RoleType : int16_t {
    None = 0,
    Goose = 1,
    Duck = 2,
    Dodo = 3,
    Bounty = 4,
    Mechanic = 5,
    Technician = 6,
    Medium = 7,
    Vigilante = 8,
    Cannibal = 9,
    Morphling = 10,
    Sheriff = 11,
    Silencer = 12,
    Canadian = 13,
    LoverDuck = 14,
    LoverGoose = 15,
    Vulture = 16,
    Professional = 17,
    Spy = 18,
    Mimic = 19,
    Detective = 20,
    Pigeon = 21,
    Birdwatcher = 22,
    Assassin = 23,
    Falcon = 24,
    Hitman = 25,
    Bodyguard = 26,
    Snitch = 27,
    Politician = 28,
    Locksmith = 29,
    Mortician = 30,
    Celebrity = 31,
    Party = 32,
    Demolitionist = 33,
    DuelingDodo = 34,
    GHGoose = 35,
    GHDuck = 36,
    GHBounty = 37,
    DNDDuck = 38,
    DNDFalcon = 39,
    DNDVulture = 40,
    DNDMorphling = 41,
    FPGoose = 42,
    ExploreGoose = 43,
    TTVampire = 44,
    TTPeasant = 45,
    TTThrall = 46
};

enum class TeamClassification {
    Crewmate,  // Goose
    Impostor,  // Duck
    Neutral    // Dodo, Vulture, Falcon, Pigeon, etc.
};

struct RoleInfo {
    const char* name;
    TeamClassification team;
};

inline RoleInfo GetRoleInfo(RoleType role) {
    switch (role) {
        case RoleType::Goose:        return { "Goose", TeamClassification::Crewmate };
        case RoleType::Duck:         return { "Duck", TeamClassification::Impostor };
        case RoleType::Dodo:         return { "Dodo", TeamClassification::Neutral };
        case RoleType::Bounty:       return { "Bounty", TeamClassification::Crewmate };
        case RoleType::Mechanic:     return { "Mechanic", TeamClassification::Crewmate };
        case RoleType::Technician:   return { "Technician", TeamClassification::Crewmate };
        case RoleType::Medium:       return { "Medium", TeamClassification::Crewmate };
        case RoleType::Vigilante:    return { "Vigilante", TeamClassification::Crewmate };
        case RoleType::Cannibal:     return { "Cannibal", TeamClassification::Impostor };
        case RoleType::Morphling:    return { "Morphling", TeamClassification::Impostor };
        case RoleType::Sheriff:      return { "Sheriff", TeamClassification::Crewmate };
        case RoleType::Silencer:     return { "Silencer", TeamClassification::Impostor };
        case RoleType::Canadian:     return { "Canadian", TeamClassification::Crewmate };
        case RoleType::LoverDuck:    return { "Lover Duck", TeamClassification::Impostor };
        case RoleType::LoverGoose:   return { "Lover Goose", TeamClassification::Crewmate };
        case RoleType::Vulture:      return { "Vulture", TeamClassification::Neutral };
        case RoleType::Professional: return { "Professional", TeamClassification::Impostor };
        case RoleType::Spy:          return { "Spy", TeamClassification::Impostor };
        case RoleType::Mimic:        return { "Mimic", TeamClassification::Crewmate };
        case RoleType::Detective:    return { "Detective", TeamClassification::Crewmate };
        case RoleType::Pigeon:       return { "Pigeon", TeamClassification::Neutral };
        case RoleType::Birdwatcher:  return { "Birdwatcher", TeamClassification::Crewmate };
        case RoleType::Assassin:     return { "Assassin", TeamClassification::Impostor };
        case RoleType::Falcon:       return { "Falcon", TeamClassification::Neutral };
        case RoleType::Hitman:       return { "Hitman", TeamClassification::Impostor };
        case RoleType::Bodyguard:    return { "Bodyguard", TeamClassification::Crewmate };
        case RoleType::Snitch:       return { "Snitch", TeamClassification::Crewmate };
        case RoleType::Politician:   return { "Politician", TeamClassification::Crewmate };
        case RoleType::Locksmith:    return { "Locksmith", TeamClassification::Crewmate };
        case RoleType::Mortician:    return { "Mortician", TeamClassification::Crewmate };
        case RoleType::Celebrity:    return { "Celebrity", TeamClassification::Crewmate };
        case RoleType::Party:        return { "Party", TeamClassification::Neutral };
        case RoleType::Demolitionist:return { "Demolitionist", TeamClassification::Impostor };
        case RoleType::DuelingDodo:  return { "Dueling Dodo", TeamClassification::Neutral };
        case RoleType::GHGoose:      return { "GH Goose", TeamClassification::Crewmate };
        case RoleType::GHDuck:       return { "GH Duck", TeamClassification::Impostor };
        case RoleType::GHBounty:     return { "GH Bounty", TeamClassification::Crewmate };
        case RoleType::DNDDuck:      return { "DND Duck", TeamClassification::Impostor };
        case RoleType::DNDFalcon:    return { "DND Falcon", TeamClassification::Neutral };
        case RoleType::DNDVulture:   return { "DND Vulture", TeamClassification::Neutral };
        case RoleType::DNDMorphling: return { "DND Morphling", TeamClassification::Impostor };
        case RoleType::FPGoose:      return { "FP Goose", TeamClassification::Crewmate };
        case RoleType::ExploreGoose: return { "Explore Goose", TeamClassification::Crewmate };
        case RoleType::TTVampire:    return { "TT Vampire", TeamClassification::Impostor };
        case RoleType::TTPeasant:    return { "TT Peasant", TeamClassification::Crewmate };
        case RoleType::TTThrall:     return { "TT Thrall", TeamClassification::Neutral };
        default:                     return { "Unknown", TeamClassification::Crewmate };
    }
}
