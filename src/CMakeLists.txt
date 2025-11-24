FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources} EMBED_FILES ../data/B612Mono.woff2 ../data/index.html ../data/main.js ../data/style.css ../data/logo.bin ../data/logo.bmp)
