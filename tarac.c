#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <direct.h>  // Windows için mkdir kütüphanesi
    #define make_directory(path) mkdir(path)
#else
    #include <unistd.h>  // Linux için sistem kütüphaneleri
    #define make_directory(path) mkdir(path, 0777)
#endif

// .sau uzantı kontrolü yapan yardımcı fonksiyon
int has_sau_extension(const char *filename) {
    size_t len = strlen(filename);
    if (len < 5) return 0;
    return strcmp(filename + len - 4, ".sau") == 0;
}

void handle_unpack(int argc, char *argv[]) {
    // Argüman sayısı kontrolü (-a arşiv_adı [hedef_dizin])
    if (argc < 3 || argc > 4) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        exit(0);
    }

    char *archive_name = argv[2];
    char *target_dir = (argc == 4) ? argv[3] : NULL;

    // Uzantı kontrolü
    if (!has_sau_extension(archive_name)) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        exit(0);
    }

    // Okuma modu binary (rb) olarak açılır (Windows/Linux uyumu için)
    FILE *arch = fopen(archive_name, "rb");
    if (!arch) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        exit(0);
    }

    // 1. AŞAMA: İlk 10 baytı oku ve toplam başlık boyutunu al
    char header_len_str[11];
    if (fread(header_len_str, 1, 10, arch) != 10) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(arch);
        exit(0);
    }
    header_len_str[10] = '\0';
    int total_header_len = atoi(header_len_str);

    if (total_header_len <= 10) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(arch);
        exit(0);
    }

    // 2. AŞAMA: Organizasyon (Metadata) kısmını belleğe oku
    int meta_len = total_header_len - 10;
    char *meta_buffer = malloc(meta_len + 1);
    if (fread(meta_buffer, 1, meta_len, arch) != (size_t)meta_len) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        free(meta_buffer);
        fclose(arch);
        exit(0);
    }
    meta_buffer[meta_len] = '\0';

    // 3. AŞAMA: Hedef Klasör Yönetimi
    if (target_dir != NULL) {
        struct stat st = {0};
        if (stat(target_dir, &st) == -1) {
            if (make_directory(target_dir) != 0) {
                printf("Hata: Hedef dizin oluşturulamadı!\n");
                free(meta_buffer);
                fclose(arch);
                exit(0);
            }
        }
    }

    // 4. AŞAMA: Metadata Parçalama ve Bilgileri Dizilere Alma
    char filenames[32][256];
    unsigned int permissions[32];
    long sizes[32];
    int extracted_file_count = 0;

    // Boru (|) karakterlerine göre kayıtları ayırıyoruz
    char *token = strtok(meta_buffer, "|");
    while (token != NULL) {
        if (strlen(token) > 0) {
            // Gelen token örn: "t1.txt,0666,27"
            char *comma1 = strchr(token, ',');
            if (comma1 != NULL) {
                *comma1 = '\0'; // Virgülü keserek dosya adını ayırıyoruz
                strcpy(filenames[extracted_file_count], token);
                
                char *comma2 = strchr(comma1 + 1, ',');
                if (comma2 != NULL) {
                    *comma2 = '\0'; // İkinci virgülü keserek izin bilgisini ayırıyoruz
                    
                    // İzin modunu Sekizlik (Octal - taban 8) olarak dönüştür
                    permissions[extracted_file_count] = strtol(comma1 + 1, NULL, 8);
                    // Dosya boyutunu dönüştür
                    sizes[extracted_file_count] = atol(comma2 + 1);
                    extracted_file_count++;
                }
            }
        }
        token = strtok(NULL, "|");
    }

    // 5. AŞAMA: Dosya İçeriklerini Yazma (Payload Extraction)
    // Dosya okuma göstergesini tam olarak ham verilerin başladığı yere konumlandırıyoruz
    fseek(arch, total_header_len, SEEK_SET);

    for (int i = 0; i < extracted_file_count; i++) {
        char final_path[512];
        if (target_dir != NULL) {
            sprintf(final_path, "%s/%s", target_dir, filenames[i]);
        } else {
            sprintf(final_path, "%s", filenames[i]);
        }

        FILE *out = fopen(final_path, "wb"); // Yazma modu binary
        if (!out) {
            printf("Hata: Dosya dışa aktarılamadı (%s)!\n", final_path);
            continue;
        }

        // Arşivden ilgili dosyanın boyutu kadar karakter çekip yeni dosyaya basıyoruz
        for (long j = 0; j < sizes[i]; j++) {
            int ch = fgetc(arch);
            if (ch != EOF) {
                fputc(ch, out);
            }
        }
        fclose(out);

        // İzin ataması: Linux'ta izinleri işler, Windows'ta unused-variable uyarısını engeller
        #ifndef _WIN32
            chmod(final_path, permissions[i]);
        #else
            (void)permissions[i]; 
        #endif
    }

    // Belleği ve açık dosyayı temizle
    free(meta_buffer);
    fclose(arch);
}