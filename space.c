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
    byte            magic_number;
    byte            star_format;
    byte            permissions[11];
    byte            pathname_length[2];
    byte*           pathname;
    byte            content_length[6];
    size_t          content_length_dec;
    byte*           content;
    byte hash;
} star_struct;

typedef struct
{
    star_struct*    stars;
    size_t          size;
} galaxy_struct;

const byte magic_number = 'c';
const size_t byte_size = sizeof(byte);
const size_t magic_number_size = 1;
const size_t star_format_size = 1;
const size_t permissions_size = 10;
const size_t pathname_length_size = 2;
const size_t content_length_size = 6;
const size_t hash_size = 1;
const size_t star_struct_size = sizeof(star_struct);

void print_read_file_error(FILE* file, const char* msg)
{
    perror(msg);
    fclose(file);
    exit(1);
}

/**
 * @brief Frees the memory allocated for a galaxy.
 *
 * @param galaxy The galaxy structure to free.
 */
void free_galaxy(galaxy_struct galaxy)
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
 * @brief Reads a galaxy from a file.
 *
 * @param pathname The path to the file containing the galaxy data.
 * @return The galaxy structure read from the file.
 */
galaxy_struct read_galaxy(const char* pathname)
{
    galaxy_struct galaxy;
    galaxy.stars = NULL;
    galaxy.size = 0;

    FILE* file = fopen(pathname, "rb");
    if (file == NULL)
    {
        perror("Unable to open the file.\n");
        return galaxy;
    }

    while (!feof(file))
    {
        star_struct new_star;

        if (fread(&new_star.magic_number, byte_size, magic_number_size,file) < magic_number_size)
        {
            if (feof(file))
            {
                break;
            }

            if (ferror(file))
            {
                print_read_file_error(file, "Error occurred while reading the magic number.\n");
            }
        }
        if (new_star.magic_number != magic_number)
        {
            fprintf(stderr, "magic number don't match.");
            free_galaxy(galaxy);
            exit(1);
        }

        if (fread(&new_star.star_format, byte_size, star_format_size, file) < star_format_size && ferror(file))
        {
            print_read_file_error(file, "Error occurred while reading the star format.\n");
        }

        if (fread(new_star.permissions, byte_size, permissions_size, file) < permissions_size && ferror(file))
        {
            print_read_file_error(file, "Error occurred while reading the permission.\n");
        }
        new_star.permissions[permissions_size] = '\0';

        if (fread(new_star.pathname_length, byte_size, pathname_length_size, file) < pathname_length_size && ferror(file))
        {
            print_read_file_error(file, "Error occurred while reading the pathname length.\n");
        }
        unsigned short pathname_length = ( new_star.pathname_length[1] << 8 ) | new_star.pathname_length[0];
        new_star.pathname = (byte*)malloc(pathname_length + 1);

        if (fread(new_star.pathname, byte_size, pathname_length, file) < pathname_length && ferror(file))
        {
            print_read_file_error(file, "Error occurred while reading the pathname.\n");
        }
        new_star.pathname[pathname_length] = '\0';

        if (fread(new_star.content_length, byte_size, content_length_size, file) < content_length_size && ferror(file))
        {
            print_read_file_error(file, "Error occurred while reading the content length.\n");
        }
        new_star.content_length_dec = 0;
        for (int i = content_length_size - 1; i >= 0; --i)
        {
            new_star.content_length_dec = ( new_star.content_length_dec << 8 ) | new_star.content_length[i];
        }
        new_star.content = (byte*)malloc(new_star.content_length_dec + 1);

        if (fread(new_star.content, byte_size, new_star.content_length_dec, file) < new_star.content_length_dec && ferror(file))
        {
            print_read_file_error(file, "Error occurred while reading the content.\n");
        }
        new_star.content[new_star.content_length_dec] = '\0';
 
        if (fread(&new_star.hash, byte_size, hash_size, file) < hash_size && ferror(file))
        {
            print_read_file_error(file, "Error occurred while reading the hash.\n");
        }

        ++galaxy.size;
        galaxy.stars = (star_struct*)realloc(galaxy.stars, galaxy.size * star_struct_size);
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
void list_galaxy(char *galaxy_pathname, int long_listing) {

    galaxy_struct galaxy = read_galaxy(galaxy_pathname);
    if (galaxy.stars != NULL)
    {
        for (size_t i = 0; i < galaxy.size; ++i)
        {
            if (long_listing)
            {
                printf("%s\t%c\t%zd\t",
                    galaxy.stars[i].permissions,
                    galaxy.stars[i].star_format,
                    galaxy.stars[i].content_length_dec
                );
            }
            printf("%s\n", galaxy.stars[i].pathname);
        }

        free_galaxy(galaxy);
    }
}


// check the files & directories stored in galaxy_pathname (subset 1)
//
// prints the files & directories stored in galaxy_pathname with a message
// either, indicating the hash byte is correct, or indicating the hash byte
// is incorrect, what the incorrect value is and the correct value would be

void check_galaxy(char *galaxy_pathname) {

    // REPLACE THIS PRINTF WITH YOUR CODE

    printf("check_galaxy called to check galaxy: '%s'\n", galaxy_pathname);
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
