cmake_minimum_required (VERSION 2.6)
project (HelloWorldBilling)

add_subdirectory(../../../../oxygine-framework/ oxygine-framework)
add_definitions(${OXYGINE_DEFINITIONS})
include_directories(${OXYGINE_INCLUDE_DIRS})
link_directories(${OXYGINE_LIBRARY_DIRS})

add_subdirectory(../../../../oxygine-billing/ oxygine-billing)
include_directories(${OXYGINE_BILLING_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_executable(HelloWorldBilling ../src/entry_point.cpp ../src/example.cpp  ../src/example.h ../src/test.h ../src/test.cpp)

if (WIN32) #disable console mode for VC++
	set_target_properties(HelloWorldBilling PROPERTIES WIN32_EXECUTABLE TRUE)
endif(WIN32)

target_link_libraries(HelloWorldBilling ${OXYGINE_CORE_LIBS} oxygine-billing)