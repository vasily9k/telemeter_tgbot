FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
idf_component_register(SRCS ${app_sources})
set(COMPONENT_REQUIRES "json")

target_add_binary_data(${COMPONENT_TARGET} ../certificates/isrg_root_x1.pem TEXT)
target_add_binary_data(${COMPONENT_TARGET} ../certificates/digi_cert.pem TEXT)
target_add_binary_data(${COMPONENT_TARGET} ../certificates/api_telegram_org.pem TEXT)
