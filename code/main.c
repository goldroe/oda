/* ----------------------------------------------------------------------
 * Author: Carlos Rivera
 * Date: 07/22/2022
 * File: main.c
 * --------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "common.c"
#include "lex.c"
#include "oda.c"

int main(int argc, char **argv) {
	run_tests();
	return 0;
}
