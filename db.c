#include "db.h"


size_t readChunk(unsigned char *chunk, size_t size, const char *filename)
{
    FILE *fp;
    memset(chunk, '\0', size);
    fp = fopen(filename, "rb");
    size_t bytesRead = 0;
    if (fp != NULL)
    {
        bytesRead = fread(chunk, 1, size, fp);
        fclose(fp);
        fp = NULL;
        return bytesRead;
    }else{
        return 0;
    }
}

int writeChunk(const unsigned char *chunk, size_t size, const char *filename)
{
    FILE *fp;
    fp = fopen(filename, "wb");
    if (fp != NULL)
    {
        fwrite(chunk, 1, size, fp);
        fclose(fp);
        fp = NULL;
        return 1;
    }else{
        return 0;
    }
}

int init_db(sqlite3 *db)
{
    char *sql;
    char *err_msg = 0;
    int rc;

    sql = "DROP TABLE IF EXISTS Blocks;"
          "CREATE TABLE Blocks(Id INTEGER PRIMARY KEY, Seed INTEGER, Data BLOB);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK )
    {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);

        return rc;
    }

    return rc;
}

int insertBlock(sqlite3 *db, int id, int seed, unsigned char *block, int bytes)
{
    int rc;
    char* sql;
    sqlite3_stmt *pStmt;

    sql = "INSERT INTO Blocks(Id, Seed, Data) VALUES(?, ?, ?)";

    rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return rc;
    }

    sqlite3_bind_int(pStmt, 1, id);
    sqlite3_bind_int(pStmt, 2, seed);
    sqlite3_bind_blob(pStmt, 3, block, bytes, SQLITE_STATIC);

    rc = sqlite3_step(pStmt);

    if (rc != SQLITE_DONE)
    {
        printf("execution failed: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return rc;
    }

    sqlite3_finalize(pStmt);
    return rc;
}

size_t getBlock(sqlite3 *db, int id, unsigned char *block)
{
    int rc;
    size_t bytes = 0;
    char sql[50];
    sqlite3_stmt *pStmt;

    sprintf(sql,"SELECT Data FROM Blocks WHERE Id = %d", id);

    rc = sqlite3_prepare_v2(db, sql, -1, &pStmt, 0);

    if (rc != SQLITE_OK ) {

        fprintf(stderr, "Failed to prepare statement\n");
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 0;
    }

    rc = sqlite3_step(pStmt);

    if (rc == SQLITE_ROW)
    {
        bytes = sqlite3_column_bytes(pStmt, 0);
    }else{
        printf("Failed to load block.\n");
        sqlite3_close(db);
        return 0;
    }

    memset(block, '\0', (size_t) bytes);
    memcpy(block, sqlite3_column_blob(pStmt, 0), (size_t) bytes);

    rc = sqlite3_finalize(pStmt);

    return bytes;
}

int main(void) {

    sqlite3 *db;
    unsigned char block[1024];
    int rc;
    size_t bytesRead;

    rc  = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db);
        printf("Could not initialize DB.\n");
        return 1;
    }

    if (init_db(db) != SQLITE_OK)
    {
        printf("Could not initialize DB.\n");
        return 1;
    }

    bytesRead = readChunk(block, 1024, "0.chk");

    if (bytesRead < 1)
    {
        printf("Could not read file.\n");
        return 1;
    }

    if (insertBlock(db, 1, 9, block, bytesRead) != SQLITE_DONE)
    {
        printf("Could not insert block.\n");
        return 1;
    }

    bytesRead = getBlock(db, 1, block);
    if (bytesRead < 1)
    {
        printf("Could not read block.\n");
        return 1;
    }

    writeChunk(block, bytesRead, "out.chk");

    return 0;
}
