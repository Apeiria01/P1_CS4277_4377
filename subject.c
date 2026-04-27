#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int vulnerable( const char * str )
{
	char buffer[12];
	/* This unrestricted strcpy is vulernable to a stack overflow */
	strcpy (buffer, str);
	return 1;
}


int main ( int argc, char ** argv )
{
	char string[1024];
	FILE *input;

	printf("Loading input file...\n");

	input = fopen("input", "r");
	fread(string, sizeof(char), 1024, input);

	vulnerable( string );

	printf("Completed.\n");
	return 0;
}

