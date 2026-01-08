#include "asm_common.h"
#include <string.h>
#include <stdlib.h>

/**
 * Pass 2 - Forward Reference Resolution (İleri Referans Çözümleme)
 * 
 * Bu fonksiyon Pass 1'den gelen .s dosyasını işleyerek:
 * 1. Forward reference'ları çözer (FRT ve ST tablolarını kullanarak)
 * 2. Final object code'u .o dosyasına yazar
 * 3. DAT ve HDRM tablolarını .t dosyasına yazar
 * 
 * .s dosyası formatı: LC(Hex)  Opcode(Hex)  [OperandBytes...]
 * Örnek: "001A  E1  00 00"  veya  "001A  E1"  veya  "001A  E1  A2"
 * 
 * Forward reference varsa, operand byte'ları (00 00) sembolün gerçek adresiyle değiştirilir.
 */

void run_pass2(FILE *sin, FILE *fobj, FILE *ftab) {
    char line[256];
    
    // ============================================================
    // ADIM 1: DAT (Direct Address Table) Tablosunu .t Dosyasına Yaz
    // ============================================================
    // DAT tablosu, relocatable adreslerin listesidir.
    // Pass 1'de direct addressing kullanan instruction'ların operand adresleri buraya eklenmiştir.
    // Bu tablo linker tarafından relocation işlemi için kullanılacak.
    fprintf(ftab, "DAT\n");
    for (int i = 0; i < 30; i++) {
        if (DAT[i].address != -1) {
            fprintf(ftab, "%X\n", DAT[i].address);
        }
    }

    // ============================================================
    // ADIM 2: HDRM Tablolarını .t Dosyasına Yaz
    // ============================================================
    // HDRM tabloları modül bilgilerini ve external reference'ları içerir:
    // - H (Header): Modül adı, başlangıç adresi, program uzunluğu
    // - D (Define): ENTRY ile tanımlanan semboller ve adresleri (diğer modüller için export)
    // - R (Reference): EXTREF ile tanımlanan external semboller (diğer modüllerden import)
    // - M (Modify): External sembollerin kullanıldığı adresler (linker için)
    fprintf(ftab, "HDRM\n");
    
    // H (Header) kaydı: Modül bilgileri
    fprintf(ftab, "H %s %X %X\n", module_name, prog_start, prog_len);
    
    // D, R, M kayıtlarını yaz
    for (int i = 0; i < 20; i++) {
        if (HDRMT[i].code == 'D') {
            // D (Define): Bu modülde tanımlanan ve export edilen semboller
            fprintf(ftab, "D %s %X\n", HDRMT[i].symbol, HDRMT[i].address);
        } else if (HDRMT[i].code == 'R') {
            // R (Reference): Bu modülde kullanılan ama başka modülde tanımlı semboller
            fprintf(ftab, "R %s\n", HDRMT[i].symbol);
        } else if (HDRMT[i].code == 'M') {
            // M (Modify): External sembolün kullanıldığı adres (linker bu adresi patch edecek)
            fprintf(ftab, "M %s %X\n", HDRMT[i].symbol, HDRMT[i].address);
        }
    }

    // ============================================================
    // ADIM 3: .s Dosyasını İşleyerek .o Dosyasını Oluştur
    // ============================================================
    // .s dosyasını başa sar (rewind) çünkü tabloları yazdıktan sonra tekrar okumamız gerekiyor
    rewind(sin);
    
    // Her satırı oku ve işle
    while (fgets(line, sizeof(line), sin)) {
        // Çok kısa satırları atla (boş satırlar, geçersiz format)
        if (strlen(line) < 4) continue;
        
        // ============================================================
        // ADIM 3.1: Location Counter (LC) Değerini Parse Et
        // ============================================================
        // .s dosyasındaki her satırın başında hex formatında LC değeri var
        // Örnek: "001A  E1  00 00" -> LC = 0x001A
        int line_lc;
        if (sscanf(line, "%x", &line_lc) != 1) {
            // Parse edilemezse (geçersiz format), satırı olduğu gibi kopyala
            // Normalde geçerli .s dosyasında bu durum olmamalı
            fprintf(fobj, "%s", line);
            continue;
        }

        // ============================================================
        // ADIM 3.2: Forward Reference Table (FRT) Kontrolü
        // ============================================================
        // Bu satırın LC değeri FRT'de var mı kontrol et
        // Eğer varsa, bu satırda bir forward reference var demektir
        // Pass 1'de tanımlanmamış bir sembol kullanılmış ve FRT'ye eklenmiş
        int patched = 0;              // Bu satır patch edildi mi?
        char patch_symbol[10] = {0};  // Patch edilecek sembol adı
        
        for (int i = 0; i < 20; i++) {
            if (FRT[i].symbol[0] != '\0' && FRT[i].address == line_lc) {
                // FRT'de bulundu! Bu satırda forward reference var
                strcpy(patch_symbol, FRT[i].symbol);
                patched = 1;
                break;
            }
        }

        // ============================================================
        // ADIM 3.3: Satırı İşle
        // ============================================================
        if (!patched) {
            // Forward reference yok → satırı olduğu gibi .o dosyasına yaz
            // (External reference'lar da burada, onları linker çözecek)
            fprintf(fobj, "%s", line);
        } else {
            // Forward reference var → sembol adresini bul ve patch et
            
            // ============================================================
            // ADIM 3.3.1: Symbol Table (ST)'dan Sembol Adresini Bul
            // ============================================================
            // Pass 1'de sembol tanımlandığında ST'ye eklenmişti
            // Şimdi bu sembolün adresini ST'den buluyoruz
            int addr = -1;
            for (int i = 0; i < 10; i++) {
                if (strcmp(ST[i].symbol, patch_symbol) == 0) {
                    addr = ST[i].address;
                    break;
                }
            }

            if (addr == -1) {
                // ============================================================
                // HATA: Sembol ST'de bulunamadı
                // ============================================================
                // Eğer bir sembol FRT'de varsa ama ST'de yoksa, bu bir hatadır.
                // External semboller Pass 1'de FRT'ye eklenmez (M kaydı olarak işaretlenir).
                // Bu durumda "Undefined Symbol" hatası verilir.
                fprintf(stderr, "ERROR: Undefined symbol %s at %X\n", patch_symbol, line_lc);
                // Hata olsa bile satırı olduğu gibi yaz (patch edilmeden)
                fprintf(fobj, "%s", line); 
            } else {
                // ============================================================
                // BAŞARILI: Sembol adresi bulundu, satırı patch et
                // ============================================================
                // Satırı yeniden oluştur:
                // - LC değeri aynı kalır
                // - Opcode aynı kalır
                // - Operand byte'ları (00 00) sembolün gerçek adresiyle değiştirilir
                
                // Opcode'u satırdan parse et
                char opcode[10]; 
                sscanf(line, "%*x %s", opcode);
                
                // Yeni satırı oluştur ve .o dosyasına yaz
                // Adres 16-bit olduğu için 2 byte'a bölünür:
                // - Yüksek byte: (addr >> 8) & 0xFF
                // - Düşük byte:  addr & 0xFF
                fprintf(fobj, "%04X  %s  %02X %02X\n", 
                        line_lc, opcode, (addr >> 8) & 0xFF, addr & 0xFF);
            }
        }
    }
}
