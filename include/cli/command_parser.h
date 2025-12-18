#ifndef MEMSIM_CLI_COMMAND_PARSER_H
#define MEMSIM_CLI_COMMAND_PARSER_H

#include <string>
#include <vector>

namespace memsim {

/**
 * @brief Types of commands supported by the CLI
 */
enum class CommandType {
    INIT_MEMORY,        // init memory <size>
    SET_ALLOCATOR,      // set allocator <type>
    MALLOC,             // malloc <size>
    FREE,               // free <block_id>
    FREE_ADDR,          // free_addr <address>
    DUMP_MEMORY,        // dump memory
    STATS,              // stats
    HELP,               // help
    EXIT,               // exit
    UNKNOWN             // Unrecognized command
};

/**
 * @brief Represents a parsed command with its arguments
 */
struct Command {
    CommandType type;
    std::vector<std::string> args;

    Command() : type(CommandType::UNKNOWN) {}
    Command(CommandType t) : type(t) {}
    Command(CommandType t, const std::vector<std::string>& a)
        : type(t), args(a) {}
};

/**
 * @brief Parser for CLI commands
 */
class CommandParser {
public:
    /**
     * @brief Parse a command string
     * @param input The input command string
     * @return Parsed command
     */
    static Command parse(const std::string& input);

    /**
     * @brief Print help information
     */
    static void printHelp();

private:
    /**
     * @brief Split a string into tokens
     * @param input String to split
     * @return Vector of tokens
     */
    static std::vector<std::string> tokenize(const std::string& input);

    /**
     * @brief Convert string to lowercase
     * @param str String to convert
     * @return Lowercase string
     */
    static std::string toLower(const std::string& str);
};

} // namespace memsim

#endif // MEMSIM_CLI_COMMAND_PARSER_H
