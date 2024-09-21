cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
project(Arena CXX)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
message(SRC_FILES="${SRC_FILES}")

add_executable(Arena ${SRC_FILES})

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(SFML_USE_STATIC_STD_LIBS TRUE)
add_subdirectory(external/SFML)
set(TMXLITE_STATIC_LIB TRUE)
set(PROJECT_STATIC_RUNTIME TRUE)
add_subdirectory(external/tmxlite/tmxlite)
add_subdirectory(external/fpm)
set(TMXLITE_LIBRARIES ${PROJECT_SOURCE_DIR}/external/tmxlite)
set(TMXLITE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/external/tmxlite/tmxlite/include)
set(FPM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/external/fpm/include)

target_include_directories(Arena ${SFML_INCLUDE_DIR} PRIVATE ${TMXLITE_INCLUDE_DIR} ${FPM_INCLUDE_DIR})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(Arena SFML::Graphics SFML::Window SFML::Network SFML::System tmxlite -static-libgcc -static-libstdc++)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # For Windows, add -static to avoid errors with winlibpthread
    target_link_libraries(Arena SFML::Graphics SFML::Window SFML::Network SFML::System tmxlite -static-libgcc -static-libstdc++ -static)
    add_custom_command(
            TARGET Arena
            COMMENT "Copy OpenAL DLL"
            PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:Arena>
            VERBATIM)
endif()
target_compile_features(Arena PRIVATE cxx_std_17)

install(TARGETS Arena DESTINATION bin)
