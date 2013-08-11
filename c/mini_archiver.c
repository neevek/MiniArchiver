#include <stdio.h>
#include <stdint.h>
#include <zlib.h>   /* for compression & decompression */
#include <unistd.h> /* for getopt */
#include <string.h> /* for strlen */
#include <sys/stat.h> /* for lstat */
#include <dirent.h>
#include <stdlib.h>

#define MAX_FILE_PATH_LEN (0xffff >> 1)
#define DIR_MARK_BIT (1 << 15)
#define VERSION 1234
#define BYTE_BUF_SIZE (1024 * 4)

#define DEBUG

uint8_t buffer[BYTE_BUF_SIZE];
char real_path_buffer[FILENAME_MAX];

static void archive (const char *root_path, const char *ar_file_path, int compress);
static void archive_internal (const char *root_path, FILE *ar_file, int compress);
static void archive_dir (const char *dir_path, int path_start_idx, FILE *ar_file, int compress);
static void archive_file(const char *file_path, int path_start_idx, FILE *ar_file, int compress);
static char *make_path_name(const char *dir, size_t dir_len, const char *path_name, size_t path_name_len);
static void write_path_name (const char *path_name, int is_dir, FILE *ar_file);
static void write_file (const char *if_name, FILE *ar_file);

static int get_file_stat(const char *if_name, struct stat *stat_buf);
static int is_file(struct stat *stat_buf);
static int is_dir(struct stat *stat_buf);

static void write_int8 (int8_t n, FILE *ar_file);
static void write_int16 (int16_t n, FILE *ar_file);
static void write_int32 (int32_t n, FILE *ar_file);

int main(int argc, const char *argv[]) {
    archive("/Users/neevek/Desktop/misc////", "/Users/neevek/Desktop/a.mar", 1);
    /*archive("/Users/neevek/Desktop/test////", "/Users/neevek/Desktop/a.mar", 1);*/
    return 0;
}

void archive (const char *root_path, const char *ar_file_path, int compress) {
    FILE *ar_file = fopen(ar_file_path, "wb");
    if (ar_file) {
        write_int16(VERSION, ar_file);

        archive_internal(root_path, ar_file, 1);
        
        fclose(ar_file);
    } else {
        perror("Failed to open file"); 
    }
}

void archive_internal (const char *root_path, FILE *ar_file, int compress) {
    if (strstr(root_path, "..")) {
        fprintf(stderr, "Error: Path '%s' contains '..'\n", root_path);
        return;
    }

    int path_start_idx = 0;
    if (*root_path == '/') {
        fprintf(stderr, "Removing leading '/' for path: %s\n", root_path);
        while (*(root_path + path_start_idx) == '/') {
            ++path_start_idx;
        }

        if (*(root_path + path_start_idx) == '\0') {
            fprintf(stderr, "Fatal: Archiving the root directory is not allowed.\n", root_path);
            return; 
        }
    }

    size_t root_path_len = strlen(root_path);
    char *ar_root_path = (char *) malloc(root_path_len + 1);
    strcpy(ar_root_path, root_path);

    /*removing trailing slashes*/
    while (*(ar_root_path + root_path_len - 1) == '/'){
        ar_root_path[root_path_len - 1] = '\0';
        --root_path_len;
    }

#if defined(DEBUG)
    printf(">>> archive_internal path: %s\n", ar_root_path);
#endif

    struct stat stat_buf;
    if (get_file_stat(ar_root_path, &stat_buf)) {
        if (is_dir(&stat_buf)) {
            archive_dir(ar_root_path, path_start_idx, ar_file, compress);
        } else if (is_file(&stat_buf)) {
            archive_file(ar_root_path, path_start_idx, ar_file, compress);
        } else {
            fprintf(stderr, "Warning: Not a regular file or directory: %s\n", root_path); 
        }
    } else {
        perror(NULL);
    }

    free(ar_root_path);
}

void archive_dir (const char *dir_path, int path_start_idx, FILE *ar_file, int compress) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dir_path)) != NULL) {
        write_path_name(dir_path + path_start_idx, 1, ar_file);

#if defined(DEBUG)
    printf(">>> archived dir: %s\n", dir_path + path_start_idx);
#endif

        size_t dir_len = strlen(dir_path);

        while ((ent = readdir (dir)) != NULL) {
            if ((ent->d_type & DT_DIR) > 0) {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    char *new_path_name = make_path_name(dir_path, dir_len, ent->d_name, ent->d_namlen);

                    archive_dir(new_path_name, path_start_idx, ar_file, compress); 

                    free(new_path_name);
                }
            } else if ((ent->d_type & DT_REG) > 0) {
                char *new_path_name = make_path_name(dir_path, dir_len, ent->d_name, ent->d_namlen);

                archive_file(new_path_name, path_start_idx, ar_file, compress);

                free(new_path_name);
            }
        }
        closedir (dir);
    }  
}

void archive_file(const char *file_path, int path_start_idx, FILE *ar_file, int compress) {
    write_path_name(file_path + path_start_idx, 0, ar_file);

    struct stat stat_buf;    
    if (get_file_stat(file_path, &stat_buf)) {
        write_int32(stat_buf.st_size, ar_file);
        write_file(file_path, ar_file);

#if defined(DEBUG)
        printf(">>> archived file: %lld = %s\n", stat_buf.st_size, file_path);
#endif
    }
}

void write_int8 (int8_t n, FILE *ar_file) {
    fwrite(&n, 1, 1, ar_file);
}

void write_int16 (int16_t n, FILE *ar_file) {
    /*fwrite(&n, 2, 1, ar_file);*/
    int8_t b = n >> 8;
    fwrite(&b, 1, 1, ar_file);
    b = n;
    fwrite(&b, 1, 1, ar_file);
}

void write_int32 (int32_t n, FILE *ar_file) {
    /*fwrite(&n, 4, 1, ar_file);*/
    int8_t b = n >> 24;
    fwrite(&b, 1, 1, ar_file);
    b = n >> 16;
    fwrite(&b, 1, 1, ar_file);
    b = n >> 8;
    fwrite(&b, 1, 1, ar_file);
    b = n;
    fwrite(&b, 1, 1, ar_file);
}

void write_path_name (const char *path_name, int is_dir, FILE *ar_file) {
    size_t len = strlen(path_name);
    if (is_dir)
        write_int16(len | DIR_MARK_BIT, ar_file);
    else
        write_int16(len, ar_file);

    fwrite(path_name, 1, len, ar_file);
}

void write_file (const char *if_name, FILE *ar_file) {
    FILE *infile = fopen(if_name, "rb");
    size_t len_read;
    while (!feof(infile)) {
        len_read = fread(buffer, 1, BYTE_BUF_SIZE, infile);
        fwrite(buffer, 1, len_read, ar_file);
    }
    fclose(infile);
}

int get_file_stat(const char *if_name, struct stat *stat_buf) {
    if (lstat(if_name, stat_buf) != 0) {
        perror("write_file"); 
        return 0;
    }
    return 1;
}

int is_file(struct stat *stat_buf) {
    return (stat_buf->st_mode & S_IFMT) == S_IFREG;
}

int is_dir(struct stat *stat_buf) {
    return (stat_buf->st_mode & S_IFMT) == S_IFDIR;
}

char *make_path_name(const char *dir, size_t dir_len, const char *path_name, size_t path_name_len) {
    char *new_path_name = (char *) malloc(dir_len + 1 + path_name_len + 1);
    new_path_name[0] = '\0';
    strcat(new_path_name, dir);
    strcat(new_path_name, "/");
    strcat(new_path_name, path_name);

    return new_path_name;
}
