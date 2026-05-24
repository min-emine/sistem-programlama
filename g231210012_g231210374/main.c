#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tarsau.h"
#include "tarac.h"

int main(int argc, char *argv[]) {
    // En temel argüman kontrolü
    if (argc < 3) {
        printf("Hata: Eksik parametre girdiniz!\n");
        printf("Kullanım:\n  Arşivleme: tarsau -b [-o arsiv_adi.sau] <dosyalar...>\n  Çıkarma: tarsau -a <arsiv_adi.sau> [hedef_dizin]\n");
        return 0;
    }

    if (strcmp(argv[1], "-b") == 0) {
        handle_pack(argc, argv);
    } 
    else if (strcmp(argv[1], "-a") == 0) {
        handle_unpack(argc, argv);
    } 
    else {
        printf("Hata: Geçersiz mod seçimi! Sadece -b veya -a kullanabilirsiniz.\n");
    }

    return 0;
}