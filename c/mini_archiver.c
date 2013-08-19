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

#define VERSION "v0.0.1"

#define MAX_FILE_PATH_LEN (0xffff >> 1)
#define DIR_MARK_BIT (1 << 15)
#define ARCHIVE_VERSION 1
#define BUFFER_SIZE (1024 * 32)
#define GZ_BUFFER_SIZE (1024 * 128)

#define DEBUG 0

static void archive_internal (const char *root_path);
static void archive_dir (const char *dir_path, int path_start_idx);
static void archive_file(const char *file_path, int path_start_idx);
static const char *make_path_name(const char *dir, size_t dir_len, const char *path_name, size_t path_name_len);
static void write_path_name (const char *path_name, int is_dir);
static void write_file (const char *if_name);

static void unarchive_internal ();
static void unarchive_dir (int16_t path_len);
static void unarchive_file (int16_t path_len);
static const char *read_unarchived_path (int16_t path_len);
static void mkdirs (const char *dir);

static int get_file_stat(const char *if_name, struct stat *stat_buf);
static int is_file(struct stat *stat_buf);
static int is_dir(struct stat *stat_buf);

static size_t read_bytes (const char *buf, size_t len);
static void write_bytes (const char *data, size_t len);
static int read_int16_le (int16_t *n);
static int read_int32_le (int32_t *n);

static void write_int16_le (int16_t n);
static void write_int32_le (int32_t n);
static int is_little_endian();

static void print_help ();

static char buffer[BUFFER_SIZE];
static gzFile ar_file;
static int use_compress = 0;
static int verbose = 0;
static int ar_file_inode_no = 0;

void archive_internal (const char *root_path) {
    if (strstr(root_path, "..")) {
        fprintf(stderr, "Error: Path '%s' contains '..'\n", root_path);
        exit(1);
    }

    int path_start_idx = 0;
    if (*root_path == '/') {
        if (verbose)
            printf("removing leading '/' for path: %s\n", root_path);
        while (*(root_path + path_start_idx) == '/') {
            ++path_start_idx;
        }

        if (*(root_path + path_start_idx) == '\0') {
            fprintf(stderr, "Fatal: Archiving the root directory is not allowed: %s\n", root_path);
            exit(1);
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
            archive_dir(ar_root_path, path_start_idx);
        } else if (is_file(&stat_buf)) {
            archive_file(ar_root_path, path_start_idx);
        } else {
            fprintf(stderr, "Warning: Not a regular file or directory: %s\n", root_path); 
        }
    } else {
        perror(NULL);
        exit(1);
    }

    free(ar_root_path);
}

void archive_dir (const char *dir_path, int path_start_idx) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dir_path)) != NULL) {
        write_path_name(dir_path + path_start_idx, 1); 

        if (verbose)
            printf("archiving dir: %s\n", dir_path + path_start_idx);

        size_t dir_len = strlen(dir_path);

        while ((ent = readdir (dir)) != NULL) {
            if ((ent->d_type & DT_DIR) > 0) {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
#ifdef __APPLE__
                    uint8_t namlen = ent->d_namlen;
#else
                    uint8_t namlen = strlen(ent->d_name);
#endif
                    const char *new_path_name = make_path_name(dir_path, dir_len, ent->d_name, namlen);

                    archive_dir(new_path_name, path_start_idx); 

                    free((char *)new_path_name);
                }
            } else if ((ent->d_type & DT_REG) > 0) {
#ifdef __APPLE__
                uint8_t namlen = ent->d_namlen;
#else
                uint8_t namlen = strlen(ent->d_name);
#endif
                const char *new_path_name = make_path_name(dir_path, dir_len, ent->d_name, namlen);

                archive_file(new_path_name, path_start_idx);

                free((char *)new_path_name);
            }
        }
        closedir (dir);
    }  
}

void archive_file(const char *file_path, int path_start_idx) {
    struct stat stat_buf;    
    if (get_file_stat(file_path, &stat_buf)) {
        if (stat_buf.st_ino == ar_file_inode_no) {
            fprintf(stderr, "ma: %s: Can't add archive to itself\n", file_path);
            return;
        }

        write_path_name(file_path + path_start_idx, 0);
        if (verbose)
            printf("archiving file: (%lld) %s\n", stat_buf.st_size, file_path);

        write_int32_le(stat_buf.st_size);
        write_file(file_path);
    }
}

void write_path_name (const char *path_name, int is_dir) {
    size_t len = strlen(path_name);
    if (is_dir)
        write_int16_le(len | DIR_MARK_BIT);
    else
        write_int16_le(len);

    write_bytes(path_name, len);
}

void write_file (const char *if_name) {
    FILE *infile = fopen(if_name, "rb");
    size_t len_read;
    while (!feof(infile)) {
        while((len_read = fread(buffer, 1, BUFFER_SIZE, infile)) > 0) {
            write_bytes(buffer, len_read);
        }
    }
    fclose(infile);
}

void write_bytes (const char *data, size_t len) {
    if (len > 0 && gzwrite(ar_file, data, len) != len) {
        fprintf(stderr, "failed to write to file: %s\n", strerror(errno));
        exit(1);
    }
}

size_t read_bytes (const char *buf, size_t len) {
    return gzread(ar_file, (voidp)buf, len);
}

void write_int16_le (int16_t n) {
    if (!is_little_endian()) {
        int16_t tmp = n;
        /*swap the byte order if it is big endian*/
        n = ((tmp << 8) & 0xff00);
        n |= (tmp >> 8) & 0xff;
    }
    write_bytes((voidp)&n, 2);
}

void write_int32_le (int32_t n) {
    if (!is_little_endian()) {
        int32_t tmp = n;
        /*swap the byte order if it is big endian*/
        n = ((tmp << 24) & 0xff000000);
        n |= ((tmp << 8) & 0xff0000);
        n |= ((tmp >> 8) & 0xff00);
        n |= ((tmp >> 24) & 0xff);
    }

    write_bytes((voidp)&n, 4);
}

int read_int16_le (int16_t *n) {
    if (is_little_endian()) {
        return read_bytes((voidp)n, 2);
    } else {
        int16_t tmp;
        size_t len_read = read_bytes((voidp)&tmp, 2);
        if (len_read <= 0)
            return len_read;

        /*swap the byte order if it is big endian*/
        *n = ((tmp << 8) & 0xff00);
        *n |= (tmp >> 8) & 0xff;

        return len_read;
    }
}

int read_int32_le (int32_t *n) {
    if (is_little_endian()) {
        return read_bytes((voidp)n, 4);
    } else {
        int32_t tmp;
        size_t len_read = read_bytes((voidp)&tmp, 4);
        if (len_read <= 0)
            return len_read;

        /*swap the byte order if it is big endian*/
        *n = ((tmp << 24) & 0xff000000);
        *n |= ((tmp << 8) & 0xff0000);
        *n |= ((tmp >> 8) & 0xff00);
        *n |= ((tmp >> 24) & 0xff);

        return len_read;
    }
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

const char *make_path_name(const char *dir, size_t dir_len, const char *path_name, size_t path_name_len) {
    char *new_path_name = (char *) malloc(dir_len + 1 + path_name_len + 1);
    new_path_name[0] = '\0';
    strcat(new_path_name, dir);
    strcat(new_path_name, "/");
    strcat(new_path_name, path_name);

    return new_path_name;
}

void unarchive_internal () {
    while (1) {
        int16_t path_len;
        if (read_int16_le(&path_len) <= 0)
            break;  /* reach the end of the archive */

        if ((path_len & DIR_MARK_BIT) > 0) {
            unarchive_dir((path_len & MAX_FILE_PATH_LEN));        
        } else {
            unarchive_file(path_len);        
        }
    }
}

void unarchive_dir (int16_t path_len) {
    const char *new_path = read_unarchived_path(path_len);

    if (verbose)
        printf("unarchiving dir: (%d), %s\n", path_len, new_path);

    mkdirs(new_path);

    free((char *)new_path);
}

void unarchive_file (int16_t path_len) {
    const char *new_path = read_unarchived_path(path_len);

    char *last_slash = strrchr(new_path, '/');
    if (last_slash && last_slash != new_path) {
        // if new_path is not a file located at the root dir "/"
        // make sure the parent dirs of the current file(new_path) is created.
        *last_slash = '\0';
        mkdirs(new_path);
        *last_slash = '/';
    }

    int32_t file_len;
    read_int32_le(&file_len);

    if (verbose)
        printf("unarchiving file: (%d), %s\n", file_len, new_path);

    FILE *out_file = fopen(new_path, "wb");
    if (out_file) {
        size_t len_read = 0, total_read = 0, rest;
        while (total_read < file_len) {
            rest = file_len - total_read;

            len_read = read_bytes(buffer, rest < BUFFER_SIZE ? rest : BUFFER_SIZE);
            total_read += len_read;
            fwrite(buffer, 1, len_read, out_file);
        }
        fclose(out_file);
    } else {
        fprintf(stderr, "failed to open '%s' for write: %s\n", new_path, strerror(errno));
        exit(1);
    }

    free((char *)new_path);
}

const char *read_unarchived_path (int16_t path_len) {
    char *new_path = (char *) malloc(path_len + 1);
    new_path[path_len] = '\0';

    size_t len_read = 0;
    while (len_read < path_len) {
        len_read += read_bytes(new_path + len_read, path_len - len_read);
    }

    return new_path;
}

void mkdirs (const char *dir) {
    char *ch = (char *)dir;
    char *last_ch = ch;

    while (*ch == '/') {
        ++ch;
    }   

    while ((ch = strchr(ch, '/'))) {
        while (*(ch + 1) == '/') {
            ++ch;
        }
        *ch = '\0';
        if (mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
            fprintf(stderr, "failed to create dir '%s': %s\n", dir, strerror(errno));

            *ch = '/';
            exit(1);
        }   

        *ch = '/';
        ++ch;
        last_ch = ch; 
    }   

    if (*last_ch != '\0' && mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
        fprintf(stderr, "failed to create dir '%s': %s\n", dir, strerror(errno));
        exit(1);
    }   
}

int is_little_endian() {
    union {
        uint32_t i;
        char c[4];
    } uint32_uni = {0x01020304};

    return uint32_uni.c[0] == 4; 
}

void print_help () {
    printf("usage: mar [options] ...\n\n");
    printf("optins:\n");
    printf("    -c: archive\n");
    printf("    -x: unarchive\n");
    printf("    -z: use gzip compression when archiving\n");
    printf("    -f: archive file(for archiving or unarchiving)\n");
    printf("    -C: output directory for unarchiving\n");
    printf("    -0: no compression but keep gzip header, this option is applied only when -z is specified.\n");
    printf("    -1 to -9: compress faster to compress better, this option is applied only when -z is specified.\n");
    printf("    -v: verbose\n\n");
    printf("examples:\n");
    printf("    mar -czf foo.mar.gz dir1 dir2 file1 file2\n");
    printf("    mar -xf foo.mar.gz -C outdir\n");
    printf("    mar -c dir1 dir2 | gzip -c > foo.mar.gz\n\n");
    printf("version: %s, all right reserved @neevek\n\n", VERSION);
}

int main(int argc, char *argv[]) {
    int archive = 0;
    int unarchive = 0;
    char compress_level = '9';
    char *file_path = NULL;
    char *output_dir = NULL;
    int c, error = 0, digits = 0;
    while ((c = getopt(argc, argv, "zcxvf:C:0123456789")) != -1) {
        switch (c) {
            case 'f':
                file_path = optarg;
                break;
            case 'c':
                archive = 1;
                break;
            case 'x':
                unarchive = 1;
                break;
            case 'z':
                use_compress = 1;
                break;
            case 'C':
                output_dir = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                compress_level = c;
                ++digits;
                break;
            case '?':
                ++error;
                break;
        }
    }

    if (error > 0) {
        exit(1);
    }

    if (archive && unarchive) {
        fprintf(stderr, "%s: can't specify both -c and -x\n", argv[0]);
        print_help();
        exit(1); 
    }

    if (!archive && !unarchive) {
        fprintf(stderr, "%s: must specify either -c or -x\n", argv[0]);
        print_help();
        exit(1); 
    }

    if (archive && optind >= argc) {
        fprintf(stderr, "%s: no files or directories specified\n", argv[0]);
        print_help();
        exit(1); 
    }

    if (digits > 1) {
        fprintf(stderr, "%s: can specify only one compression level\n", argv[0]);
        print_help();
        exit(1); 
    }

    char *mode;
    if (archive) {
        if (use_compress) {
            char tmp_mode[4] = "wb"; 
            tmp_mode[2] = compress_level; 
            tmp_mode[3] = '\0';
            mode = tmp_mode;
        } else {
            mode = "wbT";
        }

    } else {
        mode = "rb";
        if (output_dir == NULL) {
            output_dir = ".";
        }
    }

    if (file_path) {
        ar_file = gzopen(file_path, mode);
    } else {
        ar_file = gzdopen(archive ? 1 : 0, mode);
    }

    if (ar_file) {
        gzbuffer(ar_file, GZ_BUFFER_SIZE);

        if (archive) {
            if (file_path) {
                struct stat ar_file_stat;
                if (get_file_stat(file_path, &ar_file_stat)) {
                    ar_file_inode_no = ar_file_stat.st_ino;
                }
            }

            write_int16_le(ARCHIVE_VERSION);
            while (optind < argc) {
                archive_internal(argv[optind++]);
            }
        } else {
            if (chdir(output_dir) != 0) {
                fprintf(stderr, "failed to chdir to '%s': %s\n", output_dir, strerror(errno));
                exit(1);
            }

            /*ignore the version number*/
            int16_t version; 
            read_int16_le(&version);

            unarchive_internal();
        }

        gzclose(ar_file);
    } else {
        fprintf(stderr, "failed to open file '%s': %s\n", file_path ? file_path : "stdin", strerror(errno));
        exit(1);
    }

    return 0;
}
