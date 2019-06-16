cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()


project(pmcounter
  VERSION 0.0.1
  DESCRIPTION "Particulatew Matter app for PMS7003 sensor"
  LANGUAGES CXX
)

find_package(Qt5 COMPONENTS REQUIRED Core SerialPort)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
# Create code for Qt Resource Files
set(CMAKE_AUTORCC ON)


set(SOURCES pms7003reader.h pms7003reader.cpp)

set(OTHER_FILES README.md LICENSE.txt pms7003.py)

# Core lib

add_library(pmcounterlib OBJECT ${SOURCES} ${OTHER_FILES})

target_include_directories(pmcounterlib
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Mark all warnings as errors
target_compile_options(pmcounterlib PRIVATE -Werror -fPIC)

# Enable LTCG (link time optimization)
option (ltcg "Enable LTCG (link time optimization)" OFF)
if (ltcg)
  set_property(TARGET pmcounterlib PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif (ltcg)

# Use C++ 14 standard
target_compile_features(pmcounterlib PRIVATE cxx_std_14)

# Defines
target_compile_definitions(pmcounterlib PRIVATE QT_USE_QSTRINGBUILDER)

target_link_libraries(pmcounterlib
  Qt5::Core
  Qt5::SerialPort
)

# App exe

add_executable(pmcounterexe main.cpp $<TARGET_OBJECTS:pmcounterlib>)

target_include_directories(pmcounterexe PUBLIC
  $<TARGET_PROPERTY:pmcounterlib,INCLUDE_DIRECTORIES>
)

# Mark all warnings as errors
target_compile_options(pmcounterexe PRIVATE -Werror -fPIC)

# Enable LTCG (link time optimization)
option (ltcg "Enable LTCG (link time optimization)" OFF)
if (ltcg)
  set_property(TARGET pmcounterexe PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif (ltcg)

# Use C++ 14 standard
target_compile_features(pmcounterexe PRIVATE cxx_std_14)

# Defines
target_compile_definitions(pmcounterexe PRIVATE QT_USE_QSTRINGBUILDER)

target_link_libraries(pmcounterexe
  Qt5::Core
  Qt5::SerialPort
)
