#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>   // For kmalloc, kzalloc, kfree
#include <linux/vmalloc.h>  // For vmalloc, vfree

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vivek");
MODULE_DESCRIPTION("Kernel Memory Allocation Example (kmalloc, kzalloc, vmalloc)");

// Define a simple structure for illustration
struct example_struct {
    int id;
    char name[32];
};

static int __init memory_example_init(void) {
    char *kbuf_kmalloc;
    char *kbuf_kzalloc;
    struct example_struct *kstruct_kmalloc;
    char *vbuf_vmalloc;
    unsigned long i;

    printk(KERN_INFO "Memory Example: Module Loaded\n");

    // 1. kmalloc: Allocate physically contiguous memory.
    //    - Fast, suitable for small to medium-sized allocations.
    //    - Can be used in atomic contexts (with GFP_ATOMIC).
    //    - Memory is not initialized (contains garbage).
    //    - Common use: DMA buffers, frequently accessed small structures.
    printk(KERN_INFO "Memory Example: Illustrating kmalloc\n");
    kbuf_kmalloc = kmalloc(1024, GFP_KERNEL); // GFP_KERNEL allows sleeping
    if (!kbuf_kmalloc) {
        printk(KERN_ERR "Memory Example: kmalloc failed for kbuf_kmalloc\n");
        // No cleanup needed here as nothing else was allocated yet
        return -ENOMEM;
    }
    printk(KERN_INFO "Memory Example: kmalloc allocated 1024 bytes at %px\n", kbuf_kmalloc);
    // You can use this buffer, e.g., strcpy(kbuf_kmalloc, "Hello from kmalloc");
    // printk(KERN_INFO "Memory Example: kbuf_kmalloc content: %s\n", kbuf_kmalloc); // Be careful with uninitialized data

    // 2. kzalloc: Allocate physically contiguous memory and zero-initialize it.
    //    - Similar to kmalloc but memory is cleared.
    //    - Slightly slower than kmalloc due to zeroing.
    //    - Useful when you need zeroed memory to avoid uninitialized data issues.
    printk(KERN_INFO "Memory Example: Illustrating kzalloc\n");
    kbuf_kzalloc = kzalloc(512, GFP_KERNEL);
    if (!kbuf_kzalloc) {
        printk(KERN_ERR "Memory Example: kzalloc failed for kbuf_kzalloc\n");
        kfree(kbuf_kmalloc); // Free previously allocated memory
        return -ENOMEM;
    }
    printk(KERN_INFO "Memory Example: kzalloc allocated and zeroed 512 bytes at %px\n", kbuf_kzalloc);
    // Verify it's zeroed (first byte)
    printk(KERN_INFO "Memory Example: kbuf_kzalloc[0] = %d\n", kbuf_kzalloc[0]);

    // Example with a structure
    kstruct_kmalloc = kmalloc(sizeof(struct example_struct), GFP_KERNEL);
    if (!kstruct_kmalloc) {
        printk(KERN_ERR "Memory Example: kmalloc failed for kstruct_kmalloc\n");
        kfree(kbuf_kzalloc);
        kfree(kbuf_kmalloc);
        return -ENOMEM;
    }
    kstruct_kmalloc->id = 1;
    snprintf(kstruct_kmalloc->name, sizeof(kstruct_kmalloc->name), "Example Struct");
    printk(KERN_INFO "Memory Example: kstruct_kmalloc: id=%d, name='%s' at %px\n",
           kstruct_kmalloc->id, kstruct_kmalloc->name, kstruct_kmalloc);


    // 3. vmalloc: Allocate virtually contiguous memory.
    //    - Pages may not be physically contiguous.
    //    - Suitable for large allocations where physical contiguity is not required.
    //    - Slower than kmalloc due to page table manipulation.
    //    - Cannot be used in atomic contexts (can sleep).
    //    - Common use: Buffers for user-space interaction, large temporary buffers.
    printk(KERN_INFO "Memory Example: Illustrating vmalloc\n");
    // Allocate a larger buffer, e.g., 2MB (2 * 1024 * 1024 bytes)
    // For illustration, let's use a smaller size to avoid excessive memory use in simple tests.
    // Let's try to allocate more than a page (e.g., 2 * PAGE_SIZE)
    vbuf_vmalloc = vmalloc(2 * PAGE_SIZE);
    if (!vbuf_vmalloc) {
        printk(KERN_ERR "Memory Example: vmalloc failed for vbuf_vmalloc\n");
        kfree(kstruct_kmalloc);
        kfree(kbuf_kzalloc);
        kfree(kbuf_kmalloc);
        return -ENOMEM;
    }
    printk(KERN_INFO "Memory Example: vmalloc allocated %lu bytes at %px\n", (2 * PAGE_SIZE), vbuf_vmalloc);
    // You can use this buffer, e.g., vbuf_vmalloc[0] = 'V'; vbuf_vmalloc[PAGE_SIZE] = 'M';
    // Accessing vmalloc'd memory can be slightly slower due to potential TLB misses if pages are scattered.

    // Differences and when to use each:
    // - Physical Contiguity:
    //   - kmalloc/kzalloc: YES. Essential for DMA.
    //   - vmalloc: NO. Pages are virtually contiguous but can be physically scattered.
    // - Allocation Size:
    //   - kmalloc/kzalloc: Best for small to medium allocations (up to a few MB, typically < 128KB or 4MB depending on arch/config).
    //   - vmalloc: Suitable for large allocations (many MBs or GBs).
    // - Speed:
    //   - kmalloc/kzalloc: Faster, as it often involves simpler slab allocator operations.
    //   - vmalloc: Slower, involves page table updates.
    // - Atomic Context:
    //   - kmalloc/kzalloc: Can be used with GFP_ATOMIC (won't sleep, tries hard but can fail).
    //   - vmalloc: CANNOT be used in atomic context (always GFP_KERNEL or similar, which can sleep).
    // - Initialization:
    //   - kmalloc: Memory is uninitialized.
    //   - kzalloc: Memory is zero-initialized.
    //   - vmalloc: Memory is uninitialized (use vzalloc for zeroed vmalloc, though vzalloc is just vmalloc + memset).
    // - Overhead:
    //   - kmalloc/kzalloc: Lower overhead per allocation.
    //   - vmalloc: Higher overhead due to page table management.

    printk(KERN_INFO "Memory Example: All allocations successful. Will free them in exit function.\n");

    // Freeing memory will be done in the exit function to show they persist.
    // For a real driver, you'd free memory if the init fails after some allocations.
    // Here, we are keeping them to be freed by memory_example_exit.
    // To illustrate kfree and vfree, we will free them in the exit function.
    // However, for robustness, if any allocation fails, prior ones should be freed.
    // The current error handling path illustrates this.

    // Store pointers to free them in the exit function (not best practice for real modules,
    // usually you'd have them in a module-specific struct or free upon error).
    // For this example, we'll re-declare them in exit and assume they are the same.
    // A better way would be to make them global static pointers.
    // Let's make them global for proper cleanup.

    // (Pointers are already declared globally for this example's structure,
    //  but in a real module, you'd pass them or store them in a module context struct)
    // For this example, we will free the buffers allocated above.
    // The actual freeing is done in memory_example_exit.

    return 0; // Success
}

// Global pointers to hold allocated memory for freeing in exit
static char *g_kbuf_kmalloc;
static char *g_kbuf_kzalloc;
static struct example_struct *g_kstruct_kmalloc;
static char *g_vbuf_vmalloc;


static int __init memory_example_init_better(void) {
    printk(KERN_INFO "Memory Example: Module Loaded (Better Version)\n");

    g_kbuf_kmalloc = kmalloc(1024, GFP_KERNEL);
    if (!g_kbuf_kmalloc) {
        printk(KERN_ERR "Memory Example: kmalloc failed for g_kbuf_kmalloc\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "Memory Example: kmalloc allocated 1024 bytes for g_kbuf_kmalloc at %px\n", g_kbuf_kmalloc);
    // strcpy(g_kbuf_kmalloc, "Test kmalloc"); // Example usage

    g_kbuf_kzalloc = kzalloc(512, GFP_KERNEL);
    if (!g_kbuf_kzalloc) {
        printk(KERN_ERR "Memory Example: kzalloc failed for g_kbuf_kzalloc\n");
        kfree(g_kbuf_kmalloc);
        g_kbuf_kmalloc = NULL; // Mark as freed
        return -ENOMEM;
    }
    printk(KERN_INFO "Memory Example: kzalloc allocated 512 bytes for g_kbuf_kzalloc at %px\n", g_kbuf_kzalloc);
    // printk(KERN_INFO "Memory Example: g_kbuf_kzalloc[0] = %d\n", g_kbuf_kzalloc[0]); // Should be 0

    g_kstruct_kmalloc = kmalloc(sizeof(struct example_struct), GFP_KERNEL);
    if (!g_kstruct_kmalloc) {
        printk(KERN_ERR "Memory Example: kmalloc failed for g_kstruct_kmalloc\n");
        kfree(g_kbuf_kzalloc);
        g_kbuf_kzalloc = NULL;
        kfree(g_kbuf_kmalloc);
        g_kbuf_kmalloc = NULL;
        return -ENOMEM;
    }
    g_kstruct_kmalloc->id = 100;
    snprintf(g_kstruct_kmalloc->name, sizeof(g_kstruct_kmalloc->name), "Global Struct");
    printk(KERN_INFO "Memory Example: g_kstruct_kmalloc: id=%d, name='%s' at %px\n",
           g_kstruct_kmalloc->id, g_kstruct_kmalloc->name, g_kstruct_kmalloc);

    g_vbuf_vmalloc = vmalloc(2 * PAGE_SIZE);
    if (!g_vbuf_vmalloc) {
        printk(KERN_ERR "Memory Example: vmalloc failed for g_vbuf_vmalloc\n");
        kfree(g_kstruct_kmalloc);
        g_kstruct_kmalloc = NULL;
        kfree(g_kbuf_kzalloc);
        g_kbuf_kzalloc = NULL;
        kfree(g_kbuf_kmalloc);
        g_kbuf_kmalloc = NULL;
        return -ENOMEM;
    }
    printk(KERN_INFO "Memory Example: vmalloc allocated %lu bytes for g_vbuf_vmalloc at %px\n", (2 * PAGE_SIZE), g_vbuf_vmalloc);
    // g_vbuf_vmalloc[0] = 'X'; // Example usage

    printk(KERN_INFO "Memory Example: All allocations successful. Will free them in exit function.\n");
    return 0;
}


static void __exit memory_example_exit(void) {
    printk(KERN_INFO "Memory Example: Module Unloading\n");

    // Freeing vmalloc'd memory
    if (g_vbuf_vmalloc) {
        printk(KERN_INFO "Memory Example: Freeing g_vbuf_vmalloc at %px\n", g_vbuf_vmalloc);
        vfree(g_vbuf_vmalloc);
        g_vbuf_vmalloc = NULL;
    } else {
        printk(KERN_INFO "Memory Example: g_vbuf_vmalloc was already NULL or not allocated by better_init.\n");
    }

    // Freeing kmalloc'd/kzalloc'd memory
    // Note: kfree can free memory allocated by kmalloc or kzalloc.
    if (g_kstruct_kmalloc) {
        printk(KERN_INFO "Memory Example: Freeing g_kstruct_kmalloc at %px\n", g_kstruct_kmalloc);
        kfree(g_kstruct_kmalloc);
        g_kstruct_kmalloc = NULL;
    } else {
        printk(KERN_INFO "Memory Example: g_kstruct_kmalloc was already NULL or not allocated by better_init.\n");
    }

    if (g_kbuf_kzalloc) {
        printk(KERN_INFO "Memory Example: Freeing g_kbuf_kzalloc at %px\n", g_kbuf_kzalloc);
        kfree(g_kbuf_kzalloc);
        g_kbuf_kzalloc = NULL;
    } else {
        printk(KERN_INFO "Memory Example: g_kbuf_kzalloc was already NULL or not allocated by better_init.\n");
    }

    if (g_kbuf_kmalloc) {
        printk(KERN_INFO "Memory Example: Freeing g_kbuf_kmalloc at %px\n", g_kbuf_kmalloc);
        kfree(g_kbuf_kmalloc);
        g_kbuf_kmalloc = NULL;
    } else {
        printk(KERN_INFO "Memory Example: g_kbuf_kmalloc was already NULL or not allocated by better_init.\n");
    }

    printk(KERN_INFO "Memory Example: All allocated memory freed.\n");
    printk(KERN_INFO "Memory Example: Module Unloaded\n");
}

// Use the version with global pointers for proper cleanup
module_init(memory_example_init_better);
// module_init(memory_example_init); // Original version for comparison if needed
module_exit(memory_example_exit);
