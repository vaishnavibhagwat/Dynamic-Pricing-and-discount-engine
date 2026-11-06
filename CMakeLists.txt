cmake_minimum_required(VERSION 3.20)
project(PricingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MinGW: bundle the C++ runtime into each .exe so it never picks up another
# program's libstdc++-6.dll from PATH (e.g. Git for Windows ships an old one).
if(MINGW)
    add_link_options(-static-libgcc -static-libstdc++ -static)
endif()

include(FetchContent)

# Downloaded archives get the extraction time as their timestamp (CMake 3.24+).
# Set as a policy rather than a FetchContent keyword so CMake 3.20-3.23 still works.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Strict warnings for our own code, in the right syntax for each compiler.
# /utf-8 makes MSVC read the source (which contains ₹) as UTF-8.
if(MSVC)
    set(PRICING_WARNINGS /W4 /permissive- /utf-8)
else()
    set(PRICING_WARNINGS -Wall -Wextra -Wpedantic)
endif()

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# Use the same C runtime as our code on MSVC (harmless elsewhere).
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(json googletest)

add_library(pricing_core STATIC
    src/domain/Money.cpp
    src/domain/Date.cpp
    src/domain/DateRange.cpp
    src/domain/TimeOfDay.cpp
    src/domain/Product.cpp
    src/domain/Clothing.cpp
    src/domain/Perishable.cpp
    src/domain/Customer.cpp
    src/domain/CartItem.cpp
    src/domain/Cart.cpp
    src/rules/DiscountRule.cpp
    src/rules/ItemFilter.cpp
    src/rules/ItemLevelRule.cpp
    src/rules/CartLevelRule.cpp
    src/rules/PercentageDiscount.cpp
    src/rules/QuantityDiscount.cpp
    src/rules/SeasonalDiscount.cpp
    src/rules/ExpiryMarkdown.cpp
    src/rules/MembershipDiscount.cpp
    src/rules/CouponDiscount.cpp
    src/rules/BankOfferDiscount.cpp
    src/rules/RuleFactory.cpp
    src/rules/RuleStage.cpp
    src/rules/ConditionValue.cpp
    src/rules/ConditionField.cpp
    src/rules/ConditionNode.cpp
    src/rules/CompositeRule.cpp
    src/rules/RuleDecorator.cpp
    src/rules/CappedRule.cpp
    src/rules/HappyHourRule.cpp
    src/rules/FirstOrderRule.cpp
    src/language/Token.cpp
    src/language/RuleLexer.cpp
    src/language/RuleParser.cpp
    src/language/RuleScriptLoader.cpp
    src/engine/TraceEntry.cpp
    src/engine/TracePublisher.cpp
    src/engine/PriceTrace.cpp
    src/engine/BestDiscountResolver.cpp
    src/engine/PriorityResolver.cpp
    src/engine/StackingResolver.cpp
    src/engine/GstCalculator.cpp
    src/engine/DiscountCapPolicy.cpp
    src/engine/MarginGuard.cpp
    src/engine/MrpCeilingPolicy.cpp
    src/engine/PricingResult.cpp
    src/engine/PricingEngine.cpp
    src/presentation/InvoicePrinter.cpp
    src/validation/ConflictReport.cpp
    src/validation/RuleCheck.cpp
    src/validation/ExpiredRuleCheck.cpp
    src/validation/UnreachableRuleCheck.cpp
    src/validation/OverlapCheck.cpp
    src/validation/MarginStackCheck.cpp
    src/validation/RuleValidator.cpp
    src/presentation/StreamTraceObserver.cpp
    src/presentation/ConflictReportPrinter.cpp
    src/infrastructure/JsonRecord.cpp
    src/infrastructure/CatalogLoader.cpp
    src/infrastructure/CustomerLoader.cpp
    src/infrastructure/LogSink.cpp
    src/infrastructure/StreamLogSink.cpp
    src/infrastructure/FileLogSink.cpp
    src/infrastructure/Logger.cpp
    src/application/CheckoutService.cpp
    src/application/DiscountOptimizer.cpp
    src/application/WhatIfSimulator.cpp
    src/application/StoreLoader.cpp
    src/application/ConflictPolicies.cpp
    src/application/UpsellAdvisor.cpp
    src/presentation/UpsellPrinter.cpp
    src/presentation/WhatIfPrinter.cpp
    src/presentation/OptimizationPrinter.cpp
    src/presentation/CliApp.cpp
)
target_include_directories(pricing_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pricing_core PUBLIC nlohmann_json::nlohmann_json)
target_compile_options(pricing_core PRIVATE ${PRICING_WARNINGS})

add_executable(pricing_engine src/main.cpp)
target_link_libraries(pricing_engine PRIVATE pricing_core)
target_compile_definitions(pricing_engine PRIVATE DATA_DIR="${PROJECT_SOURCE_DIR}/data")
target_compile_options(pricing_engine PRIVATE ${PRICING_WARNINGS})

enable_testing()
add_subdirectory(tests)
