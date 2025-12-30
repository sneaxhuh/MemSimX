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

        case CommandType::INIT_CACHE: {
            if (cmd.args.size() < 8) {
                std::cout << "Error: Missing arguments. Usage: init cache <l1_sets> <l1_assoc> <l1_block> <l1_policy> <l2_sets> <l2_assoc> <l2_block> <l2_policy>" << std::endl;
                std::cout << "Policies: fifo, lru, lfu" << std::endl;
                break;
            }

            auto l1_sets_result = parseSize(cmd.args[0]);
            auto l1_assoc_result = parseSize(cmd.args[1]);
            auto l1_block_result = parseSize(cmd.args[2]);
            auto l1_policy_result = parseCachePolicy(cmd.args[3]);
            auto l2_sets_result = parseSize(cmd.args[4]);
            auto l2_assoc_result = parseSize(cmd.args[5]);
            auto l2_block_result = parseSize(cmd.args[6]);
            auto l2_policy_result = parseCachePolicy(cmd.args[7]);

            if (!l1_sets_result.success || !l1_assoc_result.success || !l1_block_result.success || !l1_policy_result.success ||
                !l2_sets_result.success || !l2_assoc_result.success || !l2_block_result.success || !l2_policy_result.success) {
                std::cout << "Error: Invalid cache parameters" << std::endl;
                break;
            }

            auto result = manager_.initCache(l1_sets_result.value, l1_assoc_result.value, l1_block_result.value, l1_policy_result.value,
                                            l2_sets_result.value, l2_assoc_result.value, l2_block_result.value, l2_policy_result.value);
            if (!result.success) {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::CACHE_READ: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing address. Usage: cache read <address>" << std::endl;
                break;
            }

            auto addr_result = parseAddress(cmd.args[0]);
            if (!addr_result.success) {
                std::cout << "Error: " << addr_result.error_message << std::endl;
                break;
            }

            auto result = manager_.cacheRead(addr_result.value);
            if (result.success) {
                std::cout << "Read from cache address 0x" << std::hex << addr_result.value
                          << ": 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(result.value)
                          << " (" << std::dec << static_cast<int>(result.value) << ")" << std::endl;
            } else {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::CACHE_WRITE: {
            if (cmd.args.size() < 2) {
                std::cout << "Error: Missing arguments. Usage: cache write <address> <value>" << std::endl;
                break;
            }

            auto addr_result = parseAddress(cmd.args[0]);
            if (!addr_result.success) {
                std::cout << "Error: " << addr_result.error_message << std::endl;
                break;
            }

            auto value_result = parseUInt8(cmd.args[1]);
            if (!value_result.success) {
                std::cout << "Error: " << value_result.error_message << std::endl;
                break;
            }

            auto result = manager_.cacheWrite(addr_result.value, value_result.value);
            if (result.success) {
                std::cout << "Wrote 0x" << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(value_result.value) << std::dec
                          << " to cache address 0x" << std::hex << addr_result.value << std::dec << std::endl;
            } else {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::CACHE_STATS: {
            manager_.printCacheStats();
            break;
        }

        case CommandType::CACHE_DUMP: {
            manager_.dumpCache();
            break;
        }

        case CommandType::CACHE_FLUSH: {
            manager_.flushCache();
            break;
        }

        case CommandType::INIT_VM: {
            if (cmd.args.size() < 4) {
                std::cout << "Error: Missing arguments. Usage: init vm <num_virtual_pages> <num_physical_frames> <page_size> <policy>" << std::endl;
                std::cout << "Policies: fifo, lru, clock" << std::endl;
                break;
            }

            auto vp_result = parseSize(cmd.args[0]);
            if (!vp_result.success) {
                std::cout << "Error parsing num_virtual_pages: " << vp_result.error_message << std::endl;
                break;
            }

            auto pf_result = parseSize(cmd.args[1]);
            if (!pf_result.success) {
                std::cout << "Error parsing num_physical_frames: " << pf_result.error_message << std::endl;
                break;
            }

            auto ps_result = parseSize(cmd.args[2]);
            if (!ps_result.success) {
                std::cout << "Error parsing page_size: " << ps_result.error_message << std::endl;
                break;
            }

            auto policy_result = parsePageReplacementPolicy(cmd.args[3]);
            if (!policy_result.success) {
                std::cout << "Error: " << policy_result.error_message << std::endl;
                break;
            }

            auto result = manager_.initVirtualMemory(vp_result.value, pf_result.value, ps_result.value, policy_result.value);
            if (!result.success) {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::VM_READ: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing virtual address. Usage: vm read <virtual_address>" << std::endl;
                break;
            }

            auto addr_result = parseAddress(cmd.args[0]);
            if (!addr_result.success) {
                std::cout << "Error: " << addr_result.error_message << std::endl;
                break;
            }

            auto result = manager_.vmRead(addr_result.value);
            if (result.success) {
                std::cout << "Read from virtual address 0x" << std::hex << addr_result.value
                          << ": 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(result.value)
                          << " (" << std::dec << static_cast<int>(result.value) << ")" << std::endl;
            } else {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::VM_WRITE: {
            if (cmd.args.size() < 2) {
                std::cout << "Error: Missing arguments. Usage: vm write <virtual_address> <value>" << std::endl;
                break;
            }

            auto addr_result = parseAddress(cmd.args[0]);
            if (!addr_result.success) {
                std::cout << "Error: " << addr_result.error_message << std::endl;
                break;
            }

            auto value_result = parseUInt8(cmd.args[1]);
            if (!value_result.success) {
                std::cout << "Error: " << value_result.error_message << std::endl;
                break;
            }

            auto result = manager_.vmWrite(addr_result.value, value_result.value);
            if (result.success) {
                std::cout << "Wrote 0x" << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(value_result.value) << std::dec
                          << " to virtual address 0x" << std::hex << addr_result.value << std::dec << std::endl;
            } else {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::VM_TRANSLATE: {
            if (cmd.args.empty()) {
                std::cout << "Error: Missing virtual address. Usage: vm translate <virtual_address>" << std::endl;
                break;
            }

            auto addr_result = parseAddress(cmd.args[0]);
            if (!addr_result.success) {
                std::cout << "Error: " << addr_result.error_message << std::endl;
                break;
            }

            auto result = manager_.vmTranslate(addr_result.value);
            if (result.success) {
                std::cout << "Virtual address 0x" << std::hex << addr_result.value
                          << " -> Physical address 0x" << result.value << std::dec << std::endl;
            } else {
                std::cout << "Error: " << result.error_message << std::endl;
            }
            break;
        }

        case CommandType::VM_STATS: {
            manager_.printVMStats();
            break;
        }

        case CommandType::VM_DUMP: {
            manager_.dumpVM();
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
    std::cout << "║     Memory Management Simulator v1.0                   ║\n";
    std::cout << "║                                                        ║\n";
    std::cout << "║  An OS memory management simulator with allocation     ║\n";
    std::cout << "║  strategies, caching, and virtual memory.              ║\n";
    std::cout << "║                                                        ║\n";
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

Result<PageReplacementPolicy> CLI::parsePageReplacementPolicy(const std::string& policy_str) {
    std::string lower = policy_str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower == "fifo") {
        return Result<PageReplacementPolicy>::Ok(PageReplacementPolicy::FIFO);
    } else if (lower == "lru") {
        return Result<PageReplacementPolicy>::Ok(PageReplacementPolicy::LRU);
    } else if (lower == "clock") {
        return Result<PageReplacementPolicy>::Ok(PageReplacementPolicy::CLOCK);
    } else {
        return Result<PageReplacementPolicy>::Err(
            "Invalid page replacement policy: " + policy_str + " (valid: fifo, lru, clock)"
        );
    }
}

Result<uint8_t> CLI::parseUInt8(const std::string& str) {
    try {
        // Support hex values with 0x prefix
        unsigned long value;
        if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            value = std::stoul(str, nullptr, 16);
        } else {
            value = std::stoul(str);
        }

        if (value > 255) {
            return Result<uint8_t>::Err("Value out of range for uint8_t (0-255): " + str);
        }

        return Result<uint8_t>::Ok(static_cast<uint8_t>(value));
    } catch (const std::exception&) {
        return Result<uint8_t>::Err("Invalid uint8_t value: " + str);
    }
}

Result<CachePolicy> CLI::parseCachePolicy(const std::string& policy_str) {
    std::string lower = policy_str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower == "fifo") {
        return Result<CachePolicy>::Ok(CachePolicy::FIFO);
    } else if (lower == "lru") {
        return Result<CachePolicy>::Ok(CachePolicy::LRU);
    } else if (lower == "lfu") {
        return Result<CachePolicy>::Ok(CachePolicy::LFU);
    } else {
        return Result<CachePolicy>::Err(
            "Invalid cache policy: " + policy_str + " (valid: fifo, lru, lfu)"
        );
    }
}

} // namespace memsim
