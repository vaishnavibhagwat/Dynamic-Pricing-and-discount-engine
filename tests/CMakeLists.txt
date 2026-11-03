add_executable(unit_tests
    test_money.cpp
    test_date.cpp
    test_product.cpp
    test_customer.cpp
    test_cart.cpp
    test_pricing_context.cpp
    test_repository.cpp
    test_rule_fixtures.hpp
    test_item_filter.cpp
    test_percentage_discount.cpp
    test_quantity_discount.cpp
    test_seasonal_discount.cpp
    test_expiry_markdown.cpp
    test_membership_discount.cpp
    test_coupon_discount.cpp
    test_bank_offer_discount.cpp
    test_rule_factory.cpp
    test_rule_stage.cpp
    test_trace_entry.cpp
    test_trace_publisher.cpp
    test_price_trace.cpp
    test_gst_calculator.cpp
    test_conflict_resolvers.cpp
    test_price_guards.cpp
    test_pricing_engine.cpp
    test_invoice_printer.cpp
    test_stream_trace_observer.cpp
    test_date_range.cpp
    test_conflict_report.cpp
    test_rule_checks.cpp
    test_rule_validator.cpp
    test_conflict_report_printer.cpp
    test_condition_value.cpp
    test_condition_field.cpp
    test_condition_node.cpp
    test_composite_rule.cpp
    test_rule_lexer.cpp
    test_rule_parser.cpp
    test_rule_script_loader.cpp
    test_json_record.cpp
    test_catalog_loader.cpp
    test_customer_loader.cpp
    test_checkout_service.cpp
    test_discount_optimizer.cpp
    test_what_if_simulator.cpp
    test_what_if_printer.cpp
    test_optimization_printer.cpp
    test_cli_app.cpp
    test_store_loader.cpp
    test_conflict_policies.cpp
    test_pricing_result.cpp
    test_exceptions.cpp
    test_time_of_day.cpp
    test_rule_decorator.cpp
    test_capped_rule.cpp
    test_happy_hour_rule.cpp
    test_first_order_rule.cpp
    test_upsell_advisor.cpp
    test_upsell_printer.cpp
    test_logger.cpp
    test_stream_log_sink.cpp
    test_file_log_sink.cpp
)
target_link_libraries(unit_tests PRIVATE pricing_core GTest::gtest_main)
# Writable folder for tests that need to change a data file.
set(TEST_SCRATCH_DIR "${CMAKE_CURRENT_BINARY_DIR}/scratch")
file(MAKE_DIRECTORY "${TEST_SCRATCH_DIR}/cli_data")

target_compile_definitions(unit_tests PRIVATE
    DATA_DIR="${PROJECT_SOURCE_DIR}/data"
    TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data"
    TEST_SCRATCH_DIR="${TEST_SCRATCH_DIR}")
target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(unit_tests)
