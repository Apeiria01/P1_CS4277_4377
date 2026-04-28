#include <stdio.h>
#include <string.h>

#include <stdlib.h>

extern "C" __declspec(noinline)
void hijacked()
{
    printf("HIJACKED\n");
    fflush(stdout);
    exit(42);
}

int vulnerable(const char* str, size_t len)
{
    char buffer[12];
    strncpy(buffer, str, len);
    return buffer[0] != 0;
}

int main(int argc, char** argv)
{
    volatile void* keep_hijacked = (void*)&hijacked;
    char string[1024];
    FILE* input;

    printf("Test_strncpy Loading input file...\n");

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
