#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int die(char *message) {
    printf("%s", message);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        die("No demo given\n");
    }
    FILE *fp = fopen(argv[1], "r");
    parse_demo(fp);
    fclose(fp);
    return 0;
}
