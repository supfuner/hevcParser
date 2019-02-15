#=======================================================================
#Cmake最低版本要求 
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 
PROJECT(hevcparser) #项目名称 

#允许gdb调试 
set(CMAKE_BUILD_TYPE "Debug") 
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb") 
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall") 

#添加C++11支持及其他选项 
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -g -ftest-coverage -fprofile-arcs -Wno-deprecated") 


#=========================================================================

#添加所有头文件
include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

#将所有的源文件
file(GLOB td_src
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*"
)

#添加动态库so
link_directories(
	#"${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

#=========================================================================
#生成可执行文件 
add_executable(hevcparser ${td_src}) 

#添加链接库 
target_link_libraries(hevcparser 
)







 


