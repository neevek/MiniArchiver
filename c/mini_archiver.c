#include <stdio.h>
#include <stdint.h>
#include <zlib.h>   /* for compression & decompression */
#include <unistd.h> /* for getopt */
#include <string.h> /* for strlen */
#include <sys/stat.h> /* for lstat */
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>

#define MAX_FILE_PATH_LEN (0xffff >> 1)
#define DIR_MARK_BIT (1 << 15)
#define VERSION 1
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
static void write_file (const char *if_name, FILE *out_file);

static void unarchive (const char *ar_file_path, const char *output_dir);
static void unarchive_internal (FILE *ar_file, const char *output_dir);
static void unarchive_dir (FILE *ar_file, const char *output_dir, size_t dir_len, int16_t path_len);
static void unarchive_file (FILE *ar_file, const char *output_dir, size_t dir_len, int16_t path_len);
static const char *read_unarchived_path (FILE *ar_file, const char *output_dir, size_t dir_len, int16_t path_len);
static void mkdirs (const char *dir);

static int get_file_stat(const char *if_name, struct stat *stat_buf);
static int is_file(struct stat *stat_buf);
static int is_dir(struct stat *stat_buf);

static void write_int8 (int8_t n, FILE *ar_file);
static void write_int16 (int16_t n, FILE *ar_file);
static void write_int32 (int32_t n, FILE *ar_file);
static int16_t read_int16 (FILE *ar_file);
static int32_t read_int32 (FILE *ar_file);

int main(int argc, const char *argv[]) {
    /*archive("/Users/neevek/Desktop/misc////", "/Users/neevek/Desktop/a.mar", 1);*/
    archive("/Users/xiejm/Desktop/testmar/html/", "/Users/xiejm/Desktop/testmar/html.mar", 1);
    unarchive("/Users/xiejm/Desktop/testmar/html.mar", "/Users/xiejm/Desktop/testmar/outhtml");
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
    printf("archiving dir: %s\n", dir_path + path_start_idx);
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
        printf("archiving file: (%lld) %s\n", stat_buf.st_size, file_path);
#endif
    }
}

void write_int8 (int8_t n, FILE *ar_file) {
    fwrite(&n, 1, 1, ar_file);
}

void write_int16 (int16_t n, FILE *ar_file) {
    /*n = (n >> 8) & 0xff | (n << 8) & 0xff00;*/
    fwrite(&n, 2, 1, ar_file);
}

void write_int32 (int32_t n, FILE *ar_file) {
    /*n = (n >> 24) | ((n >> 8) & 0xff00) | ((n << 8) & 0xff0000) | n << 24;*/
    fwrite(&n, 4, 1, ar_file);
}

int16_t read_int16 (FILE *ar_file) {
    int16_t n;
    if (fread(&n, 2, 1, ar_file) <= 0)
        return 0;
    return n;
}

int32_t read_int32 (FILE *ar_file) {
    int32_t n;
    if (fread(&n, 4, 1, ar_file) <= 0)
        return 0;
    return n;
}

void write_path_name (const char *path_name, int is_dir, FILE *ar_file) {
    size_t len = strlen(path_name);
    if (is_dir)
        write_int16(len | DIR_MARK_BIT, ar_file);
    else
        write_int16(len, ar_file);

    fwrite(path_name, 1, len, ar_file);
}

void write_file (const char *if_name, FILE *out_file) {
    FILE *infile = fopen(if_name, "rb");
    size_t len_read;
    while (!feof(infile)) {
        len_read = fread(buffer, 1, BYTE_BUF_SIZE, infile);
        fwrite(buffer, 1, len_read, out_file);
    }
    fclose(infile);
}

int get_file_stat(const char *if_name, struct stat *stat_buf) {
    if (lstat(if_name, stat_buf) != 0) {
        perror("get_file_stat"); 
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


/* unarchive related code */

void unarchive (const char *ar_file_path, const char *output_dir) {
    FILE *ar_file = fopen(ar_file_path, "rb");
    if (!ar_file) {
        perror("unarchive"); 
        return;
    }

#if defined(DEBUG)
    printf("unarchiving: '%s' to '%s'\n", ar_file_path, output_dir);
#endif

    /*we will modify the output_dir, so we have to make a copy*/
    const char *output_dir_cpy = (char *) malloc(strlen(output_dir) + 1);
    strcpy(output_dir_cpy, output_dir);

    /* ensures the output_dir_cpy exists */
    mkdirs(output_dir_cpy);

    uint8_t byte1, byte2;
    fread(&byte1, 1, 1, ar_file);
    fread(&byte2, 1, 1, ar_file);

    fseek(ar_file, 0, SEEK_SET);
    
    if (byte1 == 0x1f && (byte2 & 0xff) == 0x8b) {
        /* handle gzip */
    }

    /*ignore the version number*/
    short version = read_int16(ar_file);

#if defined(DEBUG)
    printf("unarchiving: %s, version=%d\n", ar_file_path, version);
#endif

    unarchive_internal(ar_file, output_dir_cpy);

    fclose(ar_file);
    free(output_dir_cpy);
}

void unarchive_internal (FILE *ar_file, const char *output_dir) {
    size_t dir_len = strlen(output_dir);
    while (1) {
        int16_t path_len = read_int16(ar_file);         
        if (path_len == 0)
            break;  /* reach the end of the archive */

        if ((path_len & DIR_MARK_BIT) > 0) {
            unarchive_dir(ar_file, output_dir, dir_len, (path_len & MAX_FILE_PATH_LEN));        
        } else {
            unarchive_file(ar_file, output_dir, dir_len, path_len);        
        }
    }
}

void unarchive_dir (FILE *ar_file, const char *output_dir, size_t dir_len, int16_t path_len) {
    const char *new_path = read_unarchived_path(ar_file, output_dir, dir_len, path_len);

#if defined(DEBUG)
    printf("unarchiving DIR: (%d), %s\n", path_len, new_path);
#endif

    if (mkdir(output_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
        fprintf(stderr, "create dir '%s' failed: %s\n", output_dir, strerror(errno));
        exit(1);
    }   

    free(new_path);
}

void unarchive_file (FILE *ar_file, const char *output_dir, size_t dir_len, int16_t path_len) {
    const char *new_path = read_unarchived_path(ar_file, output_dir, dir_len, path_len);


    char *last_slash = strrchr(new_path, '/');
    if (last_slash && last_slash != new_path) {
        *last_slash = '\0';
        mkdirs(new_path);
        *last_slash = '/';
    }

    int32_t file_len = read_int32(ar_file);

#if defined(DEBUG)
    printf("unarchiving FILE: (%d), %s\n", file_len, new_path);
#endif

    FILE *out_file = fopen(new_path, "wb");
    if (out_file) {
        size_t len_read = 0, total_read = 0, rest;
        while (total_read < file_len) {
            rest = file_len - total_read;

            len_read = fread(buffer, 1, rest < BYTE_BUF_SIZE ? rest : BYTE_BUF_SIZE, ar_file);
            total_read += len_read;
            fwrite(buffer, 1, len_read, out_file);
        }
        fclose(out_file);
    } else {
        fprintf(stderr, "Failed to open '%s' for write.", new_path);
    }

    free(new_path);
}

const char *read_unarchived_path (FILE *ar_file, const char *output_dir, size_t dir_len, int16_t path_len) {
    int len = dir_len + 1 + path_len + 1;
    char *new_path = (char *) malloc(len);
    new_path[0] = '\0';
    strcat(new_path, output_dir);
    strcat(new_path, "/");
    new_path[len - 1] = '\0';

    size_t len_read = 0;
    while (len_read < path_len) {
        char *path_pointer = new_path + dir_len + 1 + len_read;
        len_read += fread(path_pointer, 1, path_len - len_read, ar_file); 
    }

    return new_path;
}

void mkdirs (const char *dir) {
    char *ch = dir, *last_ch;

    while (*ch == '/') {
        ++ch;
    }   

    while (ch = strchr(ch, '/')) {
        while (*(ch + 1) == '/') {
            ++ch;
        }   
        *ch = '\0';
        if (mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
            fprintf(stderr, "create dir '%s' failed: %s\n", dir, strerror(errno));

            *ch = '/';
            exit(1);
        }   

        *ch = '/';
        ++ch;
        last_ch = ch; 
    }   

    if (*last_ch != '\0' && mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
        fprintf(stderr, "create dir '%s' failed: %s\n", dir, strerror(errno));
        exit(1);
    }   
}
