#include <stdio.h>
#include <stdlib.h>

extern "C" __declspec(noinline)
void hijacked()
{
    printf("HIJACKED\n");
    fflush(stdout);
    exit(42);
}
int vulnerable(const char* str)
{
    char buffer[12];
    sprintf(buffer, "%s", str);
    return buffer[0] != 0;
}

int main(int argc, char** argv)
{
    volatile void* keep_hijacked = (void*)&hijacked;
    char string[1025] = {};
    FILE* input;

    printf("Test_sprintf Loading input file...\n");

    input = fopen("input", "rb");
    if (!input) {
        printf("Could not open input.\n");
        return 1;
    }

    size_t len = fread(string, sizeof(char), 1024, input);
    fclose(input);
    string[len] = 0;

    vulnerable(string);

    printf("Completed.\n");
    return 0;
}
