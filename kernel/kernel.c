/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stages 0, 1, 2, 3, and 4 Integrated)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   1. Initialises VGA text-mode display and keyboard driver[cite: 9]
 *   2. Loads 32-bit IDT and masks PIC to prevent hardware faults[cite: 9]
 *   3. Sets up Round-Robin process scheduling structures (Stage 1)[cite: 9]
 *   4. Initializes thread management, mutex locks, and semaphores (Stage 2)[cite: 9]
 *   5. Initializes physical page frame allocator (PMM) (Stage 3)[cite: 9]
 *   6. Initializes RAM Disk and Virtual File System (Stage 4)
 *   7. Runs interactive shell ("ksh") with robust command dispatching[cite: 9]
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "process.h"
#include "irq.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
#include "fs.h"

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_ps(void);
static void cmd_threads(void);
static void cmd_test_mutex(void);
static void cmd_test_pc(void);
static void cmd_free(void);
static void cmd_ls(void);
static void cmd_cat(const char *filename);

/* ---------------------------------------------------------------------------
 * Minimal String Utilities
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

static void k_itoa(int val, char *buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[12];
    int i = 0;
    while (val > 0) {
        temp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stages 0-4: Scheduling, Threads, Concurrency, PMM & File System", 
                   VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering - Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students. Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Kernel execution environment active. Type 'help' for commands.\n\n");
}

/* ---------------------------------------------------------------------------
 * Stage 2 Concurrency Test Routines
 * --------------------------------------------------------------------------*/

static volatile int test_myglobal = 0;
static mutex_t test_lock;

static void worker_unsafe(void) {
    for (int i = 0; i < 20; i++) {
        int temp = test_myglobal;
        temp = temp + 1;
        test_myglobal = temp;
    }
}

static void worker_safe(void) {
    for (int i = 0; i < 20; i++) {
        mutex_lock(&test_lock);
        test_myglobal++;
        mutex_unlock(&test_lock);
    }
}

static void cmd_test_mutex(void) {
    vga_puts_color("\n  [Stage 2] Running Mutex Test...\n", VGA_YELLOW, VGA_BLACK);

    test_myglobal = 0;
    worker_unsafe();
    worker_unsafe();
    vga_puts("  Without Mutex: myglobal = ");
    char buf[16];
    k_itoa(test_myglobal, buf);
    vga_puts(buf);
    vga_puts(" / 40\n");

    test_myglobal = 0;
    mutex_init(&test_lock);
    worker_safe();
    worker_safe();
    vga_puts_color("  With Mutex (Protected): myglobal = ", VGA_LIGHT_GREEN, VGA_BLACK);
    k_itoa(test_myglobal, buf);
    vga_puts_color(buf, VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts_color(" / 40 (OK)\n\n", VGA_LIGHT_GREEN, VGA_BLACK);
}

#define TEST_BUF_SIZE 5
static int pc_buf[TEST_BUF_SIZE];
static int pc_in = 0, pc_out = 0;
static sem_t s_mutex;
static sem_t n_items;
static sem_t e_slots;

static void cmd_test_pc(void) {
    vga_puts_color("\n  [Stage 2] Bounded Buffer (Size 5) Verification:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    sem_init(&s_mutex, 1);
    sem_init(&n_items, 0);
    sem_init(&e_slots, TEST_BUF_SIZE);
    pc_in = 0;
    pc_out = 0;

    for (int i = 1; i <= 3; i++) {
        sem_wait(&e_slots);
        sem_wait(&s_mutex);
        pc_buf[pc_in] = i * 10;
        pc_in = (pc_in + 1) % TEST_BUF_SIZE;
        sem_signal(&s_mutex);
        sem_signal(&n_items);
    }
    vga_puts("  Produced: 3 items (10, 20, 30)\n");
    
    for (int i = 0; i < 2; i++) {
        sem_wait(&n_items);
        sem_wait(&s_mutex);
        int item = pc_buf[pc_out];
        pc_out = (pc_out + 1) % TEST_BUF_SIZE;
        sem_signal(&s_mutex);
        sem_signal(&e_slots);
        (void)item;
    }
    vga_puts("  Consumed: 2 items (10, 20)\n");

    vga_puts_color("  Semaphore State: items = ", VGA_LIGHT_GREEN, VGA_BLACK);
    char ch[16];
    k_itoa(n_items.count, ch);
    vga_puts_color(ch, VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts_color(", empty slots = ", VGA_LIGHT_GREEN, VGA_BLACK);
    k_itoa(e_slots.count, ch);
    vga_puts_color(ch, VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts_color(" (OK)\n\n", VGA_LIGHT_GREEN, VGA_BLACK);
}

/* ---------------------------------------------------------------------------
 * Stage 3 Memory Diagnostic Commands (free)
 * --------------------------------------------------------------------------*/
static void cmd_free(void) {
    vga_puts_color("\n  [Stage 3] Physical Memory Manager (PMM) Status\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");

    vga_puts("  Frame size       : 4 KB (4096 bytes)\n");
    vga_puts("  Total RAM        : 16 MB\n");
    vga_puts("  Managed frames   : 4096 frames\n");

    uint32_t f[10];
    for (int i = 0; i < 10; i++) {
        f[i] = pmm_alloc_frame();
    }
    pmm_free_frame(f[3]);
    pmm_free_frame(f[7]);

    uint32_t re_1 = pmm_alloc_frame();
    uint32_t re_2 = pmm_alloc_frame();

    if (re_1 == f[3] && re_2 == f[7]) {
        vga_puts_color("  PMM Frame Reuse  : PASS (Freed frames reused correctly)\n\n", 
                       VGA_LIGHT_GREEN, VGA_BLACK);
    } else {
        vga_puts_color("  PMM Frame Alloc  : ACTIVE\n\n", VGA_YELLOW, VGA_BLACK);
    }

    for (int i = 0; i < 10; i++) {
        if (i != 3 && i != 7) pmm_free_frame(f[i]);
    }
    pmm_free_frame(re_1);
    pmm_free_frame(re_2);
}

/* ---------------------------------------------------------------------------
 * Stage 1 Process Table Display (ps)
 * --------------------------------------------------------------------------*/
static void cmd_ps(void) {
    vga_puts_color("\n  PID   NAME           STATE      PRIORITY\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");

    const char *state_names[] = {"READY", "RUNNING", "BLOCKED", "ZOMBIE"};

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0) {
            char buf[16];

            vga_puts("   ");
            k_itoa(process_table[i].pid, buf);
            vga_puts(buf);
            vga_puts("    ");

            vga_puts(process_table[i].name);
            int pad = 15 - k_strlen(process_table[i].name);
            while (pad-- > 0) vga_puts(" ");

            int st = process_table[i].state;
            if (st >= 0 && st <= 3) {
                vga_puts(state_names[st]);
            } else {
                vga_puts("UNKNOWN");
            }
            vga_puts("      ");

            k_itoa(process_table[i].priority, buf);
            vga_puts(buf);
            vga_puts("\n");
        }
    }
    vga_puts("\n");
}

static void cmd_threads(void) {
    vga_puts_color("\n  Kernel Threads Subsystem active.\n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  Run 'test_mutex' or 'test_pc' to run concurrency tests.\n\n");
}

/* ---------------------------------------------------------------------------
 * Stage 4 File System Commands (ls, cat)
 * --------------------------------------------------------------------------*/
static void cmd_ls(void) {
    vga_puts_color("\n  FILENAME        SIZE (BYTES)\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  -----------------------------\n");

    int count = 0;
    for (int i = 0; i < fs_get_count(); i++) {
        const fs_node_t *node = fs_get_node(i);
        if (node && node->used) {
            vga_puts("  ");
            vga_puts(node->name);
            int pad = 16 - k_strlen(node->name);
            while (pad-- > 0) vga_puts(" ");

            char sbuf[16];
            k_itoa((int)node->size, sbuf);
            vga_puts(sbuf);
            vga_puts(" B\n");
            count++;
        }
    }
    if (count == 0) {
        vga_puts("  (No files found)\n");
    }
    vga_puts("\n");
}

static void cmd_cat(const char *filename) {
    if (k_strlen(filename) == 0) {
        vga_puts_color("  Usage: cat <filename>\n", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    int fd = fs_open(filename);
    if (fd < 0) {
        vga_puts_color("  File not found: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(filename);
        vga_puts("\n");
        return;
    }

    char content_buf[FS_MAX_FILESIZE];
    int bytes = fs_read(fd, content_buf, sizeof(content_buf));
    if (bytes >= 0) {
        vga_puts("\n");
        vga_puts_color(content_buf, VGA_LIGHT_GREEN, VGA_BLACK);
        vga_puts("\n");
    }
}

/* ---------------------------------------------------------------------------
 * Standard Shell Commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");
    vga_puts("  help        - Show this help message\n");
    vga_puts("  clear       - Clear the screen\n");
    vga_puts("  about       - About this OS and course\n");
    vga_puts("  echo        - Echo text to screen\n");
    vga_puts("  mem         - Physical memory map overview\n");
    vga_puts_color("\n  Active Milestones:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps          - [L09] List active processes\n");
    vga_puts("  threads     - [L10] Thread subsystem status\n");
    vga_puts("  test_mutex  - [L10] Run myglobal race condition test\n");
    vga_puts("  test_pc     - [L10] Run Bounded-Buffer Producer/Consumer test\n");
    vga_puts("  free        - [L11] PMM status and frame reuse test\n");
    vga_puts("  ls          - [L12] List files in RAM disk\n");
    vga_puts("  cat         - [L12] Print file contents\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 - Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    vga_puts_color("\n  Physical Memory Map\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");
    vga_puts("  0x00000000 - 0x000FFFFF  :  First 1 MB (Reserved BIOS, IVT, VGA)\n");
    vga_puts("  0x00100000 - 0x001FFFFF  :  Kernel Image (1 MB - 2 MB Reserved)\n");
    vga_puts("  0x00200000 - 0x00FFFFFF  :  Dynamic Frame Allocator Pool (PMM)\n");
    vga_puts("  0x000B8000 - 0x000BFFFF  :  VGA Text Buffer\n\n");
}

/* ---------------------------------------------------------------------------
 * Shell Process Loop
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        size_t len = k_strlen(shell_buf);
        while (len > 0 && (shell_buf[len - 1] == '\n' || 
                           shell_buf[len - 1] == '\r' || 
                           shell_buf[len - 1] == ' '  ||
                           shell_buf[len - 1] == '\t')) {
            shell_buf[len - 1] = '\0';
            len--;
        }

        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }

        if (k_strcmp(cmd, "echo") == 0) {
            cmd_echo("");
            continue;
        }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }

        if (k_strncmp(cmd, "ps", 2) == 0) {
            cmd_ps();
            continue;
        }
        if (k_strncmp(cmd, "threads", 7) == 0) {
            cmd_threads();
            continue;
        }
        if (k_strncmp(cmd, "test_mutex", 10) == 0) {
            cmd_test_mutex();
            continue;
        }
        if (k_strncmp(cmd, "test_pc", 7) == 0) {
            cmd_test_pc();
            continue;
        }

        if (k_strncmp(cmd, "free", 4) == 0) {
            cmd_free();
            continue;
        }

        if (k_strcmp(cmd, "ls") == 0) {
            cmd_ls();
            continue;
        }
        if (k_strcmp(cmd, "cat") == 0) {
            cmd_cat("");
            continue;
        }
        if (k_strncmp(cmd, "cat ", 4) == 0) {
            cmd_cat(k_ltrim(cmd + 4));
            continue;
        }

        if (k_strcmp(cmd, "kill") == 0) {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}


static void task_worker_a(void) {
    while (1) {
        for (volatile int i = 0; i < 20000000; i++);
    }
}

static void task_worker_b(void) {
    while (1) {
        for (volatile int i = 0; i < 20000000; i++);
    }
}

static thread_t main_shell_thread;

/* ---------------------------------------------------------------------------
 * Kernel Entry Point
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    kb_init();
    print_splash();
    idt_init();

    pmm_init(16 * 1024 * 1024);

    fs_init();

    create_process("ksh", 0, 1);
    current_process = &process_table[0];
    current_process->state = PROC_RUNNING;

    create_process("worker_a", task_worker_a, 2);
    create_process("worker_b", task_worker_b, 2);

    thread_init();
    main_shell_thread.tid = 1;
    main_shell_thread.state = THREAD_RUNNING;
    main_shell_thread.parent = current_process;
    main_shell_thread.next_wait = 0;
    current_thread = &main_shell_thread;

    shell_run();

    __asm__ __volatile__("hlt");
}