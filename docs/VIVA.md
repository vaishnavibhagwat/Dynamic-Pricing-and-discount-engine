# Viva preparation: likely questions and short answers

Each answer names the file to open if the examiner asks "show me".

## The project in one breath

> A C++17 console app that prices a cart. Every discount rule is an object behind
> the abstract `DiscountRule` interface, so new offers are new classes and
> `PricingEngine` never changes. Conflicts are settled by swappable policies
> (Strategy), guards keep prices legal and profitable, and every decision is
> recorded so the invoice explains itself. 486 unit tests, all passing.

## OOP basics

**Q: Where is abstraction?**
`DiscountRule` (pure virtual `isApplicable`, `calculateDiscount`, `stage`) and `Product` (pure virtual `gstRate`, `category`). See [include/rules/DiscountRule.hpp](../include/rules/DiscountRule.hpp).

**Q: Where is encapsulation?**
`Money` keeps paise private as `int64_t`; you can only change it through its operators. `Cart` keeps "one line per product" true by merging quantities in `operator+=`. See [include/domain/Money.hpp](../include/domain/Money.hpp).

**Q: Why not `double` for money?**
`0.1 + 0.2 != 0.3` in floating point. Paise in an integer are exact; percentages round once, to the nearest paisa.

**Q: Show runtime polymorphism.**
`PricingEngine::price` loops over `const DiscountRule*` and calls virtual functions. It never knows the concrete class. `GstCalculator` calls the virtual `Product::gstRate()`.

**Q: Compile-time polymorphism?**
`Repository<T>` is one class template that stores products, customers and rules. See [include/rules/Repository.hpp](../include/rules/Repository.hpp).

**Q: Why are copy constructors deleted on `DiscountRule` and `Product`?**
To prevent slicing: copying a derived object into a base object would cut off the derived part.

**Q: Why virtual destructors?**
Objects are deleted through base pointers (`unique_ptr<DiscountRule>`). Without a virtual destructor the derived destructor would not run (undefined behaviour).

**Q: Any raw `new`/`delete`?**
No. Every polymorphic object is owned by `std::unique_ptr` (RAII). Non-owning raw pointers are used only where something else owns the object (e.g. `CartItem` points to a catalog product).

**Q: Operator overloading examples?**
`Money + - * < ==`, `os << money`, `cart += item`, `TimeOfDay < TimeOfDay`.

**Q: Multiple inheritance?**
Only of small interfaces: `CouponDiscount : CartLevelRule, IRedeemable`; `Grocery : Product, Perishable`.

**Q: Exception handling?**
`PricingError` → `ConfigError` → `RuleParseException` (carries line and column). Callers can catch broadly or precisely. See [include/infrastructure/Exceptions.hpp](../include/infrastructure/Exceptions.hpp).

## SOLID

**S:** `StoreLoader` only loads files; `InvoicePrinter` only formats; each `PriceGuard` does one check; `CliApp` only does menus.

**O:** The test `PricingEngine.NewOfferTypeWorksWithoutChangingEngine` defines a new offer *inside the test file* and the unchanged engine prices it. See [tests/test_pricing_engine.cpp](../tests/test_pricing_engine.cpp). A new JSON offer type = one `registerType()` line in `RuleFactory`.

**L:** Any `ConflictResolver` returns one verdict per candidate; any `Product` gives a valid GST rate; a decorated rule behaves like a plain one (it even forwards coupon usage). `ItemLevelRule` marks `isApplicable`/`calculateDiscount` `final` so subclasses cannot break the contract.

**I:** `IItemLevelRule`, `ICartLevelRule`, `IRedeemable`, `Perishable`: small role interfaces. `DiscountRule` has no `recordUse()` that most rules would ignore.

**D:** `PricingEngine` takes a `ConflictResolver&` and `PriceGuard`s from outside; `ExpiryMarkdown` depends on `Perishable`, not `Grocery`; `Logger` writes to `LogSink`s.

## Design patterns

| Pattern | Where | One-line reason |
|---|---|---|
| Strategy | `ConflictResolver` (Stacking / BestDiscount / Priority) | switch the policy at run time |
| Observer | `TracePublisher` → `PriceTrace`, `StreamTraceObserver` | many listeners, engine knows none |
| Template Method | `ItemLevelRule`, `RuleDecorator` | fixed algorithm, subclasses fill hooks |
| Decorator | `CappedRule`, `HappyHourRule`, `FirstOrderRule` | add conditions without new subclasses |
| Factory | `RuleFactory`, `CatalogLoader` | type name in JSON → object |
| Composite + Interpreter | `ConditionNode` tree from `RuleParser` | evaluate text rules |
| Facade | `CheckoutService`, `StoreLoader` | hide wiring behind one call |
| Null Object | `Logger` with no sinks | logging always safe to call |

## Design questions examiners like

**Q: Why are there `dynamic_cast`s if you follow SOLID?**
They are *capability queries* to small interfaces (`IItemLevelRule`, `IRedeemable`, `Perishable`), never to concrete classes. That is how Interface Segregation is used in C++. A new class that implements the interface works with no changes.

**Q: How does a decorator keep coupon limits working?**
`RuleDecorator` implements `IRedeemable` and forwards `recordUse()` to the wrapped rule. Test: `CheckoutService.DecoratedCouponStillCountsItsUses`.

**Q: Why does the rule check (menu 8) show "problems" on your own data?**
They are warnings about worst cases: if *every* stackable offer applied together, some lines would fall below cost + 8%. At checkout `MarginGuard` prevents that. The screen says so in its note line.

**Q: What happens if the data file is broken?**
At start-up you get a clear error naming the rule and the field; on "9 Reload rules" the old rules are kept and a warning is logged.

**Q: Can you sell an expired product?**
No. It is marked EXPIRED in the catalog, refused when adding to the cart, and `CheckoutService` refuses it at checkout too.

**Q: What stops a huge quantity breaking the bill?**
`CartItem::kMaxQuantity = 1000`; checked in the constructor and when merging lines, so the total can never overflow.

**Q: How does the upsell advisor work without knowing the rules?**
For each product it prices three carts through the real engine: current, current + product, product alone. `bonus = price alone − increase in bill`. A positive bonus means the combination unlocked something.

**Q: Why are GST rates 5 / 12 / 18%?**
They are illustrative constants per product class (`Clothing::gstRate()` etc.). Since the September 2025 GST reform the 12% slab is gone and most clothing is 5%. Changing it is one line per class; a real system would read rates from a tax table (listed under future scope).

## Demo checklist (on review day)

1. `cmake --build build` and `ctest --test-dir build` (shows 486 passing).
2. `./build/pricing_engine --date 2026-10-30 --time 19:00` and follow the README's "Viva demo script".
3. Without `--date`, the app uses today; the Festive sale runs until 10 Jan 2027, and the milk (expired 1 Nov) shows as EXPIRED, which is intentional.
