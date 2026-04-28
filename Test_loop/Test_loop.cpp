#include <stdio.h>

#include <stdlib.h>

extern "C" __declspec(noinline)
void hijacked()
{
    __debugbreak();
    exit(42);
}

int vulnerable(const char* str, size_t len)
{
    char buffer[12];
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = str[i];
    }
    return buffer[0] != 0;
}

int main(int argc, char** argv)
{
    volatile void* keep_hijacked = (void*)&hijacked;
    char string[1024];
    FILE* input;

    printf("Test_loop input file...\n");
    printf("func ptr: %p\n", keep_hijacked);

    input = fopen("input", "rb");
    if (!input) {
        printf("Could not open input.\n");
        return 1;
    }

    size_t len = fread(string, sizeof(char), sizeof(string), input);
    fclose(input);

    vulnerable(string, len);

    printf("Completed.\n");
    return 0;
}
