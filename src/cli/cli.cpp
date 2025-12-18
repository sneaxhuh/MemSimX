#include "cli/cli.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <limits>

namespace memsim {

CLI::CLI(MemoryManager& manager)
    : manager_(manager), running_(false) {
}

void CLI::run() {
    running_ = true;
    printWelcome();

    std::string line;
    while (running_) {
        printPrompt();

        if (!std::getline(std::cin, line)) {
            // EOF or error
            break;
        }

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Parse and execute command
        Command cmd = CommandParser::parse(line);
        executeCommand(cmd);
    }

    std::cout << "\nGoodbye!" << std::endl;
}

void CLI::executeCommand(const Command& cmd) {
    switch (cmd.type) {
        case CommandType::INIT_MEMORY: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing size argument. Usage: init memory <size>" << std::endl;
                break;
            }

            auto size_result = parseSize(cmd.args[0]);
            if (!size_result.success) {
                std::cout << "Error: " << size_result.error_message << std::endl;
                break;
            }

            auto result = manager_.initMemory(size_result.value);
            if (!result.success) {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::SET_ALLOCATOR: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing allocator type. Usage: set allocator <type>" << std::endl;
                std::cout << "Types: first_fit, best_fit, worst_fit" << std::endl;
                break;
            }

            auto type_result = parseAllocatorType(cmd.args[0]);
            if (!type_result.success) {
                std::cout << "Error: " << type_result.error_message << std::endl;
                break;
            }

            auto result = manager_.setAllocator(type_result.value);
            if (!result.success) {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::MALLOC: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing size argument. Usage: malloc <size>" << std::endl;
                break;
            }

            auto size_result = parseSize(cmd.args[0]);
            if (!size_result.success) {
                std::cout << "Error: " << size_result.error_message << std::endl;
                break;
            }

            auto result = manager_.malloc(size_result.value);
            if (!result.success) {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::FREE: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing block ID. Usage: free <block_id>" << std::endl;
                break;
            }

            auto id_result = parseBlockId(cmd.args[0]);
            if (!id_result.success) {
                std::cout << "Error: " << id_result.error_message << std::endl;
                break;
            }

            auto result = manager_.free(id_result.value);
            if (!result.success) {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::FREE_ADDR: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing address. Usage: free_addr <address>" << std::endl;
                break;
            }

            auto addr_result = parseAddress(cmd.args[0]);
            if (!addr_result.success) {
                std::cout << "Error: " << addr_result.error_message << std::endl;
                break;
            }

            auto result = manager_.freeByAddress(addr_result.value);
            if (!result.success) {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::DUMP_MEMORY: {
            manager_.dumpMemory();
            break;
        }

        case CommandType::STATS: {
            manager_.printStats();
            break;
        }

        case CommandType::HELP: {
            CommandParser::printHelp();
            break;
        }

        case CommandType::EXIT: {
            running_ = false;
            break;
        }

        case CommandType::UNKNOWN:
        default: {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
            break;
        }
    }
}

void CLI::printWelcome() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Memory Management Simulator v1.0                  ║\n";
    std::cout << "║                                                        ║\n";
    std::cout << "║  A comprehensive OS memory management simulator        ║\n";
    std::cout << "║  featuring multiple allocation strategies and         ║\n";
    std::cout << "║  detailed performance metrics.                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Type 'help' for available commands.\n";
    std::cout << "\n";
}

void CLI::printPrompt() {
    std::cout << "> " << std::flush;
}

Result<AllocatorType> CLI::parseAllocatorType(const std::string& type_str) {
    std::string lower = type_str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower == "first_fit") {
        return Result<AllocatorType>::Ok(AllocatorType::FIRST_FIT);
    } else if (lower == "best_fit") {
        return Result<AllocatorType>::Ok(AllocatorType::BEST_FIT);
    } else if (lower == "worst_fit") {
        return Result<AllocatorType>::Ok(AllocatorType::WORST_FIT);
    } else if (lower == "buddy") {
        return Result<AllocatorType>::Ok(AllocatorType::BUDDY);
    } else {
        return Result<AllocatorType>::Err("Invalid allocator type. Valid types: first_fit, best_fit, worst_fit, buddy");
    }
}

Result<size_t> CLI::parseSize(const std::string& str) {
    try {
        size_t value = std::stoull(str);
        return Result<size_t>::Ok(value);
    } catch (const std::exception&) {
        return Result<size_t>::Err("Invalid number: " + str);
    }
}

Result<BlockId> CLI::parseBlockId(const std::string& str) {
    try {
        unsigned long value = std::stoul(str);
        if (value > std::numeric_limits<BlockId>::max()) {
            return Result<BlockId>::Err("Block ID too large");
        }
        return Result<BlockId>::Ok(static_cast<BlockId>(value));
    } catch (const std::exception&) {
        return Result<BlockId>::Err("Invalid block ID: " + str);
    }
}

Result<Address> CLI::parseAddress(const std::string& str) {
    try {
        // Support hex addresses with 0x prefix
        Address value;
        if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            value = std::stoull(str, nullptr, 16);
        } else {
            value = std::stoull(str);
        }
        return Result<Address>::Ok(value);
    } catch (const std::exception&) {
        return Result<Address>::Err("Invalid address: " + str);
    }
}

} // namespace memsim
