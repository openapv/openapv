include_directories (${PROJECT_SOURCE_DIR}/app)
include_directories (${CMAKE_BINARY_DIR}/include)
set( EXE_ENC oapv_app_enc )
set( EXE_DEC oapv_app_dec )

file (GLOB SRC_ENC "oapv_app_enc.c" )
file (GLOB INC_ENC "*.h" )
file (GLOB SRC_DEC "oapv_app_dec.c" )
file (GLOB INC_DEC "*.h" )

add_executable (${EXE_ENC} ${SRC_ENC} ${INC_ENC} )
add_executable (${EXE_DEC} ${SRC_DEC} ${INC_DEC} )
include_directories( ${EXE_ENC} PUBLIC . .. ../inc ../src ${BASE_SRC_PATH})
include_directories( ${EXE_DEC} PUBLIC . .. ../inc ../src ${BASE_SRC_PATH})

if(OAPV_APP_STATIC_BUILD)
target_link_libraries (${EXE_ENC} oapv)
target_link_libraries (${EXE_DEC} oapv)
else()
target_link_libraries (${EXE_ENC} oapv_dynamic)
target_link_libraries (${EXE_DEC} oapv_dynamic)
endif()

set_property(TARGET ${EXE_ENC} PROPERTY FOLDER "app")
set_property(TARGET ${EXE_DEC} PROPERTY FOLDER "app")

set_target_properties(${EXE_ENC} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(${EXE_DEC} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if( MSVC )
    target_compile_definitions( ${EXE_ENC} PUBLIC _CRT_SECURE_NO_WARNINGS ANY )
    target_compile_definitions( ${EXE_DEC} PUBLIC _CRT_SECURE_NO_WARNINGS ANY )
elseif( UNIX OR MINGW )
    target_compile_definitions( ${EXE_ENC} PUBLIC LINUX ANY )
    target_compile_definitions( ${EXE_DEC} PUBLIC LINUX ANY )
    target_link_libraries (${EXE_ENC} m)
    target_link_libraries (${EXE_DEC} m)
endif()

# Install rules
#
# Install executable to <prefix>/bin
include( GNUInstallDirs)
install( TARGETS ${EXE_ENC} RUNTIME COMPONENT Runtime DESTINATION ${CMAKE_INSTALL_BINDIR})
install( TARGETS ${EXE_DEC} RUNTIME COMPONENT Runtime DESTINATION ${CMAKE_INSTALL_BINDIR})


