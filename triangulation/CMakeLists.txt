cmake_minimum_required(VERSION 3.10)
project(triangulation_service)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск необходимых пакетов
find_package(Threads REQUIRED)

# Предполагаем, что используется hiredis для Redis
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)

if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
    message(WARNING "hiredis not found. You may need to install libhiredis-dev")
endif()

# Включение директорий
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${HIREDIS_INCLUDE_DIR}
)

# Список исходных файлов
set(SOURCES
    src/RedisPublisher.cpp
    src/RedisSubscriber.cpp
    src/ThreadPool.cpp
    src/TriangulationTask.cpp
    src/Triangulator.cpp
    main.cpp
)

# Создание исполняемого файла
add_executable(triangulation_service ${SOURCES})

# Подключение заголовочных файлов
target_include_directories(triangulation_service PRIVATE include)

# Подключение библиотек
target_link_libraries(triangulation_service
    Threads::Threads
    ${HIREDIS_LIBRARY}
)

# Установка целевого файла
install(TARGETS triangulation_service DESTINATION bin)
