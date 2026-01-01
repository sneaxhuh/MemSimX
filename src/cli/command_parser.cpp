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
    else if (cmd == "init" && tokens.size() >= 3 && toLower(tokens[1]) == "cache") {
        // init cache <l1_sets> <l1_assoc> <l1_block> <l1_policy> <l2_sets> <l2_assoc> <l2_block> <l2_policy>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::INIT_CACHE, args);
    }
    else if (cmd == "cache" && tokens.size() >= 3 && toLower(tokens[1]) == "read") {
        // cache read <address>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::CACHE_READ, args);
    }
    else if (cmd == "cache" && tokens.size() >= 4 && toLower(tokens[1]) == "write") {
        // cache write <address> <value>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::CACHE_WRITE, args);
    }
    else if (cmd == "cache" && tokens.size() >= 2 && toLower(tokens[1]) == "stats") {
        // cache stats
        return Command(CommandType::CACHE_STATS);
    }
    else if (cmd == "cache" && tokens.size() >= 2 && toLower(tokens[1]) == "dump") {
        // cache dump
        return Command(CommandType::CACHE_DUMP);
    }
    else if (cmd == "cache" && tokens.size() >= 2 && toLower(tokens[1]) == "flush") {
        // cache flush
        return Command(CommandType::CACHE_FLUSH);
    }
    else if (cmd == "init" && tokens.size() >= 3 && toLower(tokens[1]) == "vm") {
        // init vm <num_virtual_pages> <num_physical_frames> <page_size> <policy>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::INIT_VM, args);
    }
    else if (cmd == "vm" && tokens.size() >= 3 && toLower(tokens[1]) == "read") {
        // vm read <virtual_address>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::VM_READ, args);
    }
    else if (cmd == "vm" && tokens.size() >= 4 && toLower(tokens[1]) == "write") {
        // vm write <virtual_address> <value>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::VM_WRITE, args);
    }
    else if (cmd == "vm" && tokens.size() >= 3 && toLower(tokens[1]) == "translate") {
        // vm translate <virtual_address>
        std::vector<std::string> args(tokens.begin() + 2, tokens.end());
        return Command(CommandType::VM_TRANSLATE, args);
    }
    else if (cmd == "vm" && tokens.size() >= 2 && toLower(tokens[1]) == "stats") {
        // vm stats
        return Command(CommandType::VM_STATS);
    }
    else if (cmd == "vm" && tokens.size() >= 2 && toLower(tokens[1]) == "dump") {
        // vm dump
        return Command(CommandType::VM_DUMP);
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
    std::cout << "                                 Types: first_fit, best_fit, worst_fit, buddy" << std::endl;
    std::cout << "                                 Example: set allocator first_fit" << std::endl;
    std::cout << "                                 Note: Buddy allocator rounds allocations to" << std::endl;
    std::cout << "                                       powers of two and coalesces buddies automatically" << std::endl;
    std::cout << "\nMemory Operations:" << std::endl;
    std::cout << "  malloc <size>               - Allocate memory block of specified size" << std::endl;
    std::cout << "                                 Example: malloc 100" << std::endl;
    std::cout << "  free <block_id>             - Deallocate block by ID" << std::endl;
    std::cout << "                                 Example: free 1" << std::endl;
    std::cout << "  free_addr <physical_address>" << std::endl;
    std::cout << "                              - Deallocate block by physical address" << std::endl;
    std::cout << "                                 Example: free_addr 0" << std::endl;
    std::cout << "\nCache Hierarchy:" << std::endl;
    std::cout << "  init cache <l1_s> <l1_a> <l1_b> <l1_p> <l2_s> <l2_a> <l2_b> <l2_p>" << std::endl;
    std::cout << "                              - Initialize L1/L2 cache hierarchy" << std::endl;
    std::cout << "                                 l1_s/l2_s: number of sets" << std::endl;
    std::cout << "                                 l1_a/l2_a: associativity (ways)" << std::endl;
    std::cout << "                                 l1_b/l2_b: block size in bytes" << std::endl;
    std::cout << "                                 l1_p/l2_p: policy (fifo, lru, lfu)" << std::endl;
    std::cout << "                                 Example: init cache 4 2 16 lru 8 4 32 lru" << std::endl;
    std::cout << "  cache read <address>        - Read from cache (uses physical address)" << std::endl;
    std::cout << "                                 Example: cache read 1024" << std::endl;
    std::cout << "  cache write <address> <value>" << std::endl;
    std::cout << "                              - Write to cache (write-through)" << std::endl;
    std::cout << "                                 Example: cache write 1024 42" << std::endl;
    std::cout << "  cache stats                 - Show cache statistics (hit ratio, miss ratio)" << std::endl;
    std::cout << "  cache dump                  - Display cache contents" << std::endl;
    std::cout << "  cache flush                 - Invalidate all cache lines" << std::endl;
    std::cout << "\nVirtual Memory:" << std::endl;
    std::cout << "  init vm <vp> <pf> <ps> <policy>" << std::endl;
    std::cout << "                              - Initialize virtual memory system" << std::endl;
    std::cout << "                                 vp: number of virtual pages" << std::endl;
    std::cout << "                                 pf: number of physical frames" << std::endl;
    std::cout << "                                 ps: page size in bytes" << std::endl;
    std::cout << "                                 policy: fifo, lru, or clock" << std::endl;
    std::cout << "                                 Example: init vm 16 4 256 lru" << std::endl;
    std::cout << "  vm read <virtual_addr>      - Read from virtual address" << std::endl;
    std::cout << "                                 Example: vm read 1024" << std::endl;
    std::cout << "  vm write <virtual_addr> <value>" << std::endl;
    std::cout << "                              - Write to virtual address" << std::endl;
    std::cout << "                                 Example: vm write 1024 42" << std::endl;
    std::cout << "  vm translate <virtual_addr> - Translate virtual to physical address" << std::endl;
    std::cout << "                                 Example: vm translate 1024" << std::endl;
    std::cout << "  vm stats                    - Show virtual memory statistics (page faults, hit rate)" << std::endl;
    std::cout << "  vm dump                     - Display page table" << std::endl;
    std::cout << "\nVisualization & Statistics:" << std::endl;
    std::cout << "  dump memory                 - Display memory layout" << std::endl;
    std::cout << "  stats                       - Show allocator statistics (strategy, fragmentation, utilization)" << std::endl;
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
