#include "stdio.h"
#include <stdlib.h>
#include <string.h>

/* --- Main Code --- */
int main(int argc, char const *argv[])
{
  // Ensure that the program h as been invoked correctly
  if (argc < 1 || argc > 3) {
		fprintf(stderr, "USAGE:\n./main.out\n./main.out <input file>\n./main.out <input file> <output file>\n");
    exit(1);
	}

  // Create default input and output file names
  char inputFileName[100] = "data.txt";
  char outputFileName[100] = "output.txt";

  // Override the default input and output file names if they have been specified by the user
  switch(argc) {
    case 2:
        if(strlen(argv[1]) > 100){
            fprintf(stderr, "The <output file> specified exceeded 100 characters.\n");
            fprintf(stderr, "Exiting program...\n");
            exit(1);
        }
        strncpy(inputFileName, argv[1], 100);
        break;
    case 3:
        if(strlen(argv[1]) > 100){
            fprintf(stderr, "The <input file> specified exceeded 100 characters.\n");
            fprintf(stderr, "Exiting program...\n");
            exit(1);
        }
        if(strlen(argv[2]) > 100){
            fprintf(stderr, "The <output file> specified exceeded 100 characters.\n");
            fprintf(stderr, "Exiting program...\n");
            exit(1);
        }
        strncpy(inputFileName, argv[1], 100);
        strncpy(outputFileName, argv[2], 100);
        break;
    default:
        break;
  }
}