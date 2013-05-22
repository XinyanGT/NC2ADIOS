set(NC2ADIOS_SRC_DIR src)
set(NC2ADIOS_INCLUDE_DIR include)

# NC2ADIOS headers
include_directories(${NC2ADIOS_INCLUDE_DIR})

# Target
#add_executable(NC2ADIOS nc2adios.cpp funk.cpp)
file(GLOB NC2ADIOS_SRC_FILES "${NC2ADIOS_SRC_DIR}/*.cpp")
add_executable(NC2ADIOS ${NC2ADIOS_SRC_FILES})

# Link
target_link_libraries(NC2ADIOS ${EXTRA_LIBS})

