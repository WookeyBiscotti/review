cmake_minimum_required(VERSION 3.5)

project(pixelRemover LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(pixelRemover main.cpp)

#https://github.com/roshanpoudyal/Linux/wiki/How-to-link-GDAL-with-Cpp--application-using-CMake
# find system installed GDAL package with predefined CMake variable for finding GDAL
find_package(GDAL REQUIRED)
if(GDAL_FOUND)
    # Specify location of GDAL header files
    include_directories( include ${GDAL_INCLUDE_DIRS})
    # Specify GDAL libraries to link your cpp executable target against
    target_link_libraries(pixelRemover ${GDAL_LIBRARIES})
endif(GDAL_FOUND)
