cmake_minimum_required(VERSION 3.16)
# ====================================================================
#  车辆套牌检测系统 — 跨设备自动适配构建
#
#  首次构建（新机器）：
#      cmake -B build -G "MinGW Makefiles"
#      cmake --build build
#      cmake --build build --target run
#
#  拉取代码后如果有旧 build/ 缓存冲突：
#      rm -rf build
#      cmake -B build -G "MinGW Makefiles"
#      cmake --build build
# ====================================================================
project(MyQtApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------
#  自动处理 stale 缓存（旧机器的 build/ 与新源码路径不同时重建）
# ------------------------------------------------------------
if(EXISTS "${CMAKE_BINARY_DIR}/CMakeCache.txt")
    file(STRINGS "${CMAKE_BINARY_DIR}/CMakeCache.txt" _cached_source
         REGEX "^CMAKE_HOME_DIRECTORY:INTERNAL=")
    if(_cached_source)
        string(REGEX REPLACE "^CMAKE_HOME_DIRECTORY:INTERNAL="
               "" _cached_source "${_cached_source}")
        if(NOT "${_cached_source}" STREQUAL "${CMAKE_SOURCE_DIR}")
            message(WARNING
                "检测到旧机器的构建缓存 (源路径: ${_cached_source})，"
                "自动删除并重新配置...")
            file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeCache.txt"
                               "${CMAKE_BINARY_DIR}/CMakeFiles")
            # 重新执行配置
            execute_process(COMMAND "${CMAKE_COMMAND}" "${CMAKE_SOURCE_DIR}"
                            -B "${CMAKE_BINARY_DIR}"
                            -G "${CMAKE_GENERATOR}"
                            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
        endif()
    endif()
endif()

# ------------------------------------------------------------
#  Qt 自动查找（支持多种安装路径）
#  优先级：QT_DIR 环境变量 > 项目同级 qt/ > C:/Qt > D:/Qt > 默认搜索
# ------------------------------------------------------------
function(_find_qt_dir PREFIX RESULT_VAR)
    file(GLOB QT_DIRS LIST_DIRECTORIES true "${PREFIX}/[0-9]*.[0-9]*.[0-9]*")
    set(BEST_DIR "")
    set(BEST_VER "")
    foreach(DIR ${QT_DIRS})
        get_filename_component(VER "${DIR}" NAME)
        if(VER MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
            if(BEST_DIR STREQUAL "" OR VER VERSION_GREATER BEST_VER)
                set(BEST_DIR "${DIR}")
                set(BEST_VER "${VER}")
            endif()
        endif()
    endforeach()
    set(${RESULT_VAR} "${BEST_DIR}" PARENT_SCOPE)
endfunction()

if(DEFINED ENV{QT_DIR})
    set(QT_BASE_DIR "$ENV{QT_DIR}")
    message(STATUS "Qt from env QT_DIR: ${QT_BASE_DIR}")
endif()
if(NOT QT_BASE_DIR)
    _find_qt_dir("${CMAKE_CURRENT_SOURCE_DIR}/../qt" QT_BASE_DIR)
endif()
if(NOT QT_BASE_DIR)
    _find_qt_dir("C:/Qt" QT_BASE_DIR)
endif()
if(NOT QT_BASE_DIR)
    _find_qt_dir("C:/code/QT4/qt" QT_BASE_DIR)
endif()
if(NOT QT_BASE_DIR)
    _find_qt_dir("D:/Qt" QT_BASE_DIR)
endif()
if(NOT QT_BASE_DIR)
    _find_qt_dir("$ENV{HOME}/Qt" QT_BASE_DIR)
endif()

if(QT_BASE_DIR)
    set(CMAKE_PREFIX_PATH "${QT_BASE_DIR}/mingw_64")
    get_filename_component(QT_VER "${QT_BASE_DIR}" NAME)
    message(STATUS "Qt auto-detected: ${QT_BASE_DIR} (version ${QT_VER})")
else()
    message(STATUS "QT_DIR not set, relying on find_package standard search")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network Sql)

# ------------------------------------------------------------
#  运行时输出目录（exe 和资源文件都在此处）
# ------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ------------------------------------------------------------
#  自动收集源文件（排除构建目录和临时文件）
# ------------------------------------------------------------
file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "${CMAKE_BINARY_DIR}/.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/\\.vscode/build/.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "tempCodeRunnerFile\\.")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/build\\..*/.*")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/ModelA/appicon.rc)

# ------------------------------------------------------------
#  头文件搜索路径
# ------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelA
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelA/login
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelB
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelB/Dao
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelB/util
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelB/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelC
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelC/User
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelC/service
    ${CMAKE_CURRENT_SOURCE_DIR}/modelf
    ${CMAKE_CURRENT_SOURCE_DIR}/modele
)

# ------------------------------------------------------------
#  复制运行时资源文件到 exe 所在目录
# ------------------------------------------------------------
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModelA/style.qss
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/style.qss COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModelA/OIP-C.png
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/OIP-C.png COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModelA/appicon.ico
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/appicon.ico COPYONLY)
# AI 模型文件
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/modelf/plate_detect.onnx
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plate_detect.onnx COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/modelf/plate_rec_color.onnx
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plate_rec_color.onnx COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/modelf/vehicle_attribute_model.onnx
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vehicle_attribute_model.onnx COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/modelf/plate_inference.py
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plate_inference.py COPYONLY)


# ------------------------------------------------------------
#  链接 Qt 库
# ------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
)

# OpenCV（可选，用于 AI 车牌识别）
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
    target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
else()
    message(STATUS "OpenCV not found - AI 车牌识别功能将受限")
endif()

# ------------------------------------------------------------
#  部署工具查找
# ------------------------------------------------------------
find_program(WINDEPLOYQT windeployqt
    HINTS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin"
)

# 每次构建后自动部署 Qt DLL
if(WINDEPLOYQT)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${WINDEPLOYQT} "$<TARGET_FILE:${PROJECT_NAME}>"
            --no-translations
            --no-opengl-sw
        COMMENT "Auto-deploying Qt DLLs with windeployqt..."
    )
endif()

# ------------------------------------------------------------
#  自定义目标：run （构建后直接启动）
# ------------------------------------------------------------
add_custom_target(run
    COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.exe"
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    DEPENDS ${PROJECT_NAME}
    COMMENT "启动 ${PROJECT_NAME}"
)

# ------------------------------------------------------------
#  自定义目标：deploy （部署 Qt DLL，可选）
# ------------------------------------------------------------
if(WINDEPLOYQT)
    add_custom_target(deploy
        COMMAND ${WINDEPLOYQT} "./bin/${PROJECT_NAME}.exe"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Deploy Qt DLLs with windeployqt"
    )
endif()

# ------------------------------------------------------------
#  构建成功提示
# ------------------------------------------------------------
message(STATUS "构建完成！运行方式：")
message(STATUS "  cmake --build ${CMAKE_BINARY_DIR} --target run")
