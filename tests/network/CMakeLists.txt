mu_add_test(
    NAME test_paris_jewel_bank
    SOURCES
        test_paris_jewel_bank.cpp
        test_paris_advanced_stats.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Network/Paris/JewelBankProtocol.cpp
        ${CMAKE_SOURCE_DIR}/src/source/GameLogic/Character/AdvancedStatsSummary.cpp
)

mu_add_test(
    NAME test_server_config_validation
    SOURCES
        test_server_config_validation.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Data/GameConfig/GameConfigValidation.cpp
)

mu_add_test(
    NAME test_byte_order
    SOURCES
        test_byte_order.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_transform_viewport_entry
    SOURCES
        test_transform_viewport_entry.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_connection_library_load
    SOURCES
        test_connection_library_load.cpp
    LINK_LIBS
        MuClient
)

# Control socket tests only exist in builds that enable it. Without it, the
# control_socket_leak test proves that none of its sources were compiled into
# the client (same mechanics as tests/editor's editor_leak).
if(ENABLE_CONTROL_SOCKET)
    mu_add_test(
        NAME test_control_protocol
        SOURCES
            test_control_protocol.cpp
        LINK_LIBS
            MuClient
    )

    mu_add_test(
        NAME test_control_events
        SOURCES
            test_control_events.cpp
        LINK_LIBS
            MuClient
    )
else()
    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/muclient_sources.txt"
        CONTENT "$<JOIN:$<TARGET_PROPERTY:MuClient,SOURCES>,\n>")

    add_test(
        NAME control_socket_leak
        COMMAND ${CMAKE_COMMAND}
            "-DSOURCES_FILE=${CMAKE_CURRENT_BINARY_DIR}/muclient_sources.txt"
            "-DCONTROL_DIR=${PROJECT_SOURCE_DIR}/src/source/App/Control"
            "-DTRANSPORT_FILE=${PROJECT_SOURCE_DIR}/src/source/Core/Platform/LocalSocket.cpp"
            "-DINJECTOR_FILE=${PROJECT_SOURCE_DIR}/src/source/Core/Input/SyntheticInput.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/test_control_leak.cmake")
endif()

mu_add_test(
    NAME test_chat_command_catalog
    SOURCES
        test_chat_command_catalog.cpp
        ${CMAKE_SOURCE_DIR}/src/source/GameLogic/Commands/ChatCommandCatalog.cpp
)

if(DOTNET_EXECUTABLE)
    add_test(
        NAME login_diagnostics_contract
        COMMAND "${CMAKE_COMMAND}"
            "-DMU_LOGIN_SOURCE=${CMAKE_SOURCE_DIR}/ClientLibrary/ConnectionManager.ClientToServer.Custom.cs"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/test_login_diagnostics.cmake"
    )

    add_test(
        NAME dotnet_binding_generator
        COMMAND "${CMAKE_COMMAND}" -E env --unset=Platform
            "${DOTNET_EXECUTABLE}" run
            --project ${CMAKE_CURRENT_SOURCE_DIR}/binding_generator_self_check/BindingGeneratorSelfCheck.csproj
            --configuration Release
            -- ${CMAKE_SOURCE_DIR}/ClientLibrary/GenerateBindingsHeader.xslt
               ${CMAKE_SOURCE_DIR}/ClientLibrary/GenerateFunctions.xslt
               ${CMAKE_SOURCE_DIR}/src/source/Dotnet
    )

    add_test(
        NAME managed_log_callback
        COMMAND "${CMAKE_COMMAND}" -E env --unset=Platform
            "${DOTNET_EXECUTABLE}" run
            --project ${CMAKE_CURRENT_SOURCE_DIR}/managed_log_self_check/ManagedLogSelfCheck.csproj
            --configuration Release
    )
    set_tests_properties(managed_log_callback PROPERTIES TIMEOUT 30)

    add_test(
        NAME outbound_flush_loop
        COMMAND "${CMAKE_COMMAND}" -E env --unset=Platform
            "${DOTNET_EXECUTABLE}" run
            --project ${CMAKE_CURRENT_SOURCE_DIR}/outbound_sender_self_check/OutboundSenderSelfCheck.csproj
            --configuration Release
    )
    set_tests_properties(outbound_flush_loop PROPERTIES TIMEOUT 30)
endif()

mu_add_test(
    NAME test_paris_server_metadata
    SOURCES
        test_paris_server_metadata.cpp
        test_paris_stat_allocation.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Network/Paris/EventTimerState.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Network/Paris/StatServerLimits.cpp
        ${CMAKE_SOURCE_DIR}/src/source/GameLogic/Character/StatAllocationState.cpp
)

mu_add_test(
    NAME custom_monster_viewport_tests
    SOURCES test_custom_monster_viewport.cpp
)

mu_add_test(NAME active_invasion_state_tests SOURCES test_active_invasion_state.cpp ${CMAKE_SOURCE_DIR}/src/source/GameLogic/Events/ActiveInvasionState.cpp)
