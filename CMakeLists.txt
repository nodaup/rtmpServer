# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("rtpmServer" VERSION 0.0.1)

# --------------------------------------------------------------------------
# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED True)


if(MSVC)
	SET(CMAKE_CXX_FLAGS "/EHsc")  # deal with MSVC Warning C4530, mac
else()
	set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")
endif()
add_definitions(-DNOMINMAX)

include(./.SET_LOCAL_PATH.cmake)



set(USE_VIDEO_ENGINE_IMP7 1)
add_subdirectory("module/videoEngineImp7")
set(MODULE_PATH ${PROJECT_SOURCE_DIR}/module)
set(VIDEO_ENGINE_INCLUDE "${MODULE_PATH}/videoEngineImp7/api" "${MODULE_PATH}/videoEngineImp7/include")
set(VIDEO_ENGINE_LIB "videoEngineImp7")

# audioEngine
OPTION(USE_AUDIO_ENGINE_IMPL5 "use the audio engine imp5 module" ON)
add_subdirectory("module/audioEngineImp5")
set(AUDIO_ENGINE_INCLUDE "${MODULE_PATH}/audioEngineImp5/api" "${MODULE_PATH}/audioEngineImp5/include")
set(AUDIO_ENGINE_LIB "audioEngineImp5")


#-------------------------------------
# ffmpeg lib and include
#-------------------------------------

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY "avcodec")
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY "avformat")
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY "avutil")
find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY "avdevice")
find_path(AVFILTER_INCLUDE_DIR libavfilter/avfilter.h)
find_library(AVFILTER_LIBRARY "avfilter")
find_path(POSTPROC_INCLUDE_DIR libpostproc/postprocess.h)
find_library(POSTPROC_LIBRARY "postproc")
find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY "swresample")
find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY "swscale")

message(AVCODEC_INCLUDE_DIR ===== ${AVCODEC_INCLUDE_DIR})
message(JAVFORMAT_INCLUDE_DIR ===== ${AVFORMAT_INCLUDE_DIR})
message(AVUTIL_INCLUDE_DIR ===== ${AVUTIL_INCLUDE_DIR})
message(AVDEVICE_INCLUDE_DIR ===== ${AVDEVICE_INCLUDE_DIR})
message(AVFILTER_INCLUDE_DIR ===== ${AVFILTER_INCLUDE_DIR})
message(SWRESAMPLE_INCLUDE_DIR ===== ${SWRESAMPLE_INCLUDE_DIR})
message(AVCODEC_LIBRARY ===== ${AVCODEC_LIBRARY})
message(AVFORMAT_LIBRARY ===== ${AVFORMAT_LIBRARY})
message(AVUTIL_LIBRARY ===== ${AVUTIL_LIBRARY})
message(AVDEVICE_LIBRARY ===== ${AVDEVICE_LIBRARY})
message(AVFILTER_LIBRARY ===== ${AVFILTER_LIBRARY})
message(SWRESAMPLE_LIBRARY ===== ${SWRESAMPLE_LIBRARY})
message(SWSCALE_LIBRARY ===== ${SWSCALE_LIBRARY})

#-------------------------------------
# opencv lib and include
#-------------------------------------

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

#JRTP
if (WIN32) # Windows
  find_path(JRTPLIB_INCLUDE_PATH jrtplib3)
  find_library(JRTP_LIBRARY jrtplib)
  message(JRTP_LIBRARY ===== ${JRTP_LIBRARY})
  message(JRTPLIB_INCLUDE_PATH ===== ${JRTPLIB_INCLUDE_PATH})
#message(JTHREAD_LIBRARY ===== ${JTHREAD_LIBRARY})
#message(JTHREAD_INCLUDE_PATH   ===== ${JTHREAD_INCLUDE_PATH})
ELSE() # MAC or linux

  find_library(JRTP_LIBRARY jrtp)
  #find_library(JTHREAD_LIBRARY jthread)
  message(STATUS "JRTP library status:")
  message(STATUS "    jrtplib libraries: ${JRTP_LIBRARY}")
  message(STATUS "    jrtplib include path: ${JRTPLIB_INCLUDE_PATH}")

endif()

# asio
if(WIN32)
    find_path(ASIO_DIR asio)
elseif(UNIX)
    find_path(ASIO_DIR asio)
    find_library(PTHEARD_LIBRARY pthread)
endif()



option(LOG_USE_ASYN "LOG_USE_ASYN" ON)
set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE)
set(LOG_FILE_NAME "appliaction.log")
set(LOG_THREAD_COUNT 1)



configure_file(config.h.in "${PROJECT_SOURCE_DIR}/include/config.h")

if(WIN32)


elseif(UNIX)
	find_library(PTHEARD_LIBRARY pthread)

endif()


aux_source_directory(src CPPLIST)

# 包含子项目。
if(WIN32)
add_executable (${PROJECT_NAME} 
      ${CPPLIST}

      "src/rtpServer.cpp"
      "src/main.cpp"
      #"test/mp42rtmp.cpp"
      "src/rtpParse.cpp"
      "src/ConvertH264.hpp" 
      "src/manager.cpp"
      "src/netManager.cpp"  
      "src/AudioSender.cpp" 
      "src/VideoSender.cpp" 
      "src/mixManager.cpp" 
      #"test/test.cpp"
      )
elseif(UNIX)
add_executable (${PROJECT_NAME} 
      "src/Boot.cpp"
      "src/HttpServer.cpp"
      "src/rtpServer.cpp"
      "src/stick.cpp"
      "src/stickManager.cpp"
      "src/HttpServer.cpp"
	  "src/Utils.cpp"
      "src/SdpParse.cpp" 
      "src/SdpGen.cpp"
      #"test/test_5g.cpp"
	  "test/ConvertH264.hpp"
	  "test/httpUtil.h"
	  "test/HttpUtil.cpp"
      )
endif()


# 配置链接库
if(WIN32)
    target_include_directories( ${PROJECT_NAME}  
		PRIVATE 
			${PROJECT_SOURCE_DIR}/include
            ${JRTPLIB_INCLUDE_PATH}
			${AVCODEC_INCLUDE_DIR}
            ${AVFORMAT_INCLUDE_DIR}
            ${AVUTIL_INCLUDE_DIR}
            ${AVDEVICE_INCLUDE_DIR}
            ${AVFILTER_INCLUDE_DIR}
            ${SWRESAMPLE_INCLUDE_DIR}
            ${SWSCALE_INCLUDE_DIR}
            ${OpenCV_INCLUDE_DIRS}
			${ASIO_DIR}
            ${VIDEO_ENGINE_INCLUDE}
            ${AUDIO_ENGINE_INCLUDE}
	)
	target_link_libraries( ${PROJECT_NAME}  
		PRIVATE 
            ${JRTP_LIBRARY}
			${AVCODEC_LIBRARY}
            ${AVFORMAT_LIBRARY}
            ${AVUTIL_LIBRARY}
            ${AVDEVICE_LIBRARY}
            ${AVFILTER_LIBRARY}
            ${SWRESAMPLE_LIBRARY}
            ${SWSCALE_LIBRARY}
            ${OpenCV_LIBS}
            ${VIDEO_ENGINE_LIB}
            ${AUDIO_ENGINE_LIB}

	)
   
elseif(UNIX)
    target_include_directories( ${PROJECT_NAME}  
		PRIVATE 
			${PROJECT_SOURCE_DIR}/include
            ${JRTPLIB_INCLUDE_PATH}
			${AVCODEC_INCLUDE_DIR}
            ${AVFORMAT_INCLUDE_DIR}
            ${AVUTIL_INCLUDE_DIR}
            ${AVDEVICE_INCLUDE_DIR}
            ${AVFILTER_INCLUDE_DIR}
            ${SWRESAMPLE_INCLUDE_DIR}
            ${SWSCALE_INCLUDE_DIR}
            ${OpenCV_INCLUDE_DIRS}
            ${VIDEO_ENGINE_INCLUDE}
            ${AUDIO_ENGINE_INCLUDE}

	)
	target_link_libraries( ${PROJECT_NAME}  
		PRIVATE 
		    ${PTHEARD_LIBRARY}
            ${JRTP_LIBRARY}
			${AVCODEC_LIBRARY}
            ${AVFORMAT_LIBRARY}
            ${AVUTIL_LIBRARY}
            ${AVDEVICE_LIBRARY}
            ${AVFILTER_LIBRARY}
            ${SWRESAMPLE_LIBRARY}
            ${SWSCALE_LIBRARY}
            ${OpenCV_LIBS}
            ${VIDEO_ENGINE_LIB}
            ${AUDIO_ENGINE_LIB}
			
	)
endif()

# Copy configuration to output directory
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/resources/application.ini"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        )

