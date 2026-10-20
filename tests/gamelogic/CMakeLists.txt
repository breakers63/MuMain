mu_add_test(
    NAME test_direction_turning
    SOURCES
        test_direction_turning.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_personal_shop_currency
    SOURCES
        test_personal_shop_currency.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_advanced_stats_summary
    SOURCES
        test_advanced_stats_summary.cpp
        ${CMAKE_SOURCE_DIR}/src/source/GameLogic/Character/AdvancedStatsSummary.cpp
)

mu_add_test(
    NAME test_battle_pass_reward_state
    SOURCES test_battle_pass_reward_state.cpp
)

mu_add_test(
    NAME test_stat_allocation_state
    SOURCES test_stat_allocation_state.cpp
)

mu_add_test(NAME test_market_price SOURCES test_market_price.cpp)
mu_add_test(NAME test_soul_allocation SOURCES test_soul_allocation.cpp)
mu_add_test(NAME test_item_rarity_config SOURCES test_item_rarity_config.cpp LINK_LIBS MuClient)
target_compile_definitions(test_item_rarity_config PRIVATE MU_CLIENT_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src/source")
