cmake_minimum_required(VERSION 3.16)

project(record_dll LANGUAGES CXX)
enable_language(CXX C)

set(CMAKE_PREFIX_PATH $ENV{QTDIR512_64})
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Core Multimedia Network REQUIRED)
             
set(qt_path "C:/Qt/Qt5.12.4/5.12.4/msvc2017_64/include")
include_directories(${qt_path} ${qt_path}/QtCore ../libmp3lame/vector ../libmp3lame)

file(GLOB head_files ./*.h)
source_group("Header Files" FILES ${head_files})

file(GLOB sour_files ./*.cpp)
source_group("Source Files" FILES ${sour_files})

file(GLOB lib_files ../libmp3lame/*.c ../libmp3lame/*.h)
source_group("libmp3lame" FILES ${lib_files})

file(GLOB vector_files ../libmp3lame/vector/*.*)
source_group("libmp3lame\\vector" FILES ${vector_files})

set(src_list
 ${head_files}
 ${sour_files}
 ${lib_files}
 ${vector_files}
)

#设置输出文件存放路径
set(LIBRARY_OUTPUT_PATH ./bin)
#设置DEBUG输出文件名
set(CMAKE_DEBUG_POSTFIX d)
add_library(${PROJECT_NAME} SHARED ${src_list})

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Multimedia Qt5::Network)
#设置编译宏
target_compile_definitions(${PROJECT_NAME} PRIVATE RECORD_DLL_LIB)

