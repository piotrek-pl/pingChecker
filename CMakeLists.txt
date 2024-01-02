cmake_minimum_required(VERSION 3.5)

project(pingChecker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########################   BOOST ##################################
set(BOOST_ROOT "C:/Boost/boost_1_83_0")

# Znajdź Boost
find_package(Boost REQUIRED)

# Dołącz katalogi nagłówków Boost
if(Boost_FOUND)
    include_directories(pingChecker PRIVATE ${Boost_INCLUDE_DIRS})
endif()
###################################################################

add_executable(pingChecker main.cpp)

include(GNUInstallDirs)
install(TARGETS pingChecker
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)



