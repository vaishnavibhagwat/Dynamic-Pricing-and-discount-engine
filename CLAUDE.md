# Dynamic Pricing & Discount Rule Engine (CS2303 OOP project)

## Goal
C++17 console app. Pricing rules are objects; adding a new offer must not
require changing PricingEngine. Code must clearly demonstrate OOP + SOLID
for a viva.

## Architecture (top to bottom; each layer uses only abstractions below it)
Presentation (CLI, InvoicePrinter, ConflictReportPrinter) -> Application
(CheckoutService, WhatIfSimulator, DiscountOptimizer) -> Rule validation
(RuleValidator, RuleCheck subclasses, ConflictReport; reads loaded rules and
reuses engine guards) -> Engine (PricingEngine, ConflictResolver, guards, GST)
-> Rules (DiscountRule + subclasses, CompositeRule, RuleFactory,
Repository<T>) -> Rule language (RuleParser) -> Domain (Product hierarchy,
Customer, Cart, Money, PricingContext, DateRange) -> Infrastructure (JSON
loaders, Logger, exceptions)

## Rules for code
- C++17, CMake, nlohmann/json, GoogleTest. No other libraries.
- Headers in include/<layer>/, sources in src/<layer>/, tests in tests/.
- Abstract bases use pure virtual functions and virtual destructors.
- Own polymorphic objects with std::unique_ptr. No raw new/delete.
- Money is a class holding paise as int64; never use double for money.
- DiscountRule API: isApplicable(const PricingContext&),
  calculateDiscount(const PricingContext&), name(), priority(), stackable().
- Every rule decision (applied or rejected + reason) goes to PriceTrace.
- const-correct; no `using namespace std` in headers.
- Every new class gets unit tests. Build and run tests before saying done.
- Add a short Doxygen comment on each class saying which OOP/SOLID idea
  it demonstrates.

## Commands
- Build: cmake -S . -B build && cmake --build build
- Test:  ctest --test-dir build --output-on-failure
- Run:   ./build/pricing_engine
