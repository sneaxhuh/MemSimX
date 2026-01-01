#ifndef MEMSIM_CLI_CLI_H
#define MEMSIM_CLI_CLI_H

#include "cli/command_parser.h"
#include "manager/memory_manager.h"

namespace memsim {

/**
 * @brief Command-line interface for the memory simulator
 *
 * Provides a REPL (Read-Eval-Print Loop) for interactive memory management.
 */
class CLI {
public:
    /**
     * @brief Construct a CLI
     * @param manager Reference to the memory manager
     */
    explicit CLI(MemoryManager& manager);

    /**
     * @brief Run the CLI main loop
     */
    void run();

private:
    MemoryManager& manager_;
    bool running_;

    /**
     * @brief Execute a parsed command
     * @param cmd The command to execute
     */
    void executeCommand(const Command& cmd);

    /**
     * @brief Print welcome message
     */
    void printWelcome();

    /**
     * @brief Print prompt
     */
    void printPrompt();

    /**
     * @brief Parse allocator type from string
     * @param type_str Allocator type string
     * @return AllocatorType or error
     */
    Result<AllocatorType> parseAllocatorType(const std::string& type_str);

    /**
     * @brief Parse size_t from string
     * @param str String to parse
     * @return size_t value or error
     */
    Result<size_t> parseSize(const std::string& str);

    /**
     * @brief Parse BlockId from string
     * @param str String to parse
     * @return BlockId value or error
     */
    Result<BlockId> parseBlockId(const std::string& str);

    /**
     * @brief Parse Address from string (supports hex with 0x prefix)
     * @param str String to parse
     * @return Address value or error
     */
    Result<Address> parseAddress(const std::string& str);

    /**
     * @brief Parse PageReplacementPolicy from string
     * @param policy_str Policy string (fifo, lru)
     * @return PageReplacementPolicy or error
     */
    Result<PageReplacementPolicy> parsePageReplacementPolicy(const std::string& policy_str);

    /**
     * @brief Parse uint8_t from string
     * @param str String to parse
     * @return uint8_t value or error
     */
    Result<uint8_t> parseUInt8(const std::string& str);

    /**
     * @brief Parse CachePolicy from string
     * @param policy_str Policy string (fifo, lru, lfu)
     * @return CachePolicy or error
     */
    Result<CachePolicy> parseCachePolicy(const std::string& policy_str);
};

} // namespace memsim

#endif // MEMSIM_CLI_CLI_H
