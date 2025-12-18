#ifndef MEMSIM_COMMON_RESULT_H
#define MEMSIM_COMMON_RESULT_H

#include <string>
#include <utility>

namespace memsim {

/**
 * @brief Result type for operations that can fail
 * @tparam T The type of the successful result value
 *
 * Provides a clean way to handle errors without exceptions.
 * Usage:
 *   Result<int> result = someOperation();
 *   if (result.success) {
 *       use(result.value);
 *   } else {
 *       handle_error(result.error_message);
 *   }
 */
template<typename T>
struct Result {
    bool success;
    T value;
    std::string error_message;

    // Constructor for success case
    static Result<T> Ok(T val) {
        return Result<T>{true, std::move(val), ""};
    }

    // Constructor for error case
    static Result<T> Err(const std::string& msg) {
        return Result<T>{false, T{}, msg};
    }

    // Check if result is successful
    explicit operator bool() const {
        return success;
    }
};

/**
 * @brief Specialization for void operations
 *
 * Used for operations that don't return a value but can fail.
 */
template<>
struct Result<void> {
    bool success;
    std::string error_message;

    // Constructor for success case
    static Result<void> Ok() {
        return Result<void>{true, ""};
    }

    // Constructor for error case
    static Result<void> Err(const std::string& msg) {
        return Result<void>{false, msg};
    }

    // Check if result is successful
    explicit operator bool() const {
        return success;
    }
};

} // namespace memsim

#endif // MEMSIM_COMMON_RESULT_H
