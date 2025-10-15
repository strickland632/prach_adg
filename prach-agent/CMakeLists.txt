# prach-agent/CMakeLists.txt

set(AGENT_SRC_DIR src)
set(AGENT_HDR_DIR hdr)

# Don't treat directories as sources
file(GLOB_RECURSE AGENT_SOURCES LIST_DIRECTORIES false "${AGENT_SRC_DIR}/*.cc")

add_executable(prach-agent ${AGENT_SOURCES})

set_target_properties(prach-agent PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

# Add local headers + srsRAN generated headers
target_include_directories(prach-agent PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/${AGENT_HDR_DIR}"
  ${PROJECT_SOURCE_DIR}/lib/include
  ${PROJECT_BINARY_DIR}/lib/include
)

add_executable(prach-agent ${AGENT_SOURCES})

set_target_properties(prach-agent PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LINKER_LANGUAGE CXX
)

target_include_directories(prach-agent PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/${AGENT_HDR_DIR}"
)

target_link_libraries(prach-agent PRIVATE

    -Wl,--start-group
    srsran_phy
    srsran_phch
    srsran_common
    support
    srslog
    srsran_rf
    srsran_common
   
   

    # External deps discovered by top-level
    ${FFT_LIBRARIES}
    ${SEC_LIBRARIES}
    ${LIQUID_LIBRARIES}
    ${ATOMIC_LIBS}
    pthread

    m      # libm (some DSP paths use it)
)

# Optional/add-on libs (only if found/enabled upstream)
if (UHD_FOUND)
  if (TARGET uhd::uhd)
    target_link_libraries(prach-agent PRIVATE uhd::uhd)
  else()
    find_library(UHD_LIB uhd)
    if (UHD_LIB)
      target_link_libraries(prach-agent PRIVATE ${UHD_LIB})
    endif()
  endif()
endif()

if (ZEROMQ_FOUND)
  if (DEFINED ZEROMQ_LIBRARIES)
    target_link_libraries(prach-agent PRIVATE ${ZEROMQ_LIBRARIES})
  else()
    find_library(LIBZMQ zmq)
    if (LIBZMQ)
      target_link_libraries(prach-agent PRIVATE ${LIBZMQ})
    endif()
  endif()
endif()

find_package(yaml-cpp QUIET)
if (yaml-cpp_FOUND)
  target_link_libraries(prach-agent PRIVATE yaml-cpp)
endif()

find_package(fmt QUIET)
if (fmt_FOUND)
  target_link_libraries(prach-agent PRIVATE fmt::fmt)
else()
  find_library(FMT_LIB fmt)
  if (FMT_LIB)
    target_link_libraries(prach-agent PRIVATE ${FMT_LIB})
  endif()
endif()

find_package(spdlog QUIET)
if (spdlog_FOUND)
  target_link_libraries(prach-agent PRIVATE spdlog::spdlog)
else()
  find_library(SPDLOG_LIB spdlog)
  if (SPDLOG_LIB)
    target_link_libraries(prach-agent PRIVATE ${SPDLOG_LIB})
  endif()
endif()

find_library(VOLK_LIB volk)
if (VOLK_LIB)
  target_link_libraries(prach-agent PRIVATE ${VOLK_LIB})
endif()

# Install
install(TARGETS prach-agent DESTINATION /usr/local/bin OPTIONAL)
