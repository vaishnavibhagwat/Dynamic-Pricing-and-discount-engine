#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace infrastructure {

/**
 * @brief Root of the project's own exception hierarchy.
 *
 * OOP idea: inheritance for error handling - callers can catch one base
 * type for "anything the pricing system threw" or a specific subclass.
 */
class PricingError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Bad or unreadable configuration data (e.g. a malformed rules.json).
 *
 * OOP idea: specialisation through inheritance (is-a PricingError).
 */
class ConfigError : public PricingError {
public:
    using PricingError::PricingError;
};

/**
 * @brief A syntax or type error in a text rule file, with its exact position.
 *
 * OOP idea: specialisation through inheritance - it is-a ConfigError, so
 * code that already handles bad configuration also handles bad rule text,
 * while a caller that cares can catch it and read line() / column().
 * what() reads "<source>:<line>:<column>: <detail>".
 */
class RuleParseException : public ConfigError {
public:
    /// @p line and @p column are 1-based.
    RuleParseException(std::string source, int line, int column, std::string detail)
        : ConfigError(source + ":" + std::to_string(line) + ":" + std::to_string(column) + ": " +
                      detail),
          source_(std::move(source)), line_(line), column_(column), detail_(std::move(detail)) {}

    const std::string& source() const noexcept { return source_; }
    int line() const noexcept { return line_; }
    int column() const noexcept { return column_; }
    /// The message without the position prefix.
    const std::string& detail() const noexcept { return detail_; }

private:
    std::string source_;
    int line_;
    int column_;
    std::string detail_;
};

} // namespace infrastructure
