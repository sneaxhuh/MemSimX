#include "cli/cli.h"
#include "manager/memory_manager.h"

int main() {
    memsim::MemoryManager manager;
    memsim::CLI cli(manager);

    cli.run();

    return 0;
}
