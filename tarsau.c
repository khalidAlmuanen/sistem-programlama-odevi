#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define IO_BUF_SIZE 65536

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

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                i++;
                continue;
            }

            if (!is_ascii_text(argv[i])) {
                fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", argv[i]);
                return 1;
            }
        }
        printf("Tum dosyalar ASCII formatina uygun. Arsivleme hazir.\n");

    } else if (strcmp(argv[1], "-a") == 0) {
        printf("Cikarma modu secildi.\n");
    } else {
        fprintf(stderr, "Hata: Gecersiz secenek '%s'.\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
