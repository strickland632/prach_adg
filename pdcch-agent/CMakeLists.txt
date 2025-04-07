file(GLOB_RECURSE AGENT_SOURCES LIST_DIRECTORIES true src/*.cc)
include_directories(hdr)

add_executable(pdcch-agent ${AGENT_SOURCES})
target_link_libraries(pdcch-agent
srsran_phy srsran_phch srsran_common srsran_rf)

install(TARGETS pdcch-agent DESTINATION /usr/local/bin OPTIONAL)
