#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>

#define MAX_FILES 32
#define IO_BUF_SIZE 65536

typedef struct {
    char path[PATH_MAX];
    char name[PATH_MAX];
    mode_t perms;
    off_t size;
} FileInfo;

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

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                i++;
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
                               files[i].name, 
                               (unsigned int)files[i].perms, 
                               (long long)files[i].size);
        }

        printf("TOC Olusturuldu (%d bayt): %s\n", toc_pos, toc);

    } else if (strcmp(argv[1], "-a") == 0) {
        printf("Cikarma modu secildi.\n");
    } else {
        fprintf(stderr, "Hata: Gecersiz secenek '%s'.\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
