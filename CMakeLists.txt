cmake_minimum_required (VERSION 2.6)
project (OXYGINE_BILLING)

set(OXYGINE_BILLING_INCLUDE_DIRS 
	${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${OXYGINE_BILLING_INCLUDE_DIRS})
add_library(oxygine-billing STATIC 
	src/billing.cpp 
	src/billing.h
	src/sim/BillingSimulator.cpp 
	src/sim/BillingSimulator.h
)

#target_link_libraries(oxyginemagicparticles)

set(OXYGINE_BILLING_INCLUDE_DIRS ${OXYGINE_BILLING_INCLUDE_DIRS} PARENT_SCOPE)
set(OXYGINE_BILLING_LIBS ${OXYGINE_BILLING_LIBS} PARENT_SCOPE)