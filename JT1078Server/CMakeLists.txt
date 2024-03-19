# CMakeList.txt: JT1078Server 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。

#set(WORKING_DIRECTORY )
set(EXECUTABLE_OUTPUT_PATH "/home/xingxing/bin")

include_directories("/home/xingxing/FFMPEG/include")
link_directories("/home/xingxing/FFMPEG/lib")

include_directories("/home/xingxing/BOOST/include")

include_directories("/home/xingxing/MUDUO/build/debug-cpp11/include")
link_directories("/home/xingxing/MUDUO/build/debug-cpp11/lib")

link_directories("/home/xingxing/BOOST/lib")

aux_source_directory("JT1078" JT1078DIR)
aux_source_directory("Codec" CODECDIR)
aux_source_directory("RtmpStream" RTMPSTREAMDIR)



add_executable (JT1078Server "JT1078Server.cpp" "JT1078Server.h"  ${JT1078DIR} ${CODECDIR} ${RTMPSTREAMDIR})

target_link_libraries(JT1078Server "avcodec" "avfilter" "avdevice" "avformat" "swscale" "swresample" "postproc" "avutil")
target_link_libraries(JT1078Server muduo_net muduo_base pthread)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET JT1078Server PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
