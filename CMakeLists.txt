cmake_minimum_required(VERSION 3.16)
project(LocalLifePlatform VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra)

# 第三方依赖
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)
set(SQLITE3_C ${THIRD_PARTY_DIR}/sqlite3/sqlite3.c)

# 收集全部业务源文件
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(locallife
  ${APP_SOURCES}
  ${SQLITE3_C}
)
set_source_files_properties(${SQLITE3_C} PROPERTIES LANGUAGE C)

target_include_directories(locallife PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${THIRD_PARTY_DIR}/httplib
  ${THIRD_PARTY_DIR}/json
  ${THIRD_PARTY_DIR}/picosha2
  ${THIRD_PARTY_DIR}/sqlite3
)

if(WIN32)
  # cpp-httplib 在 Windows 上需要 Winsock 与加密库
  target_link_libraries(locallife PRIVATE ws2_32 crypt32)
endif()

# 若存在 sql 目录则将其随构建产物拷贝（供运行时初始化数据库）
if(EXISTS ${CMAKE_SOURCE_DIR}/sql)
  file(COPY ${CMAKE_SOURCE_DIR}/sql DESTINATION ${CMAKE_BINARY_DIR}/data)
endif()

