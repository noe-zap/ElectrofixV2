# TestSimu (ElectrofixV2)

## Project Overview
Coop simulator game inspired by Supermarket Simulator, set in electronics/gaming retail stores (Best Buy, Fnac, Micromania style). Built with Unreal Engine 5.7.

## Architecture
- **Primary module**: `TestSimu` (Runtime, C++)
- **Main content**: `Content/ShopSim/` — blueprints, widgets, characters, maps
- **Code style**: Primarily Blueprint-based with C++ for performance-critical or reusable systems
- **Input system**: Enhanced Input (IA_ actions + IMC_ mapping contexts)
- **Rendering**: Lumen GI, Virtual Shadow Maps, Ray Tracing, Substrate

## Source Structure
```
Source/TestSimu/
  TestSimu.Build.cs          # Module dependencies
  TestSimu.h / .cpp          # Module entry point
  Character/                 # Character-related components
  Tools/                     # Tool actor classes
  Types/                     # Shared structs and enums
  UI/                        # UMG widget classes
```

## Content Structure
```
Content/ShopSim/
  Blueprints/               # All gameplay blueprints
    Area/                   # Spawn and navigation areas
    Components/             # Blueprint components (AI, gameplay)
    Data/                   # Structs, enums, data tables
    Demo/                   # Demo configurations
    Game/                   # Game modes, controllers, save system
    Items/                  # Products and store items
    Misc/                   # Interfaces, function libraries
    Store/                  # Furniture, NPCs, checkout, repair
  Characters/               # Mannequin assets and animations
  Input/                    # IA_ actions and IMC_ mapping contexts
  Maps/                     # Level maps
  Materials/                # Materials
  Meshes/                   # Static meshes
  Widgets/                  # UMG widgets and UI textures
```

## Key Blueprints
- `BP_StoreCharacter` — Player character controller
- `GM_ShopSim` — Main game mode
- `StorekeepComponent` — Core store management component
- `IMC_StoreKeeper` — Primary input mapping context

## Build & Run
- Open `TestSimu.uproject` in UE 5.7
- C++ compilation: Build from IDE or UE editor (Live Coding supported)
- Target: Windows Desktop, DX12

## Multiplayer
- Coop game — replication matters for gameplay systems
- Server-authoritative tool spawning pattern for shared state
