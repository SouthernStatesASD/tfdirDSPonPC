add_library(
        pathological STATIC # SHARED / MODULE
        src/pathological/pathopub.cpp
        src/pathological/pathosub.cpp
        src/pathological/zhelpers.hpp
		src/pathological/Subscriber.cpp
		src/pathological/Subscriber.h)

target_include_directories(pathological PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

#find cppzmq wrapper, installed by make of cppzmq
find_package(cppzmq)
if(cppzmq_FOUND)
	message(STATUS "ZeroMQ Include Dir: " ${ZeroMQ_INCLUDE_DIR} )
	message(STATUS "C++ZMQ Include Dir: " ${cppzmq_INCLUDE_DIR} )
	include_directories(${cppzmq_INCLUDE_DIR})
	message(STATUS "C++ZMQ Library: " ${cppzmq_LIBRARY} )
	target_link_libraries(pathological ${cppzmq_LIBRARY})
endif()


include_directories(${PROJECT_SOURCE_DIR}/rapidjson/src)

target_link_libraries(pathological PRIVATE cppzmq)
target_link_libraries(pathological PRIVATE configuration)