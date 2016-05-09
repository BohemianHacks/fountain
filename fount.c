#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>


#define CHUNK_SIZE 1024
#define CHUNKS_PER_BLOCK 5
#define FILENAME_LENGTH 20

off_t fsize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}

unsigned rand_interval(const unsigned min, const unsigned max)
{
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

unsigned getCount(const unsigned long *nums)
{
    unsigned count = 0;
    for (unsigned ii = 0; ii < CHUNKS_PER_BLOCK; ii++)
    {
        if ((int)nums[ii] >= 0) count++;
    }
    return count;
}

unsigned long numDiff(const unsigned long *nums, const unsigned long *numsFound)
{
    if (!(abs(getCount(nums) - getCount(numsFound)) == 1)) return -1;
    for (unsigned long ii = 0; ii < CHUNKS_PER_BLOCK; ii++)
    {
        unsigned found = 0;
        for (unsigned jj = 0; jj < CHUNKS_PER_BLOCK; jj++)
        {
            if (((int)nums[ii] > 0) && ((int)numsFound[jj] > 0))
            {
                if (nums[ii] == numsFound[jj]) found = 1;
            }
        }
        if (!found) return nums[ii];
    }
    return -1;
}

unsigned findChunks(const unsigned long *nums, unsigned long *numsFound)
{
    unsigned count = 0;
    unsigned numCount = getCount(nums);
    char chunkNum[FILENAME_LENGTH];

    DIR *d;
    struct dirent *ent;

    for (unsigned ii = 0; ii < CHUNKS_PER_BLOCK; ii++) numsFound[ii] = -1;

    d = opendir(".");
    if (d)
    {
        while((ent = readdir(d)) != NULL)
        {
            if(strstr(ent->d_name, ".chk") != NULL)
            {
                memset(chunkNum, '\0', FILENAME_LENGTH);
                memcpy(chunkNum, ent->d_name , strstr(ent->d_name, ".chk")-ent->d_name);
                chunkNum[strstr(ent->d_name, ".chk")-ent->d_name] = '\0';
                for (unsigned ii = 0; ii < numCount; ii++)
                {
                    if (atoi(chunkNum) == (int)nums[ii])
                    {
                        for (unsigned jj = 0; jj < numCount; jj++)
                        {
                            if ((int)numsFound[jj] == -1)
                            {
                                numsFound[jj] = (unsigned long) atoi(chunkNum);
                                break;
                            }
                        }
                        count++;
                    }
                    if (count >= numCount)
                    {
                        closedir(d);
                        return numCount;
                    }
                }
            }
        }
        closedir(d);
    }
    return count;
}

unsigned getNums(unsigned long index, unsigned seed, size_t chunkCount, unsigned long *nums)
{
    srand(seed);
    for (unsigned long ii = 0; ii <= index; ii++)
    {
        for (unsigned jj = 0; jj < CHUNKS_PER_BLOCK; jj++) nums[jj] = -1;
        unsigned count = rand_interval(1, CHUNKS_PER_BLOCK);
        for (unsigned jj = 0; jj < count; jj++)
        {
            if (ii == index)
            {
                nums[jj] = (unsigned long) rand_interval(0, chunkCount-1);
            }else{
                rand_interval(0, chunkCount-1);
            }
        }
        if (ii == index) return count;
    }
    return 0;
}

int writeChunk(const unsigned char *chunk, const char *filename)
{
    FILE *fp;
    fp = fopen(filename, "wb");
    if (fp != NULL)
    {
        fwrite(chunk, 1, CHUNK_SIZE, fp);
        fclose(fp);
        fp = NULL;
        return 1;
    }else{
        return 0;
    }
}

size_t getChunk(const char *filename, unsigned char chunk[CHUNK_SIZE], unsigned offset)
{
    size_t charsRead = 0;
    FILE *inFile;
    off_t fileSize = fsize(filename);

    memset(chunk, '\0', CHUNK_SIZE);

    inFile = fopen(filename, "rb");
    if (inFile != NULL){
        fseek(inFile , offset*CHUNK_SIZE, SEEK_SET);
        if (offset*CHUNK_SIZE+CHUNK_SIZE > fileSize)
        {
            charsRead = fread(chunk, 1, fileSize-(offset*CHUNK_SIZE), inFile);
        }else{
            charsRead = fread(chunk, 1, CHUNK_SIZE, inFile);
        }
        fclose(inFile);
        inFile = NULL;

        return charsRead;
    }
    return -1;
}
size_t chew(const char *filename, unsigned seed)
{
    off_t fileSize = 0;
    size_t chunkCount = 0;
    unsigned offset = 0;
    char saveFile[FILENAME_LENGTH];
    unsigned char block[CHUNK_SIZE];
    unsigned char chunks[CHUNKS_PER_BLOCK][CHUNK_SIZE];
    FILE *fp;

    srand(seed);

    fileSize = fsize(filename);
    chunkCount = fileSize/CHUNK_SIZE;

    if (fileSize % CHUNK_SIZE > 0) chunkCount += 1;

    for (unsigned long ii = 0; ii < chunkCount*2; ii++)
    {
        unsigned count = rand_interval(1, CHUNKS_PER_BLOCK);

        for (unsigned jj = 0; jj < count; jj++)
        {
            offset = rand_interval(0, chunkCount-1);
            getChunk(filename, chunks[jj], offset);
        }
        memset(block, '\0', CHUNK_SIZE);
        for (unsigned jj = 0; jj < count; jj++)
        {
            for (size_t kk = 0; kk < CHUNK_SIZE; kk++) block[kk] ^= chunks[jj][kk];
            sprintf(saveFile, "%lu.blk", ii);
            fp = fopen(saveFile, "wb");
            if (fp != NULL){
                fwrite(block, 1, CHUNK_SIZE, fp);
                fclose(fp);
                fp = NULL;
            }else{
                printf("Error saving block: %s", saveFile);
                exit(EXIT_FAILURE);
            }
        }
    }
    return chunkCount;
}

size_t readChunk(unsigned char *chunk, const char *filename)
{
    FILE *fp;
    memset(chunk, '\0', CHUNK_SIZE);
    fp = fopen(filename, "rb");
    size_t bytesRead = 0;
    if (fp != NULL)
    {
        bytesRead = fread(chunk, 1, CHUNK_SIZE, fp);
        fclose(fp);
        fp = NULL;
        return bytesRead;
    }else{
        return 0;
    }
}


void xorChunks(unsigned char *result, unsigned char *inOne, unsigned char *inTwo)
{
    for (size_t ii = 0; ii < CHUNK_SIZE; ii++)
    {
        result[ii] = inOne[ii] ^ inTwo[ii];
    }
}

size_t singleChunk(size_t chunkCount, unsigned long seed)
{
    char filename[FILENAME_LENGTH];
    unsigned char block[CHUNK_SIZE];
    unsigned long nums[CHUNKS_PER_BLOCK];
    size_t changeCount = 0;

    DIR *d;
    struct dirent *ent;

    d = opendir(".");
    if (d)
    {
        while((ent = readdir(d)) != NULL)
        {
            if(strstr(ent->d_name, ".blk") != NULL)
            {
                char blockNum[FILENAME_LENGTH];
                memcpy(blockNum, ent->d_name , strstr(ent->d_name, ".blk")-ent->d_name);
                blockNum[strstr(ent->d_name, ".blk")-ent->d_name] = '\0';
                getNums(atoi(blockNum), seed, chunkCount, nums);
                if (getCount(nums) == 1)
                {
                    if (readChunk(block, ent->d_name))
                    {
                        memset(filename, '\0', FILENAME_LENGTH);
                        sprintf(filename, "%lu.chk", nums[0]);
                        if (access(filename, F_OK) == -1)
                        {
                            if (writeChunk(block, filename))
                            {
                                remove(ent->d_name);
                                changeCount++;
                            }
                        }
                    }
                }
            }
        }
        closedir(d);
        return changeCount;
    }
    return 0;
}

size_t multiChunk(size_t chunkCount, unsigned long seed)
{
    char filename[FILENAME_LENGTH];
    unsigned char block[CHUNK_SIZE];
    unsigned char chunk[CHUNK_SIZE];
    unsigned long nums[CHUNKS_PER_BLOCK];
    unsigned long numsFound[CHUNKS_PER_BLOCK];
    size_t changeCount = 0;

    DIR *d;
    struct dirent *ent;


    d = opendir(".");
    if (d)
    {
        while((ent = readdir(d)) != NULL)
        {
            if(strstr(ent->d_name, ".blk") != NULL)
            {
                char blockNum[FILENAME_LENGTH];
                unsigned foundCount = 0;
                memcpy(blockNum, ent->d_name , strstr(ent->d_name, ".blk")-ent->d_name);
                blockNum[strstr(ent->d_name, ".blk")-ent->d_name] = '\0';
                getNums(atoi(blockNum), seed, chunkCount, nums);

                foundCount = findChunks(nums, numsFound);
                if (getCount(nums) - foundCount == 1)
                {
                    readChunk(block, ent->d_name);

                    for (unsigned ii = 0; ii < foundCount; ii++)
                    {
                        memset(filename, '\0', FILENAME_LENGTH);
                        sprintf(filename, "%lu.chk", numsFound[ii]);
                        readChunk(chunk, filename);
                        xorChunks(block, block, chunk);
                    }
                    for (unsigned ii = 0; ii < getCount(nums)-1; ii++)
                    {
                        if ((int)numDiff(nums, numsFound) >= 0)
                        {
                            memset(filename, '\0', FILENAME_LENGTH);
                            sprintf(filename, "%lu.chk", numDiff(nums, numsFound));
                            if (writeChunk(block, filename))
                            {
                                changeCount++;
                                remove(ent->d_name);
                                closedir(d);
                                return changeCount;
                            }
                        }
                    }
                }
            }
        }
        closedir(d);
        return changeCount;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned seed = 9;
    size_t chunkCount = 0;
    size_t extChunks = 0;
    char filename[FILENAME_LENGTH];
    unsigned char block[CHUNK_SIZE];
    FILE *fp;


    printf("Chewing file...\n");
    chunkCount = chew("doge.jpg", seed);
    printf("Rebuilding...\n");

    extChunks += singleChunk(chunkCount, seed);
    while (extChunks < chunkCount)
    {
        extChunks += multiChunk(chunkCount, seed);
        printf("%lu/%lu\n", extChunks, chunkCount);
    }
    //rebuild from chunks
    memset(filename, '\0', FILENAME_LENGTH);
    sprintf(filename, "out.jpg");
    if (access(filename, F_OK) != -1) remove(filename);
    for (size_t ii = 0; ii < chunkCount - 1; ii++)
    {
        memset(filename, '\0', FILENAME_LENGTH);
        sprintf(filename, "%lu.chk", ii);

        if (!(readChunk(block, filename)))
        {
            perror("Rebuild");
            printf("%s\n", filename);
        }
        fp = fopen("out.jpg", "ab");
        if (fp)
        {
            fwrite(block, 1, CHUNK_SIZE, fp);
            fclose(fp);
            fp = NULL;
            //remove(filename);
        }
    }
    memset(filename, '\0', FILENAME_LENGTH);
    sprintf(filename, "%lu.chk", chunkCount - 1);
    fp = fopen("out.jpg", "ab");
    readChunk(block, filename);
    fwrite(block, 1, fsize("doge.jpg") % CHUNK_SIZE, fp);
    fclose(fp);
    fp = NULL;
    return EXIT_SUCCESS;
}
