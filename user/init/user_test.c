/**
 * User Mode Test Program
 *
 * Simple test program that runs in Ring 3 and makes syscalls.
 */

#include "../lib/syscall.h"

/**
 * Entry point for user mode program
 */
void user_main(void) {
    // Get our process ID
    int pid = getpid();

    // Print greeting
    puts("[User Mode] Hello from user space! PID: ");
    print_num(pid);
    puts("\n");

    // Test loop - count and display uptime
    for (int i = 0; i < 10; i++) {
        puts("[User Mode] Iteration ");
        print_num(i);
        puts(", Uptime: ");
        print_num((int)get_time());
        puts(" ms\n");

        // Sleep for 1 second
        sleep_ms(1000);
    }

    puts("[User Mode] Test complete, exiting...\n");
    exit(0);
}
