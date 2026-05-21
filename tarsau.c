#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_FILES 32
#define MAX_TOTAL_BYTES (200LL * 1024LL * 1024LL)
#define IO_BUF_SIZE 65536
#define HEADER_LEN 10
#define DEFAULT_OUTPUT "a.sau"
#define SAU_EXT ".sau"

typedef struct {
    char path[PATH_MAX];
    char name[PATH_MAX];
    mode_t perms;
    off_t size;
} FileInfo;

typedef struct {
    char name[PATH_MAX];
    mode_t perms;
    off_t size;
} Entry;

static int is_ascii_text(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;

    unsigned char buf[IO_BUF_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, fp)) > 0) {
        for (size_t i = 0; i < n; i++) {
            if (buf[i] > 0x7F) {
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);
    return 1;
}

static int makedirs(const char *path) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof tmp, "%s", path);
    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
    return 0;
}

void print_usage(const char *prog) {
    fprintf(stderr, "Kullanim:\n");
    fprintf(stderr, "  %s -b dosya1 [dosya2 ...] [-o arsiv.sau]\n", prog);
    fprintf(stderr, "  %s -a arsiv.sau [hedef_dizin]\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Hata: Arsivlenecek dosya belirtilmedi.\n");
            return 1;
        }

        FileInfo files[MAX_FILES];
        int nfiles = 0;
        const char *outname = DEFAULT_OUTPUT;
        long long total_size = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) outname = argv[++i];
                continue;
            }

            if (nfiles >= MAX_FILES) {
                fprintf(stderr, "Hata: En fazla %d dosya belirtilebilir.\n", MAX_FILES);
                return 1;
            }

            struct stat st;
            if (stat(argv[i], &st) != 0 || !S_ISREG(st.st_mode)) {
                fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", argv[i]);
                return 1;
            }

            if (!is_ascii_text(argv[i])) {
                fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", argv[i]);
                return 1;
            }

            total_size += st.st_size;
            if (total_size > MAX_TOTAL_BYTES) {
                fprintf(stderr, "Hata: Toplam dosya boyutu 200MB limitini asiyor.\n");
                return 1;
            }

            strncpy(files[nfiles].path, argv[i], PATH_MAX);
            char tmp[PATH_MAX];
            strncpy(tmp, argv[i], PATH_MAX);
            strncpy(files[nfiles].name, basename(tmp), PATH_MAX);
            files[nfiles].perms = st.st_mode & 0777;
            files[nfiles].size = st.st_size;
            nfiles++;
        }

        char toc[8192] = {0};
        int toc_pos = 0;
        for (int i = 0; i < nfiles; i++) {
            toc_pos += sprintf(toc + toc_pos, "|%s,%04o,%lld|", 
                               files[i].name, (unsigned int)files[i].perms, (long long)files[i].size);
        }

        FILE *out = fopen(outname, "wb");
        if (!out) { perror(outname); return 1; }

        char header[HEADER_LEN + 1];
        sprintf(header, "%010d", toc_pos);
        fwrite(header, 1, HEADER_LEN, out);
        fwrite(toc, 1, toc_pos, out);

        unsigned char buf[IO_BUF_SIZE];
        for (int i = 0; i < nfiles; i++) {
            FILE *fp = fopen(files[i].path, "rb");
            if (fp) {
                size_t n;
                while ((n = fread(buf, 1, sizeof buf, fp)) > 0) {
                    fwrite(buf, 1, n, out);
                }
                fclose(fp);
            }
        }
        fclose(out);
        printf("Dosyalar birleştirildi.\n");

    } else if (strcmp(argv[1], "-a") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            return 1;
        }

        if (argc > 4) {
            fprintf(stderr, "Hata: -a parametresinden sonra en fazla 2 parametre alinabilir.\n");
            return 1;
        }

        const char *archive = argv[2];
        const char *destdir = (argc == 4) ? argv[3] : NULL;

        const char *ext = strrchr(archive, '.');
        if (!ext || strcmp(ext, SAU_EXT) != 0) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            return 1;
        }

        FILE *fp = fopen(archive, "rb");
        if (!fp) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            return 1;
        }

        char header[HEADER_LEN + 1] = {0};
        if (fread(header, 1, HEADER_LEN, fp) != HEADER_LEN) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            fclose(fp); return 1;
        }

        for (int i = 0; i < HEADER_LEN; i++) {
            if (!isdigit(header[i])) {
                fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
                fclose(fp); return 1;
            }
        }

        long long toc_len = atoll(header);
        if (toc_len <= 0) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            fclose(fp); return 1;
        }

        char *toc = malloc(toc_len + 1);
        if (fread(toc, 1, toc_len, fp) != (size_t)toc_len) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            free(toc); fclose(fp); return 1;
        }
        toc[toc_len] = '\0';

        if (destdir) makedirs(destdir);

        Entry entries[MAX_FILES];
        int count = 0;
        char *p = toc;
        while (*p && count < MAX_FILES) {
            if (*p == '|') {
                p++;
                char *end = strchr(p, '|');
                if (end) {
                    *end = '\0';
                    unsigned int p_oct;
                    if (sscanf(p, "%[^,],%o,%lld", entries[count].name, &p_oct, (long long *)&entries[count].size) != 3) {
                        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
                        free(toc); fclose(fp); return 1;
                    }
                    entries[count].perms = (mode_t)p_oct;
                    count++;
                    p = end + 1;
                } else break;
            } else p++;
        }

        if (count == 0) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            free(toc); fclose(fp); return 1;
        }

        unsigned char buf[IO_BUF_SIZE];
        for (int i = 0; i < count; i++) {
            char outpath[PATH_MAX];
            if (destdir) sprintf(outpath, "%s/%s", destdir, entries[i].name);
            else strcpy(outpath, entries[i].name);

            FILE *out = fopen(outpath, "wb");
            if (out) {
                off_t rem = entries[i].size;
                while (rem > 0) {
                    size_t chunk = (rem > IO_BUF_SIZE) ? IO_BUF_SIZE : (size_t)rem;
                    size_t got = fread(buf, 1, chunk, fp);
                    if (got == 0) break;
                    fwrite(buf, 1, got, out);
                    rem -= got;
                }
                fclose(out);
                chmod(outpath, entries[i].perms);
            }
        }

        printf("Dosyalar başarıyla çıkarıldı ve izinler geri yüklendi.\n");
        free(toc);
        fclose(fp);

    } else {
        fprintf(stderr, "Hata: Geçersiz seçenek '%s'.\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
    return 0;
}
