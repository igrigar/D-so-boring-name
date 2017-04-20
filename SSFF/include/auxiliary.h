/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	01/03/2017
 */

int namei(char *name);
int ialloc();
int ifree(int i);
int balloc();
int bfree(int b);
int bmap(int i, int offset);
uint32_t computeCRC(int i, int type);
