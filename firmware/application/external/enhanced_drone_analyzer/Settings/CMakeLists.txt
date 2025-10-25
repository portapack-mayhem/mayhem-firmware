# CMakeLists.txt for Enhanced Drone Analyzer Settings App
# Defines the modular settings application build configuration

# Settings App Sources
set(EDA_SETTINGS_SOURCES
    ui_settings_combined.cpp
    enhanced_drone_analyzer_settings_main.cpp
)

# Settings App Headers
set(EDA_SETTINGS_HEADERS
    ui_settings_combined.hpp
)

# Define settings app target
add_executable(enhanced_drone_analyzer_settings ${EDA_SETTINGS_SOURCES})

# Include directories
target_include_directories(enhanced_drone_analyzer_settings PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
)

# Link necessary libraries
target_link_libraries(enhanced_drone_analyzer_settings PRIVATE
    # TODO: Add required Portapack libraries
    # porta_lib
    # common_lib
)

# Compile definitions
target_compile_definitions(enhanced_drone_analyzer_settings PRIVATE
    EDA_SETTINGS_APP=1
)

# Installation
install(TARGETS enhanced_drone_analyzer_settings
    RUNTIME DESTINATION bin
)

# Build configuration message
message(STATUS "Enhanced Drone Analyzer Settings App configured")

# Print source files for verification
message(STATUS "Settings app sources: ${EDA_SETTINGS_SOURCES}")
message(STATUS "Settings app headers: ${EDA_SETTINGS_HEADERS}")
