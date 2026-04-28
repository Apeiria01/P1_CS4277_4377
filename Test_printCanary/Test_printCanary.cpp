#include <stdint.h>
#include <stdio.h>

#include <intrin.h>
#pragma intrinsic(_AddressOfReturnAddress)


extern "C" {
volatile uintptr_t __cs4277_canary_guard = 0;
}

static void print_stack_words(const char* label, volatile uintptr_t* base,
    int first, int last) {
    printf("%s (%zu-byte words):\n", label, sizeof(uintptr_t));
    for (int i = first; i <= last; ++i) {
        volatile uintptr_t* slot = base + i;
        printf("  [%+03d] %p : 0x%016llx\n",
            i, (const void*)slot, (unsigned long long)*slot);
    }
}



int main() {
    volatile uintptr_t local_probe = 0;
    printf("__cs4277_canary_guard address: %p\n", (const void*)&__cs4277_canary_guard);
    printf("__cs4277_canary_guard value: 0x%016llx\n", (unsigned long long)__cs4277_canary_guard);
    printf("main local probe address: %p\n", (const void*)&local_probe);
    print_stack_words("stack near local_probe", &local_probe, -8, 16);
    printf("main return address slot: %p\n", _AddressOfReturnAddress());
    print_stack_words("stack near return address slot", (volatile uintptr_t*)_AddressOfReturnAddress(), -8, 4);

    return 0;
}
