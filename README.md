# Dynamic Pricing & Discount Rule Engine

CS2303 Object-Oriented Programming project. A C++17 console app that prices a
shopping cart using **pricing rules as objects**: percentage, quantity,
seasonal, expiry, membership, coupon and bank offers, plus rules written in a
small text language. Adding a new kind of offer never requires changing
`PricingEngine`.

Every decision (rule applied or rejected, and why; guard passed or adjusted) is
recorded in a price trace and can be shown after checkout.

## Features

- GST invoice with per-line item discounts, order-level offers and GST
  (5 / 12 / 18%), with all money held as integer paise.
- Three conflict policies: **Stacking**, **BestDiscount** and **Priority**,
  switchable at checkout.
- Safety guards: discount cap (60%), margin floor (8%) and an MRP ceiling.
- **What-if** table: the same cart priced under every policy, side by side.
- **Conflict report**: finds expired, unreachable, overlapping and
  margin-breaking rules before they reach a customer.
- **Best offer combination**: searches offer sets for the lowest total.
- Rules from JSON (`data/rules.json`) and from a text rule language
  (`data/rules.txt`), reloadable while the app runs.
- **Rule decorators**: wrap *any* offer as "happy hour 18:00–21:00",
  "max ₹500 off" or "first order only" from JSON, with no new offer classes.
  Decorators can be stacked.
- **Session log**: `--log FILE` records data loads, every order placed
  (customer, policy, total, offers used), rule reloads and errors.
- **Smart upsell advisor**: suggests the item that makes your cart cheaper,
  e.g. "add 1 Kurta: alone it costs ₹1,120, but your bill goes up by only
  ₹56, because it unlocks buy-3 and FESTIVE500". It asks the real engine
  for prices, so it understands every offer type, including future ones.

## Innovations

### 1. Rule decorators (Decorator pattern)

Real shops attach the same restrictions to many different offers: a time
window, a maximum saving, new customers only. Writing a subclass per
combination (`HappyHourCappedCoupon`, ...) would explode. Instead,
[RuleDecorator](include/rules/RuleDecorator.hpp) *is a* `DiscountRule`
and *has a* `DiscountRule`. It passes every call through to the offer it
wraps and adds one rule through three hooks (`allows`, `blockedReason`,
`limit`):

| Decorator | JSON | Effect |
|-----------|------|--------|
| [HappyHourRule](include/rules/HappyHourRule.hpp) | `{"type": "happy_hour", "from": "18:00", "to": "21:00"}` | Only during the window (may cross midnight) |
| [CappedRule](include/rules/CappedRule.hpp) | `{"type": "cap", "max": 500}` | At most ₹500 per line (item offers) or per order (coupon, bank) |
| [FirstOrderRule](include/rules/FirstOrderRule.hpp) | `{"type": "first_order"}` | Only if the customer has never checked out |

```json
{ "id": "HAPPY_HOUR_EARBUDS", "type": "percentage", "percent": 25,
  "product_ids": ["E2"], "name": "Evening happy hour: 25% off Wireless Earbuds",
  "priority": 65,
  "decorators": [ { "type": "happy_hour", "from": "18:00", "to": "21:00" },
                  { "type": "cap", "max": 300 } ] }
```

The engine, the resolvers, the validator and checkout never learn that
decorators exist. A decorated coupon still counts its uses, because the
decorator forwards `IRedeemable::recordUse()`. Each rejection is explained
in the trace, e.g. *"only between 18:00 and 21:00 (ordered at 14:00)"* or
*"first order only; Asha Rao has placed 1 order"*. A new decorator type is
one class plus one `registerDecorator()` line in `RuleFactory`.

### 2. Smart upsell advisor

[UpsellAdvisor](include/application/UpsellAdvisor.hpp) (menu **11**) answers
"what should I add to pay less overall?". For each catalog product it prices
three carts through `PricingEngine`: the current cart, the cart plus that
product, and the product alone.

```
bonus = (price of the product alone) - (how much the bill goes up when it is added)
```

A positive bonus is money saved only because of the combination: a coupon
threshold reached, a quantity break, a bank offer's minimum, a cart-total
text rule. The advisor never looks at rule types or thresholds, so it works
for offers written next year with no changes (Open/Closed). It only quotes,
so coupon usage is never touched.

```
  1. Add 1 x Cotton Kurta (C1)
     Bought alone it costs ₹1,120.00; added to this cart your bill
     goes up by only ₹56.00 - you save ₹1,064.00 extra.
     Unlocks: Buy 3 or more of a kurta/shirt, get 15% off, FESTIVE500: ...
```

## Requirements

| Tool | Version |
|------|---------|
| C++ compiler | C++17 (GCC 9+, Clang 10+, or MSVC 2019+) |
| CMake | 3.20 or newer |
| Internet on first configure | CMake downloads nlohmann/json 3.11.3 and GoogleTest 1.15.2 |
| Doxygen (optional) | for the API docs; Graphviz is optional for nicer graphs |
| PlantUML (optional) | to render the diagrams in `docs/` |

No other libraries are used.

## Build, test and run

```sh
# configure + build
cmake -S . -B build
cmake --build build

# run all unit tests (GoogleTest, via CTest)
ctest --test-dir build --output-on-failure

# run the app
./build/pricing_engine            # Linux / macOS / Git Bash
.\build\pricing_engine.exe        # Windows PowerShell

# options: pick the data folder, the order date and the clock time,
# and keep a session log
./build/pricing_engine --date 2026-10-30 --time 19:00 --log session.log
```

A session log looks like this:

```
[INFO] Loaded 8 products, 4 customers and 17 rules from .../data
[INFO] Order placed by CU1 (Asha Rao), policy Stacking: 1 line, total ₹15,292.80, offers: DIWALI_ELECTRONICS
[INFO] Reloaded 17 rules
[INFO] Session ended
```

By default the app uses today's date and the current time. `--time` is handy
for demonstrating happy-hour offers at any hour.

On Windows with MSYS2 / MinGW, the build links the C++ runtime statically,
so the `.exe` works from any terminal. Without this, Git Bash's own older
`libstdc++-6.dll` can be picked up and the tests crash or misbehave.

The app reads its data from the project's `data/` folder (the path is built in
at compile time).

### Using the app

```
  1  View catalog              6  View price trace
  2  Build cart                7  Run what-if
  3  Pick customer             8  View conflict report
  4  Apply coupon and bank     9  Reload rules
  5  Checkout                 10  Find best offer combination
  0  Exit                     11  Smart upsell suggestions
```

A quick demo: `2` → `a` → `E1` → `1` → `b` (add a phone), `3` → `CU42` (Gold
customer), `4` → `FESTIVE500` → `HDFC`, `5` → `1` (checkout with Stacking),
then `6` to see why each offer was or was not applied, and `7` to compare
policies.

The app reads commands from standard input, so a session can be scripted:

```sh
printf '2\na\nE1\n1\nb\n5\n1\n6\n0\n' | ./build/pricing_engine
```

### Viva demo script (about 2 minutes)

Start with `./build/pricing_engine --date 2026-10-30 --time 19:00`.

1. **Upsell:** `2` → `a` → `C1` → `2` → `b` (two kurtas), `4` → `FESTIVE500`
   → Enter, then `11`. The advisor shows that a third kurta adds only ₹56 to
   the bill.
2. **First order only:** customer Asha (CU1) has never ordered. `4` →
   `WELCOME200` → Enter, then check out twice (`5` → `1`, `5` → `1`) and
   press `6`. The first invoice has *[first order only] -₹200*; the second
   trace says *"first order only; Asha Rao has placed 1 order"*.
3. **Happy hour:** `2` → `c` → `a` → `E2` → `1` → `b`, then `5` → `1`. The
   invoice shows *[happy hour 18:00-21:00] [max ₹300.00 off]*. Restart with
   `--time 14:00`, check out the same cart and press `6`: the trace says
   *"only between 18:00 and 21:00 (ordered at 14:00)"*.
4. **Open/Closed proof:** show
   [tests/test_rule_decorator.cpp](tests/test_rule_decorator.cpp) and
   `PricingEngine.NewOfferTypeWorksWithoutChangingEngine`.

### API documentation and diagrams

```sh
doxygen Doxyfile                      # -> docs/api/html/index.html
plantuml docs/*.puml                  # -> PNGs next to each .puml
```

| Diagram | File |
|---------|------|
| Class diagram (all layers) | [docs/class-diagram.puml](docs/class-diagram.puml) |
| Sequence: checkout (menu 5) | [docs/sequence-checkout.puml](docs/sequence-checkout.puml) |
| Sequence: start-up and rule loading | [docs/sequence-load-rules.puml](docs/sequence-load-rules.puml) |
| Sequence: smart upsell (menu 11) | [docs/sequence-upsell.puml](docs/sequence-upsell.puml) |

The PlantUML extension for VS Code previews `.puml` files directly (Alt+D).
It renders locally only when Java is installed. Without Java, set the
extension's `plantuml.render` setting to `PlantUMLServer`, or paste a file
into the online editor at plantuml.com. If Graphviz is not installed, add
`-Playout=smetana` when running `plantuml` on the class diagram.

## Architecture

Each layer uses only the layers below it.

```
Presentation    CliApp, InvoicePrinter, ConflictReportPrinter, WhatIfPrinter,
                OptimizationPrinter, UpsellPrinter, StreamTraceObserver
Application     CheckoutService, WhatIfSimulator, DiscountOptimizer,
                UpsellAdvisor, StoreLoader, standardConflictPolicies(),
                GuardFactory
Rule validation RuleValidator, RuleCheck + 4 checks, ConflictReport
Engine          PricingEngine, ConflictResolver + 3 policies, PriceGuard + 3
                guards, TracePublisher/TraceObserver, PriceTrace, GstCalculator
Rules           DiscountRule + subclasses, RuleDecorator + 3 decorators,
                CompositeRule, ConditionNode tree, RuleFactory, Repository<T>
Rule language   RuleLexer, RuleParser, RuleScriptLoader
Domain          Product hierarchy, Perishable, Customer, Cart, CartItem,
                Money, Date, DateRange, TimeOfDay, PricingContext
Infrastructure  CatalogLoader, CustomerLoader, JsonRecord, Logger + LogSink
                (stream, file), exception hierarchy
```

```
include/<layer>/   headers        src/<layer>/   sources
tests/             unit tests     data/          catalog, customers, rules
docs/              UML diagrams   Doxyfile       API doc config
```

### Pricing pipeline

1. Item offers, by stage: Expiry → Seasonal → Percentage → Quantity → Membership
2. Price guards on each line (cap, margin floor, MRP ceiling)
3. Order offers: Coupon → Bank offer
4. Order offers are split across lines in proportion
5. GST per line, at the product's own rate

The chosen `ConflictResolver` decides which competing offers are given at
steps 1 and 3.

### Adding a new offer type

1. Write `class MyOffer final : public ItemLevelRule` (or `CartLevelRule`)
   and implement its hooks.
2. Register it in `RuleFactory` with one `registerType("my_offer", ...)` line.
3. Add a test.

`PricingEngine`, the resolvers and the CLI do not change. The test
`PricingEngine.NewOfferTypeWorksWithoutChangingEngine` in
[tests/test_pricing_engine.cpp](tests/test_pricing_engine.cpp) proves this
with an offer type defined only in the test file.

### Text rule language

```
RULE BIG_BASKET: IF (category == Furniture OR category == Electronics)
                 AND cart_total >= 20000 THEN 3% OFF PRIORITY 10 NAME "Big basket bonus"
```

`RuleLexer` turns the text into tokens. `RuleParser` builds a `ConditionNode`
tree (And / Or / Not / Comparison) inside a `CompositeRule`. The grammar and
field list are documented in
[include/language/RuleParser.hpp](include/language/RuleParser.hpp).

## OOP and SOLID concepts: where to find them

| Concept | Where it is shown | What to point at |
|---------|-------------------|------------------|
| **Encapsulation** | [include/domain/Money.hpp](include/domain/Money.hpp) | Paise kept private as `int64`. No `double` anywhere, and all arithmetic goes through the class. |
| Encapsulation (read-only state) | [include/engine/PricingResult.hpp](include/engine/PricingResult.hpp) | Totals are computed from lines and never stored twice. |
| **Abstraction** (pure virtual) | [include/rules/DiscountRule.hpp](include/rules/DiscountRule.hpp) | `isApplicable()`, `calculateDiscount()`, `stage()` |
| Abstract class + virtual destructor | [include/domain/Product.hpp](include/domain/Product.hpp) | `gstRate()`, `category()` pure virtual; `virtual ~Product()` |
| **Inheritance** | [include/domain/Electronics.hpp](include/domain/Electronics.hpp), [Clothing.hpp](include/domain/Clothing.hpp), [Furniture.hpp](include/domain/Furniture.hpp), [Grocery.hpp](include/domain/Grocery.hpp) | Four `Product` subclasses, each with its own GST rate |
| Multi-level inheritance | [include/rules/ItemLevelRule.hpp](include/rules/ItemLevelRule.hpp) → [PercentageDiscount.hpp](include/rules/PercentageDiscount.hpp) | `DiscountRule` → `ItemLevelRule` → concrete offer |
| Multiple inheritance of interfaces | [include/rules/CouponDiscount.hpp](include/rules/CouponDiscount.hpp), [include/domain/Grocery.hpp](include/domain/Grocery.hpp) | `CartLevelRule + IRedeemable`; `Product + Perishable` |
| **Runtime polymorphism** | [src/engine/PricingEngine.cpp](src/engine/PricingEngine.cpp), [src/engine/GstCalculator.cpp](src/engine/GstCalculator.cpp) | The engine calls rules only through base-class pointers. GST calls virtual `Product::gstRate()`. |
| Compile-time polymorphism (templates) | [include/rules/Repository.hpp](include/rules/Repository.hpp) | `Repository<T>` stores products, rules and anything else with an `id()` |
| Operator overloading | [include/domain/Money.hpp](include/domain/Money.hpp), [include/domain/Cart.hpp](include/domain/Cart.hpp), [include/domain/TimeOfDay.hpp](include/domain/TimeOfDay.hpp) | `Money + - * < ==`, `<<`; `cart += CartItem`; `TimeOfDay < <= ==` |
| **Composition** | [include/rules/CompositeRule.hpp](include/rules/CompositeRule.hpp), [include/engine/PricingEngine.hpp](include/engine/PricingEngine.hpp) | Rule *has a* condition tree; engine *owns* its guards, publisher and GST calculator |
| Aggregation (non-owning) | [include/domain/CartItem.hpp](include/domain/CartItem.hpp), [include/engine/TracePublisher.hpp](include/engine/TracePublisher.hpp) | A cart line refers to a catalog product; the publisher refers to observers |
| **RAII / smart pointers** | [include/rules/Repository.hpp](include/rules/Repository.hpp), [src/language/RuleParser.cpp](src/language/RuleParser.cpp) | `std::unique_ptr` owns every polymorphic object. The `FILE*` is closed by a `unique_ptr` deleter. No raw `new`/`delete`. |
| Slicing prevention (deleted copy) | [include/rules/DiscountRule.hpp](include/rules/DiscountRule.hpp) | Copy and move are `= delete` on polymorphic bases |
| Dangling-reference prevention | [include/domain/PricingContext.hpp](include/domain/PricingContext.hpp) | Constructors taking temporaries are `= delete` |
| const-correctness | [include/domain/PricingContext.hpp](include/domain/PricingContext.hpp) | Rules see the order only through a read-only view |
| Null Object + Strategy (logging) | [include/infrastructure/Logger.hpp](include/infrastructure/Logger.hpp), [LogSink.hpp](include/infrastructure/LogSink.hpp), [FileLogSink.hpp](include/infrastructure/FileLogSink.hpp) | A logger with no sinks is silent, so code never checks "is logging on?". Destinations are swappable `LogSink`s. `FileLogSink` closes its file by RAII. |
| Exception hierarchy | [include/infrastructure/Exceptions.hpp](include/infrastructure/Exceptions.hpp) | `PricingError` → `ConfigError` → `RuleParseException` (with line and column) |
| **S - Single Responsibility** | [include/application/StoreLoader.hpp](include/application/StoreLoader.hpp), [include/engine/TracePublisher.hpp](include/engine/TracePublisher.hpp), [include/presentation/InvoicePrinter.hpp](include/presentation/InvoicePrinter.hpp) | Loading data, fanning out trace entries and printing are each one class. `CliApp` only handles menus. |
| **O - Open/Closed** | [include/engine/PricingEngine.hpp](include/engine/PricingEngine.hpp), [src/rules/RuleFactory.cpp](src/rules/RuleFactory.cpp), [include/application/ConflictPolicies.hpp](include/application/ConflictPolicies.hpp) | New offer = new subclass + one registry line. New policy = one line, and the CLI menu builds itself. Proved by tests in [test_pricing_engine.cpp](tests/test_pricing_engine.cpp), [test_cli_app.cpp](tests/test_cli_app.cpp) and [test_expiry_markdown.cpp](tests/test_expiry_markdown.cpp). |
| **L - Liskov Substitution** | [include/domain/Product.hpp](include/domain/Product.hpp), [include/presentation/StreamTraceObserver.hpp](include/presentation/StreamTraceObserver.hpp) | Any `Product` or `TraceObserver` subclass works wherever the base is expected. `ItemLevelRule` marks `isApplicable` `final` so subclasses cannot break its contract. |
| **I - Interface Segregation** | [include/rules/IItemLevelRule.hpp](include/rules/IItemLevelRule.hpp), [ICartLevelRule.hpp](include/rules/ICartLevelRule.hpp), [IRedeemable.hpp](include/rules/IRedeemable.hpp), [include/domain/Perishable.hpp](include/domain/Perishable.hpp) | Small role interfaces. Only coupons implement `IRedeemable`, and only perishable products implement `Perishable`. |
| **D - Dependency Inversion** | [include/engine/PricingEngine.hpp](include/engine/PricingEngine.hpp), [include/application/CheckoutService.hpp](include/application/CheckoutService.hpp), [include/rules/ExpiryMarkdown.hpp](include/rules/ExpiryMarkdown.hpp) | The engine depends on `ConflictResolver`, `PriceGuard` and `TraceObserver`. Guards are injected via `GuardFactory`. The expiry rule depends on `Perishable`, not `Grocery`. |
| **Decorator pattern** | [include/rules/RuleDecorator.hpp](include/rules/RuleDecorator.hpp), [CappedRule.hpp](include/rules/CappedRule.hpp), [HappyHourRule.hpp](include/rules/HappyHourRule.hpp), [FirstOrderRule.hpp](include/rules/FirstOrderRule.hpp) | Is-a *and* has-a `DiscountRule`. Behaviour is added by wrapping at run time, and wrappers nest. |
| Open/Closed through behaviour, not types | [include/application/UpsellAdvisor.hpp](include/application/UpsellAdvisor.hpp) | Finds unlockable savings by asking the engine for prices, never by inspecting rule classes |
| Strategy pattern | [include/engine/ConflictResolver.hpp](include/engine/ConflictResolver.hpp) | `StackingResolver`, `BestDiscountResolver`, `PriorityResolver` |
| Observer pattern | [include/engine/TraceObserver.hpp](include/engine/TraceObserver.hpp), [TracePublisher.hpp](include/engine/TracePublisher.hpp) | `PriceTrace` and the live `StreamTraceObserver` both listen |
| Template Method pattern | [include/rules/ItemLevelRule.hpp](include/rules/ItemLevelRule.hpp), [include/rules/RuleDecorator.hpp](include/rules/RuleDecorator.hpp) | `calculateDiscount()` is fixed; subclasses fill in `appliesToItem()` and `discountForItem()`. Decorators fill in `allows()` and `limit()`. |
| Factory / registry | [src/rules/RuleFactory.cpp](src/rules/RuleFactory.cpp), [src/infrastructure/CatalogLoader.cpp](src/infrastructure/CatalogLoader.cpp) | `"type"` string → creator function |
| Composite + Interpreter | [include/rules/ConditionNode.hpp](include/rules/ConditionNode.hpp), [include/language/RuleParser.hpp](include/language/RuleParser.hpp) | `And`/`Or`/`Not`/`Comparison` nodes evaluated as a tree |
| Facade | [include/application/CheckoutService.hpp](include/application/CheckoutService.hpp), [include/application/StoreLoader.hpp](include/application/StoreLoader.hpp) | One call hides engine, guard and loader wiring |

Every class also has a Doxygen comment naming the OOP/SOLID idea it shows
(run `doxygen Doxyfile` to browse them).

## Tests

GoogleTest, one test file per class (`tests/test_<class>.cpp`), 476 tests,
including scripted end-to-end CLI sessions. Shared builders live in
[tests/test_rule_fixtures.hpp](tests/test_rule_fixtures.hpp).

```sh
ctest --test-dir build --output-on-failure
ctest --test-dir build -R PricingEngine      # just one suite
```
