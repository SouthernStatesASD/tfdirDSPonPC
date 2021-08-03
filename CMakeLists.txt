cmake_minimum_required(VERSION 3.9)
project(tfdirDSPonPC)

set(CMAKE_CXX_STANDARD 11)

add_executable(
        tfdirDSPonPC
        main.cpp
        TFDIRContainer.cpp
        TFDIRContainer.h
        Comtrade.cpp
        Comtrade.h
        phasor.cpp
        phasor.h
        data_stream.h
        Constants.h
        P2PComm.h
        P2PComm.cpp
        zhelpers.hpp
        Subscriber.cpp
        Subscriber.h
        Pathological.cpp)

#target_include_directories(dfdir PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

find_package(cppzmq)
if(cppzmq_FOUND)
    message(STATUS "ZeroMQ Include Dir: " ${ZeroMQ_INCLUDE_DIR} )
    message(STATUS "C++ZMQ Include Dir: " ${cppzmq_INCLUDE_DIR} )
    include_directories(${cppzmq_INCLUDE_DIR})
    message(STATUS "C++ZMQ Library: " ${cppzmq_LIBRARY} )
    #    target_link_libraries(sam_coordinator_arm ${cppzmq_LIBRARY})
    #else()
    #message("CAN'T FIND cppzmq!!!")
endif()

include_directories(${PROJECT_SOURCE_DIR}/rapidjson/src)

target_link_libraries(tfdirDSPonPC PRIVATE cppzmq)
#target_link_libraries(sam_coordinator_arm PRIVATE configuration)

#add_subdirectory(dfdir)
#target_link_libraries(sam_coordinator_arm PRIVATE dfdir)
add_subdirectory(configuration)
target_link_libraries(tfdirDSPonPC PRIVATE configuration)
#add_subdirectory(Pathological)
#target_link_libraries(sam_coordinator_arm PRIVATE Pathological)