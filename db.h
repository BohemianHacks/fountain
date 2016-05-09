#ifndef _DB_H
#define _DB_H

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

int init_db(sqlite3 *db);

int insertBlock(sqlite3 *db, int id, int seed, unsigned char *block, int bytes);

size_t getBlock(sqlite3 *db, int id, unsigned char *block);

#endif


