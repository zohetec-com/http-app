cmake_minimum_required(VERSION 3.5)

project(
    http-app
    VERSION 0.1
)

add_library(http-app SHARED
    httpd-app.cpp)
install(TARGETS http-app)

#add_executable(test-url test-url.cpp)
#target_link_libraries(test-url jgb-core)
