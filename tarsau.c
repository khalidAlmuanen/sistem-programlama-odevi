#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("Arsivleme modu secildi.\n");
        /* Gelecek gunlerde icerik eklenecek */
    } else if (strcmp(argv[1], "-a") == 0) {
        printf("Cikarma modu secildi.\n");
        /* Gelecek gunlerde icerik eklenecek */
    } else {
        fprintf(stderr, "Hata: Gecersiz secenek '%s'.\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
