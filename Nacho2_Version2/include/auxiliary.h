/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	01/03/2017
 */

int ialloc(void);

int alloc(void);

int ifree(int inodo_id);

int free2(int block_id);

int namei ( char *fname );

int bmap ( int inodo_id, int offset );