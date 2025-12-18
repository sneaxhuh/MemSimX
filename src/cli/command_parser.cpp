#include "cli/command_parser.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace memsim {

Command CommandParser::parse(const std::string& input) {
    // Tokenize the input
    std::vector<std::string> tokens = tokenize(input);

    // Empty command
    if (tokens.empty()) {
        return Command(CommandType::UNKNOWN);
    }

    // Get the command keyword (first token, lowercase)
    std::string cmd = toLower(tokens[0]);

    // Parse based on command keyword
    if (cmd == "init" && tokens.size() >= 3 && toLower(tokens[1]) == "memory") {
        // init memory <size>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::INIT_MEMORY, args);
    }
    else if (cmd == "set" && tokens.size() >= 3 && toLower(tokens[1]) == "allocator") {
        // set allocator <type>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::SET_ALLOCATOR, args);
    }
    else if (cmd == "malloc" && tokens.size() >= 2) {
        // malloc <size>
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());
        return Command(CommandType::MALLOC, args);
    }
    else if (cmd == "free" && tokens.size() >= 2) {
        // free <block_id>
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());
        return Command(CommandType::FREE, args);
    }
    else if (cmd == "free_addr" && tokens.size() >= 2) {
        // free_addr <address>
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());
        return Command(CommandType::FREE_ADDR, args);
    }
    else if (cmd == "dump" && tokens.size() >= 2 && toLower(tokens[1]) == "memory") {
        // dump memory
        return Command(CommandType::DUMP_MEMORY);
    }
    else if (cmd == "stats") {
        // stats
        return Command(CommandType::STATS);
    }
    else if (cmd == "help") {
        // help
        return Command(CommandType::HELP);
    }
    else if (cmd == "exit" || cmd == "quit") {
        // exit or quit
        return Command(CommandType::EXIT);
    }
    else {
        // Unknown command
        return Command(CommandType::UNKNOWN);
    }
}

void CommandParser::printHelp() {
    std::cout << "\n=== Memory Simulator Commands ===" << std::endl;
    std::cout << "\nMemory Management:" << std::endl;
    std::cout << "  init memory <size>          - Initialize physical memory with specified size" << std::endl;
    std::cout << "                                 Example: init memory 1024" << std::endl;
    std::cout << "\nAllocator Configuration:" << std::endl;
    std::cout << "  set allocator <type>        - Set allocation strategy" << std::endl;
    std::cout << "                                 Types: first_fit, best_fit, worst_fit" << std::endl;
    std::cout << "                                 Example: set allocator first_fit" << std::endl;
    std::cout << "\nMemory Operations:" << std::endl;
    std::cout << "  malloc <size>               - Allocate memory block of specified size" << std::endl;
    std::cout << "                                 Example: malloc 100" << std::endl;
    std::cout << "  free <block_id>             - Deallocate block by ID" << std::endl;
    std::cout << "                                 Example: free 1" << std::endl;
    std::cout << "  free_addr <address>         - Deallocate block by address" << std::endl;
    std::cout << "                                 Example: free_addr 0" << std::endl;
    std::cout << "\nVisualization & Statistics:" << std::endl;
    std::cout << "  dump memory                 - Display memory layout" << std::endl;
    std::cout << "  stats                       - Show allocation statistics" << std::endl;
    std::cout << "\nGeneral:" << std::endl;
    std::cout << "  help                        - Show this help message" << std::endl;
    std::cout << "  exit                        - Exit the simulator" << std::endl;
    std::cout << std::endl;
}

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string CommandParser::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace memsim
