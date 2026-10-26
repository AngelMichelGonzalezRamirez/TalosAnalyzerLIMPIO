cmake_minimum_required(VERSION 3.16)
project(TalosAnalyzer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Seleccionar en Qt Creator el kit Qt instalado en el equipo.
# Las traducciones .ts son fuente opcional; no se exige LinguistTools.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(TALOS_HEADERS
    talosanalyzer.h
    talossemantica.h
    talos_matrices_ii.h
    talos_tipos_ii.h
)
set(TALOS_CORE_SOURCES
    talosanalyzer.cpp
    talossemantica.cpp
    talos_modulos.cpp
    talos_semantica_ui.cpp
    talos_traza_ui.cpp
)
set(TALOS_FORMS talosanalyzer.ui)

add_executable(TalosAnalyzer WIN32
    main.cpp
    ${TALOS_CORE_SOURCES}
    ${TALOS_HEADERS}
    ${TALOS_FORMS}
    TalosAnalyzer_en_CA.ts
)
target_link_libraries(TalosAnalyzer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
source_group("Forms" FILES ${TALOS_FORMS})
source_group("Header Files" FILES ${TALOS_HEADERS})
source_group("Source Files" FILES main.cpp ${TALOS_CORE_SOURCES})

# La importacion normal muestra un solo destino: TalosAnalyzer.
option(TALOS_BUILD_TESTS "Compilar las pruebas opcionales de Anexos" OFF)
if(TALOS_BUILD_TESTS)
    enable_testing()
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Anexos/Pruebas" "pruebas")
endif()
