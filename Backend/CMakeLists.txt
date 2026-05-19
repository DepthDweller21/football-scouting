cmake_minimum_required(VERSION 3.16)
project(BackendCrowServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prevent stale CMake cache from forcing Boost mode in Crow.
set(CROW_USE_BOOST OFF CACHE BOOL "Use standalone Asio for Crow" FORCE)
add_subdirectory(external/Crow)

# MVC-oriented layout: model / service / view / controller / routes
add_executable(server
    main.cpp
    src/model/domain.cpp
    src/model/auth_csv_store.cpp
    src/model/domain_csv_store.cpp
    src/model/scouting_repository.cpp
    src/model/formation_graph.cpp
    src/model/formation_templates.cpp
    src/service/scouting_service.cpp
    src/structures/MaxHeap.cpp
    src/view/json_view.cpp
    src/view/simulation_view.cpp
    src/structures/AVLTree.cpp
    src/controller/auth_controller.cpp
    src/controller/controller_helpers.cpp
    src/controller/health_controller.cpp
    src/controller/players_controller.cpp
    src/controller/schools_controller.cpp
    src/controller/simulation_controller.cpp
    src/controller/teams_controller.cpp
    src/controller/formations_controller.cpp
    src/routes/auth_routes.cpp
    src/routes/formations_routes.cpp
    src/routes/players_routes.cpp
    src/routes/router.cpp
    src/routes/schools_routes.cpp
    src/routes/simulation_routes.cpp
    src/routes/teams_routes.cpp
)
target_include_directories(server PRIVATE src)
target_link_libraries(server PRIVATE Crow::Crow)
