// Copyright (c) 2016 Brian Barto
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the MIT License. See LICENSE for more details.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "main.h"
#include "modules/mirrorfield.h"

/*
 * MODULE DESCRIPTION
 * 
 * The mirrorfield module manages the loading, validating, and traversing
 * of the mirror field. The cryptographic algorithm is implemented here.
 * If the debug flag is set then this module also draws the mirror field
 * and animates the encryption process.
 */

#define MIRROR_NONE      3
#define MIRROR_FORWARD   0
#define MIRROR_STRAIGHT  1
#define MIRROR_BACKWARD  2
#define DIR_DOWN         1
#define DIR_LEFT         2
#define DIR_RIGHT        3
#define DIR_UP           4

// Static Variables
static int grid[GRID_SIZE * GRID_SIZE];
static unsigned char perimeterChars[GRID_SIZE * 4];

/*
 * The mirrorfield_init() function initializes any static variables.
 */
void mirrorfield_init(void) {
	// Init grid values to zero
	memset(grid, 0, sizeof(grid));
	
	// Init perimeter chars to zero
	memset(perimeterChars, 0, sizeof(perimeterChars));
}

/*
 * The mirrorfield_set() function accepts one character at a time and
 * loads them sequentially into the static variables that contain the
 * mirror field and perimeter characters.
 * 
 * Zero is returned if it gets a character it doesn't expect, although
 * this is just a cursory error checking process. 
 */
int mirrorfield_set(unsigned char ch) {
	static int i = -1;
	int t;

	// Increment our static counter
	++i;
	
	// Set Mirror Char
	if (i < GRID_SIZE * GRID_SIZE) {
		t = ((i / GRID_SIZE) * GRID_SIZE) + (i % GRID_SIZE);
		if (ch == '/') {
			grid[t] = MIRROR_FORWARD;
		} else if (ch == '\\') {
			grid[t] = MIRROR_BACKWARD;
		} else if (ch == '-') {
			grid[t] = MIRROR_STRAIGHT;
		} else if (ch == ' ') {
			grid[t] = MIRROR_NONE;
		} else {
			return 0;
		}
		return 1;
	}
	
	// Set Inner Perimeter Chars
	t = i - (GRID_SIZE * GRID_SIZE);
	if (t < GRID_SIZE * 4) {
		perimeterChars[t] = ch;
		return 1;
	}
	
	return 0;
}

/*
 * The mirrorfield_validate() function checks that the data contained in
 * the static variables for the mirror field and perimeter character is
 * valid.
 * 
 * Zero is returned if invalid.
 */
int mirrorfield_validate(void) {
	int i, i2;

	// Check mirrors
	for (i = 0; i < GRID_SIZE * GRID_SIZE; ++i) {
		if (grid[i] < 0 || grid[i] > 3) {
			return 0;
		}
	}
	
	// Check for duplicate perimeter chars
	for (i = 0; i < GRID_SIZE * 4; ++i) {
		for (i2 = i+1; i2 < GRID_SIZE * 4; ++i2) {
			if (perimeterChars[i] == perimeterChars[i2]) {
				return 0;
			}
		}
	}
	
	return 1;
}

/*
 * The mirrorfield_crypt_char() function receives a cleartext character
 * and traverses the mirror field to find it's cyphertext equivelent,
 * which is then returned. It also calls mirrorfield_roll_chars() after
 * the cyphertext character is determined.
 */
unsigned char mirrorfield_crypt_char(unsigned char ch, int debug) {
	int r, c, t;
	unsigned char ech;
	int direction = 0;
	int startCharPos;
	int endCharPos = -1;
	int visited[GRID_SIZE * GRID_SIZE];
	
	static int evenodd = 0;
	
	// Toggle evenodd var
	evenodd = (evenodd + 1) % 2;
	
	// Init visited array to all zeros
	memset(visited, 0, sizeof(visited));
	
	// For the debug flag
	struct timespec ts;
	ts.tv_sec = debug / 1000;
	ts.tv_nsec = (debug % 1000) * 1000000;
	
	// Determine perimeter char location
	for (startCharPos = 0; startCharPos < GRID_SIZE * 4; ++startCharPos)
		if (perimeterChars[startCharPos] == ch)
			break;

	// Determining starting row/col and starting direction
	if (startCharPos < GRID_SIZE) {
		direction = DIR_DOWN;
		r = 0;
		c = startCharPos;
	} else if (startCharPos < GRID_SIZE * 2) {
		direction = DIR_LEFT;
		r = startCharPos - GRID_SIZE;
		c = GRID_SIZE - 1;
	} else if (startCharPos < GRID_SIZE * 3) {
		direction = DIR_RIGHT;
		r = startCharPos - (GRID_SIZE * 2);
		c = 0;
	} else if (startCharPos < GRID_SIZE * 4) {
		direction = DIR_UP;
		r = GRID_SIZE - 1;
		c = startCharPos - (GRID_SIZE * 3);
	}

	// Traverse through the grid
	while (endCharPos < 0) {
		
		// Translate row/column to position in grid
		t = (r * GRID_SIZE) + c;
		
		// Draw mirror field if debug flag is set
		if (debug) {
			mirrorfield_draw(r, c);
			fflush(stdout);
			nanosleep(&ts, NULL);
		}
		
		// If we already encountered this mirror, unspin it before
		// changing direction. We can only spin mirrors once per char.
		if (visited[t]) {
			grid[t] = (grid[t] + 2) % 3;
		}

		// Change direction if we hit a mirror
		switch (grid[t]) {
			case MIRROR_FORWARD:
				switch (direction) {
					case DIR_DOWN:
						direction = DIR_LEFT;
						break;
					case DIR_LEFT:
						direction = DIR_DOWN;
						break;
					case DIR_RIGHT:
						direction = DIR_UP;
						break;
					case DIR_UP:
						direction = DIR_RIGHT;
						break;
				}
				break;
			case MIRROR_BACKWARD:
				switch (direction) {
					case DIR_DOWN:
						direction = DIR_RIGHT;
						break;
					case DIR_LEFT:
						direction = DIR_UP;
						break;
					case DIR_RIGHT:
						direction = DIR_DOWN;
						break;
					case DIR_UP:
						direction = DIR_LEFT;
						break;
				}
				break;
				
		}
		
		// Spin mirror and mark as visited
		if (grid[t] != MIRROR_NONE) {
			grid[t] = (grid[t] + 1) % 3;
			visited[t] = 1;
		}

		// Advance position and check if we are out of grid bounds. If yes, we found our end character position.
		switch (direction) {
			case DIR_DOWN:
				if (++r == GRID_SIZE) {
					endCharPos = c + (GRID_SIZE * 3);
				}
				break;
			case DIR_LEFT:
				if (--c == -1) {
					endCharPos = r + (GRID_SIZE * 2);
				}
				break;
			case DIR_RIGHT:
				if (++c == GRID_SIZE) {
					endCharPos = r + GRID_SIZE;
				}
				break;
			case DIR_UP:
				if (--r == -1) {
					endCharPos = c;
				}
				break;
		}
	}
	
	// Get ending character from position
	ech = perimeterChars[endCharPos];

	// This is a way of returning the cleartext char as the cyphertext char and still preserve decryption.
	if ((int)perimeterChars[startCharPos] == startCharPos || (int)perimeterChars[endCharPos] == endCharPos) {
		if (evenodd) {
			ech = perimeterChars[startCharPos];
		}
	}
	
	// Roll start and end chars
	mirrorfield_roll_chars(startCharPos, endCharPos);

	// Return crypted char
	return ech;
}

/*
 * The mirrorfield_roll_chars() function received the starting and ending
 * positions of the cleartext and cyphertext character respectively, and
 * implements a character rolling process to make the perimeter character
 * positions dynamic and increase randomness in the output.
 * 
 * No value is returned.
 */
void mirrorfield_roll_chars(int startCharPos, int endCharPos) {
	int startRollCharPos;
	int endRollCharPos;
	unsigned char tempChar;
	
	static int lastStartCharPos = -1;
	static int lastEndCharPos = -1;

	// Determine start and end roll chars
	if (startCharPos == 0) {
		startRollCharPos = (startCharPos + (int)perimeterChars[startCharPos] + (int)perimeterChars[startCharPos + 1]) % (GRID_SIZE * 4);
	} else {
		startRollCharPos = (startCharPos + (int)perimeterChars[startCharPos] + (int)perimeterChars[startCharPos - 1]) % (GRID_SIZE * 4);
	}
	if (endCharPos == 0) {
		endRollCharPos = (endCharPos + (int)perimeterChars[endCharPos] + (int)perimeterChars[endCharPos + 1]) % (GRID_SIZE * 4);
	} else {
		endRollCharPos = (endCharPos + (int)perimeterChars[endCharPos] + (int)perimeterChars[endCharPos - 1]) % (GRID_SIZE * 4);
	}
	
	// Characters can't roll to their own position, to the other char position, or to either previous position
	while (startRollCharPos == startCharPos || startRollCharPos == endCharPos || startRollCharPos == lastStartCharPos || startRollCharPos == lastEndCharPos) {
		startRollCharPos = (startRollCharPos + (GRID_SIZE/2)) % (GRID_SIZE * 4);
	}
	while (endRollCharPos == endCharPos || endRollCharPos == startCharPos || endRollCharPos == lastEndCharPos || endRollCharPos == lastStartCharPos) {
		endRollCharPos = (endRollCharPos + (GRID_SIZE/2)) % (GRID_SIZE * 4);
	}
	
	// Roll the larger of the start/end chars first. This only matters if their
	// roll position is the same.
	if ((int)perimeterChars[startCharPos] > (int)perimeterChars[endCharPos]) {

		// Roll start char
		tempChar = perimeterChars[startCharPos];
		perimeterChars[startCharPos] = perimeterChars[startRollCharPos];
		perimeterChars[startRollCharPos] = tempChar;
		
		// Roll end char
		tempChar = perimeterChars[endCharPos];
		perimeterChars[endCharPos] = perimeterChars[endRollCharPos];
		perimeterChars[endRollCharPos] = tempChar;
		
	} else {
		
		// Roll end char
		tempChar = perimeterChars[endCharPos];
		perimeterChars[endCharPos] = perimeterChars[endRollCharPos];
		perimeterChars[endRollCharPos] = tempChar;
		
		// Roll start char
		tempChar = perimeterChars[startCharPos];
		perimeterChars[startCharPos] = perimeterChars[startRollCharPos];
		perimeterChars[startRollCharPos] = tempChar;
	}
		
	// Remember start/end position for next char
	lastStartCharPos = startCharPos;
	lastEndCharPos = endCharPos;
	
}

/*
 * The mirrorfield_draw() function draws the current state of the mirror
 * field and perimeter characters. It receives x/y coordinates and highlights
 * that position on the field.
 */
void mirrorfield_draw(int pos_r, int pos_c) {
	int r, c;
	static int resetCursor = 0;
	
	// Save cursor position if we need to reset it
	// Otherwise, clear screen.
	if (resetCursor)
		printf("\033[s");
	else
		printf("\033[2J");

	// Set cursor position to 0,0
	printf("\033[H");
	
	for (r = -1; r <= GRID_SIZE; ++r) {

		for (c = -1; c <= GRID_SIZE; ++c) {
			
			// Highlight cell if r/c match
			if (r == pos_r && c == pos_c) {
				printf("\x1B[30m"); // foreground black
				printf("\x1B[47m"); // background white
			}
			
			// Print apropriate mirror field character
			if (r == -1 && c == -1)                // Upper left corner
				printf("%2c", ' ');
			else if (r == -1 && c == GRID_SIZE)         // Upper right corner
				printf("%2c", ' ');
			else if (r == GRID_SIZE && c == -1)         // Lower left corner
				printf("%2c", ' ');
			else if (r == GRID_SIZE && c == GRID_SIZE)  // Lower right corner
				printf("%2c", ' ');
			else if (r == -1)                           // Top chars
				printf("%2x", perimeterChars[c]);
			else if (c == GRID_SIZE)                    // Right chars
				printf("%2x", perimeterChars[r + GRID_SIZE]);
			else if (r == GRID_SIZE)                    // Bottom chars
				printf("%2x", perimeterChars[c + (GRID_SIZE * 3)]);
			else if (c == -1)                           // Left chars
				printf("%2x", perimeterChars[r + (GRID_SIZE * 2)]);
			else if (grid[(r * GRID_SIZE) + c] == MIRROR_FORWARD)
				printf("%2c", '/');
			else if (grid[(r * GRID_SIZE) + c] == MIRROR_BACKWARD)
				printf("%2c", '\\');
			else if (grid[(r * GRID_SIZE) + c] == MIRROR_STRAIGHT)
				printf("%2c", '-');
			else
				printf("%2c", ' ');
			
			// Un-Highlight cell if r/c match
			if (r == pos_r && c == pos_c)
				printf("\x1B[0m");

		}
		printf("\n");
	}
	printf("\n");
	
	// Restore cursor position if we need to
	if (resetCursor)
		printf("\033[u");
	else
		resetCursor = 1;
}


