

set(AGENT_SRC_DIR src)
set(AGENT_HDR_DIR hdr)
file(GLOB_RECURSE AGENT_SOURCES LIST_DIRECTORIES true "${AGENT_SRC_DIR}/*.cc")

add_executable(pdcch-agent ${AGENT_SOURCES})


set_target_properties(pdcch-agent PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LINKER_LANGUAGE CXX
)


target_include_directories(pdcch-agent PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/${AGENT_HDR_DIR}"
)


target_link_libraries(pdcch-agent PRIVATE

    -Wl,--start-group
    srsran_phy
    srsran_phch
    srsran_common
    support
    srslog
    srsran_rf
    -Wl,--end-group

    pthread

)



install(TARGETS pdcch-agent DESTINATION /usr/local/bin OPTIONAL)
