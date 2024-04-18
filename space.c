////////////////////////////////////////////////////////////////////////
// COMP1521 24T1 --- Assignment 2: `space', a simple file archiver
// <https://www.cse.unsw.edu.au/~cs1521/24T1/assignments/ass2/index.html>
//
// Written by YOUR-NAME-HERE (z5555555) on INSERT-DATE-HERE.
//
// 2024-03-08   v1.1    Team COMP1521 <cs1521 at cse.unsw.edu.au>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "space.h"


// ADD ANY extra #defines HERE


// ADD YOUR FUNCTION PROTOTYPES (AND STRUCTS IF ANY) HERE

typedef unsigned char byte;

typedef struct
{
    byte            magicNumber;
    byte            starFormat;
    byte            permissions[11];
    byte            pathnameLength[2];
    size_t          pathnameLengthDec;
    byte*           pathname;
    byte            contentLength[6];
    size_t          contentLengthDec;
    byte*           content;
    byte            original_hash;
    byte            calculated_hash;
} starStruct;

typedef struct
{
    starStruct*    stars;
    size_t          size;
} galaxyStruct;

const byte magicNumber = 'c';
const size_t byteSize = sizeof(byte);
const size_t magicNumberSize = 1;
const size_t starFormatSize = 1;
const size_t permissionsSize = 10;
const size_t pathnameLengthSize = 2;
const size_t contentLengthSize = 6;
const size_t hashSize = 1;
const size_t starStructSize = sizeof(starStruct);

/**
 * @brief Frees the memory allocated for a galaxy.
 *
 * @param galaxy The galaxy structure to free.
 */
void freeGalaxy(galaxyStruct galaxy)
{
    for (size_t i = 0; i < galaxy.size; ++i)
    {
        if (galaxy.stars[i].pathname != NULL)
        {
            free(galaxy.stars[i].pathname);
            galaxy.stars[i].pathname = NULL;
        }
        if (galaxy.stars[i].content != NULL)
        {
            free(galaxy.stars[i].content);
            galaxy.stars[i].content = NULL;
        }
    }
    if (galaxy.stars != NULL)
    {
        free(galaxy.stars);
        galaxy.stars = NULL;
    }
}

/**
 * @brief Prints an error message, closes the given file, and exits the program.
 *
 * This function is used to handle errors that occur while reading a galaxy from a file.
 * It prints an error message using perror(), closes the file, and terminates the program with an exit status of 1.
 *
 * @param file The file being read from.
 * @param galaxy The galaxy structure being populated.
 * @param msg The error message to be printed.
 */
void printReadGalaxyError(FILE* file, galaxyStruct galaxy, const char* msg)
{
    perror(msg);
    fclose(file);
    exit(1);
}

/**
 * @brief Calculates the hash of an item.
 *
 * This function calculates the hash of an item by iterating over its bytes and
 * updating the hash value with each byte using the `galaxy_hash` function.
 *
 * @param lastHash The hash value computed for the previous items.
 * @param item A pointer to the item's byte array.
 * @param itemSize The size of the item's byte array.
 * @return The calculated hash value.
 */
byte calculateItemHash(byte lastHash, byte* item, size_t itemSize)
{
    byte hash = lastHash;
    for (size_t i = 0; i < itemSize; ++i)
    {
        hash = galaxy_hash(hash, item[i]);
    }
    return hash;
}

/**
 * @brief Calculates the hash value for a star structure.
 *
 * This function calculates the hash value for a star structure by combining
 * various fields and their byte arrays using the `galaxy_hash` function. The
 * calculated hash value is stored in the `calculated_hash` field of the `star`
 * structure.
 *
 * @param star A pointer to the star structure.
 */
void calculateHash(starStruct* star)
{
    star->calculated_hash = galaxy_hash(star->calculated_hash, star->magicNumber);
    star->calculated_hash = galaxy_hash(star->calculated_hash, star->starFormat);
    star->calculated_hash = calculateItemHash(star->calculated_hash, star->permissions, permissionsSize);
    star->calculated_hash = calculateItemHash(star->calculated_hash, star->pathnameLength, pathnameLengthSize);
    star->calculated_hash = calculateItemHash(star->calculated_hash, star->pathname, star->pathnameLengthDec);
    star->calculated_hash = calculateItemHash(star->calculated_hash, star->contentLength, contentLengthSize);
    star->calculated_hash = calculateItemHash(star->calculated_hash, star->content, star->contentLengthDec);
}

/**
 * @brief Compares the calculated hash value with the original hash value.
 *
 * This function compares the calculated hash value with the original hash value
 * stored in the `original_hash` field of the `star` structure. If the hash values
 * match, it prints "correct hash". Otherwise, it prints "incorrect hash" along
 * with the expected and actual hash values.
 *
 * @param star A pointer to the star structure.
 */
void compareHash(starStruct* star)
{
    printf("%s - ", star->pathname);
    if (star->calculated_hash == star->original_hash)
    {
        printf("correct hash\n");
    }
    else
    {
        printf("incorect hash %0#x should be %0#x\n", star->calculated_hash, star->original_hash);
    }
}

/**
 * @brief Reads a galaxy from a file.
 *
 * @param pathname The path to the file containing the galaxy data.
 * @return The galaxy structure read from the file.
 */
galaxyStruct readGalaxy(const char* pathname)
{
    galaxyStruct galaxy;
    galaxy.stars = NULL;
    galaxy.size = 0;

    FILE* file = fopen(pathname, "rb");
    if (file == NULL)
    {
        perror("Unable to open the file.\n");
        exit(1);
    }

    while (!feof(file))
    {
        starStruct new_star;

        if (fread(&new_star.magicNumber, byteSize, magicNumberSize,file) < magicNumberSize)
        {
            if (feof(file))
            {
                break;
            }

            if (ferror(file))
            {
                printReadGalaxyError(file, galaxy, "Error occurred while reading the magic number.\n");
            }
        }
        if (new_star.magicNumber != magicNumber)
        {
            printReadGalaxyError(file, galaxy, "magic number don't match.\n");
        }
        

        if (fread(&new_star.starFormat, byteSize, starFormatSize, file) < starFormatSize && ferror(file))
        {
            printReadGalaxyError(file, galaxy, "Error occurred while reading the star format.\n");
        }

        if (fread(new_star.permissions, byteSize, permissionsSize, file) < permissionsSize && ferror(file))
        {
            printReadGalaxyError(file, galaxy, "Error occurred while reading the permission.\n");
        }
        new_star.permissions[permissionsSize] = '\0';

        if (fread(new_star.pathnameLength, byteSize, pathnameLengthSize, file) < pathnameLengthSize && ferror(file))
        {
            printReadGalaxyError(file, galaxy, "Error occurred while reading the pathname length.\n");
        }
        new_star.pathnameLengthDec = ( new_star.pathnameLength[1] << 8 ) | new_star.pathnameLength[0];
        new_star.pathname = (byte*)malloc(new_star.pathnameLengthDec + 1);

        if (fread(new_star.pathname, byteSize, new_star.pathnameLengthDec, file) < new_star.pathnameLengthDec && ferror(file))
        {
            printReadGalaxyError(file, galaxy, "Error occurred while reading the pathname.\n");
        }
        new_star.pathname[new_star.pathnameLengthDec] = '\0';

        if (fread(new_star.contentLength, byteSize, contentLengthSize, file) < contentLengthSize && ferror(file))
        {
            printReadGalaxyError(file, galaxy, "Error occurred while reading the content length.\n");
        }
        new_star.contentLengthDec = 0;
        for (int i = contentLengthSize - 1; i >= 0; --i)
        {
            new_star.contentLengthDec = ( new_star.contentLengthDec << 8 ) | new_star.contentLength[i];
        }
        new_star.content = (byte*)malloc(new_star.contentLengthDec + 1);

        if (fread(new_star.content, byteSize, new_star.contentLengthDec, file) < new_star.contentLengthDec && ferror(file))
        {
            printReadGalaxyError(file, galaxy, "Error occurred while reading the content.\n");
        }
        new_star.content[new_star.contentLengthDec] = '\0';
 
        if (fread(&new_star.original_hash, byteSize, hashSize, file) < hashSize && ferror(file))
        {
            printReadGalaxyError(file, galaxy, "Error occurred while reading the hash.\n");
        }

        new_star.calculated_hash = 0;
        calculateHash(&new_star);

        ++galaxy.size;
        galaxy.stars = (starStruct*)realloc(galaxy.stars, galaxy.size * starStructSize);
        galaxy.stars[galaxy.size - 1] = new_star;
    }

    fclose(file);
    return galaxy;
}

// print the files & directories stored in galaxy_pathname (subset 0)
//
// if long_listing is non-zero then file/directory permissions, formats & sizes
// are also printed (subset 0)

/**
 * @brief Lists the contents of a galaxy.
 *
 * @param galaxy_pathname The path to the galaxy file.
 * @param long_listing Flag indicating whether to include detailed information in the listing.
 */
void list_galaxy(char *galaxy_pathname, int long_listing)
{

    galaxyStruct galaxy = readGalaxy(galaxy_pathname);
    if (galaxy.stars != NULL)
    {
        for (size_t i = 0; i < galaxy.size; ++i)
        {
            if (long_listing)
            {
                printf("%s\t%c\t%zd\t",
                    galaxy.stars[i].permissions,
                    galaxy.stars[i].starFormat,
                    galaxy.stars[i].contentLengthDec
                );
            }
            printf("%s\n", galaxy.stars[i].pathname);
        }

        freeGalaxy(galaxy);
    }
}


// check the files & directories stored in galaxy_pathname (subset 1)
//
// prints the files & directories stored in galaxy_pathname with a message
// either, indicating the hash byte is correct, or indicating the hash byte
// is incorrect, what the incorrect value is and the correct value would be

void check_galaxy(char *galaxy_pathname)
{
    galaxyStruct galaxy = readGalaxy(galaxy_pathname);
    if (galaxy.stars != NULL)
    {
        for (size_t i = 0; i < galaxy.size; ++i)
        {
            compareHash(&galaxy.stars[i]);
        }
    }

}


// extract the files/directories stored in galaxy_pathname (subset 1 & 3)

void extract_galaxy(char *galaxy_pathname) {

    // REPLACE THIS PRINTF WITH YOUR CODE

    printf("extract_galaxy called to extract galaxy: '%s'\n", galaxy_pathname);
}


// create galaxy_pathname containing the files or directories specified in
// pathnames (subset 2 & 3)
//
// if append is zero galaxy_pathname should be over-written if it exists
// if append is non-zero galaxys should be instead appended to galaxy_pathname
//                       if it exists
//
// format specifies the galaxy format to use, it must be one STAR_FMT_6,
// STAR_FMT_7 or STAR_FMT_8

void create_galaxy(char *galaxy_pathname, int append, int format,
                   int n_pathnames, char *pathnames[n_pathnames]) {

    // REPLACE THIS CODE PRINTFS WITH YOUR CODE

    printf("create_galaxy called to create galaxy: '%s'\n", galaxy_pathname);
    printf("format = %x\n", format);
    printf("append = %d\n", append);
    printf("These %d pathnames specified:\n", n_pathnames);
    for (int p = 0; p < n_pathnames; p++) {
        printf("%s\n", pathnames[p]);
    }
}


// ADD YOUR EXTRA FUNCTIONS HERE
