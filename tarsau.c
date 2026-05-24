#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include "tarsau.h"

#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024) // 200 MB

// Bir dosyanın sadece ASCII karakterlerden oluşup oluşmadığını test eder
int is_file_ascii(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;

    int ch;
    while ((ch = fgetc(f)) != EOF) {
        if (ch < 0 || ch > 127) {
            fclose(f);
            return 0; // ASCII değil
        }
    }
    fclose(f);
    return 1; // Tamamen ASCII
}

void handle_pack(int argc, char *argv[]) {
    char *output_filename = "a.sau";
    int file_start_index = 3; // Varsayılan: tarsau -b ...

    // -o parametresi var mı kontrolü
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_filename = argv[i + 1];
                // Giriş dosyalarının başladığı index kayar
                file_start_index = i + 2; 
                break;
            } else {
                printf("Hata: -o parametresinden sonra arşiv adı belirtilmedi!\n");
                exit(0);
            }
        }
    }

    // Eğer -o yoksa ama girdiler doğrudan verilmişse index ayarla
    if (file_start_index == 3 && strcmp(argv[2], "-o") != 0) {
        file_start_index = 2;
    }

    int file_count = argc - file_start_index;
    if (file_count <= 0) {
        printf("Hata: Arşivlenecek giriş dosyası belirtilmedi!\n");
        exit(0);
    }
    if (file_count > MAX_FILES) {
        printf("Hata: Giriş dosyası sayısı en fazla %d olabilir!\n", MAX_FILES);
        exit(0);
    }

    long total_size = 0;
    struct stat st;

    // 1. AŞAMA: Doğrulama (Validation)
    for (int i = file_start_index; i < argc; i++) {
        char *filename = argv[i];

        // Dosya varlık ve boyut kontrolü
        if (stat(filename, &st) != 0) {
            printf("%s giriş dosyasının formatı uyumsuzdur!\n", filename);
            exit(0);
        }

        total_size += st.st_size;

        // Format/ASCII Kontrolü
        if (!is_file_ascii(filename)) {
            printf("%s giriş dosyasının formatı uyumsuzdur!\n", filename);
            exit(0);
        }
    }

    if (total_size > MAX_TOTAL_SIZE) {
        printf("Hata: Giriş dosyalarının toplam boyutu 200 MB'ı geçemez!\n");
        exit(0);
    }

    // 2. AŞAMA: Metadata / Organizasyon Bölümünü Hazırlama
    // Maksimum dinamik metadata alanı tahsis edelim
    char *meta_buffer = malloc(65536); 
    if (!meta_buffer) return;
    meta_buffer[0] = '\0';

    for (int i = file_start_index; i < argc; i++) {
        char entry[512];
        stat(argv[i], &st);
        // İzinleri octal maske ile al (örn: 0644)
        int mode = st.st_mode & 0777; 
        sprintf(entry, "|%s,%04o,%ld", argv[i], mode, st.st_size);
        strcat(meta_buffer, entry);
    }
    strcat(meta_buffer, "|"); // Kapanış ayırıcı

    // İlk 10 baytlık alanın hesabı (kendisi de dahil)
    int meta_content_len = strlen(meta_buffer);
    int total_header_len = 10 + meta_content_len;

    // 3. AŞAMA: Arşive Yazma
    FILE *out = fopen(output_filename, "w");
    if (!out) {
        printf("Hata: Çıktı dosyası oluşturulamadı!\n");
        free(meta_buffer);
        exit(0);
    }

    // İlk 10 baytı soluna sıfır koyarak ASCII formatında yaz
    fprintf(out, "%010d", total_header_len);
    // Organizasyon verisini yaz
    fputs(meta_buffer, out);
    free(meta_buffer);

    // Ham dosya içeriklerini art arda ekle
    for (int i = file_start_index; i < argc; i++) {
        FILE *in = fopen(argv[i], "r");
        if (in) {
            int ch;
            while ((ch = fgetc(in)) != EOF) {
                fputc(ch, out);
            }
            fclose(in);
        }
    }

    fclose(out);
}