# UniversalSlashingSimulator - CMake Build Configuration
#
# This project builds a DLL that can be injected into Fortnite
# to provide STW (Save the World) gameserver functionality.
#
# Requirements:
# - Windows SDK
# - MinHook library (user provides)
# - Memcury library (user provides)
# - C++17 compiler

cmake_minimum_required(VERSION 3.16)

project(UniversalSlashingSimulator
    VERSION 0.1.0
    DESCRIPTION "Version-agnostic STW Gameserver Framework"
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Debug definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DUSS_DEBUG)
endif()

# ============================================================================
# Source Files
# ============================================================================

# Core sources
set(CORE_SOURCES
    Core/Logging/Log.cpp
    Core/Memory/Memory.cpp
    Core/Versioning/VersionResolver.cpp
)

set(CORE_HEADERS
    Core/Common.h
    Core/Logging/Log.h
    Core/Memory/Memory.h
    Core/Versioning/VersionInfo.h
    Core/Versioning/VersionResolver.h
    Core/Hooks/HookTypes.h
)

# Engine sources
set(ENGINE_SOURCES
    Engine/CoreTypes/ObjectArray.cpp
    Engine/CoreTypes/NamePool.cpp
    Engine/CoreTypes/OffsetResolver.cpp
    Engine/UObject/UObjectWrapper.cpp
    Engine/Reflection/PropertyIterator.cpp
    Engine/Replication/FastArraySerializer.cpp
    Engine/Events/ProcessEventDispatcher.cpp
    Engine/EngineCore.cpp
)

set(ENGINE_HEADERS
    Engine/CoreTypes/ObjectArray.h
    Engine/CoreTypes/NamePool.h
    Engine/CoreTypes/OffsetResolver.h
    Engine/UObject/UObjectWrapper.h
    Engine/Reflection/PropertyIterator.h
    Engine/Replication/FastArraySerializer.h
    Engine/Events/ProcessEventDispatcher.h
    Engine/EngineCore.h
)

# STW sources
set(STW_SOURCES
    STW/GameMode/STWGameMode.cpp
    STW/Missions/MissionManager.cpp
    STW/Missions/MissionObjective.cpp
    STW/Player/STWPlayerController.cpp
    STW/Player/STWPlayerPawn.cpp
    STW/Inventory/InventoryManager.cpp
    STW/Building/BuildingManager.cpp
)

set(STW_HEADERS
    STW/GameMode/STWGameMode.h
    STW/Missions/MissionManager.h
    STW/Missions/MissionObjective.h
    STW/Missions/MissionTypes.h
    STW/Player/STWPlayerController.h
    STW/Player/STWPlayerPawn.h
    STW/Inventory/InventoryManager.h
    STW/Inventory/InventoryTypes.h
    STW/Building/BuildingManager.h
    STW/Building/BuildingTypes.h
)

# Entry point
set(ENTRY_SOURCES
    Entry/DllMain.cpp
)

# ============================================================================
# Library Target
# ============================================================================

add_library(${PROJECT_NAME} SHARED
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    ${STW_SOURCES}
    ${STW_HEADERS}
    ${ENTRY_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# External Dependencies (User Provides)
# ============================================================================

# MinHook - User must provide
# Add to: external/minhook/include and external/minhook/lib
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/minhook")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/minhook/include
    )
    find_library(MINHOOK_LIB
        NAMES MinHook libMinHook.x64
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/external/minhook/lib
    )
    if(MINHOOK_LIB)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${MINHOOK_LIB})
        message(STATUS "Found MinHook: ${MINHOOK_LIB}")
    endif()
endif()

# Memcury - User must provide (header-only typically)
# Add to: external/memcury/include
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/memcury")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/memcury/include
    )
    message(STATUS "Found Memcury: ${CMAKE_CURRENT_SOURCE_DIR}/external/memcury")
endif()

# Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    psapi
)

# ============================================================================
# Compiler Settings
# ============================================================================

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4             # Warning level 4
        /WX-            # Warnings not as errors (for development)
        /MP             # Multi-processor compilation
        /permissive-    # Strict conformance
        /Zc:__cplusplus # Report correct C++ version
    )

    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /O2
            /Ob2
            /GL
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            /LTCG
        )
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ============================================================================
# Output Settings
# ============================================================================

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "USS"
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "UniversalSlashingSimulator Configuration:")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  Build Type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Output:      ${CMAKE_BINARY_DIR}/bin/USS.dll")
message(STATUS "")
message(STATUS "External Dependencies:")
message(STATUS "  MinHook:     ${MINHOOK_LIB}")
message(STATUS "  Memcury:     (header-only, check external/memcury)")
message(STATUS "")
