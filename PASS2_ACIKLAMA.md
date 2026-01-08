# Pass 2 - Forward Reference Resolution (İleri Referans Çözümleme)

## Genel Bakış

Pass 2, assembler'ın ikinci aşamasıdır ve Pass 1'den gelen ara kod dosyasını (`.s`) işleyerek final object code dosyasını (`.o`) ve tablo dosyasını (`.t`) üretir. Pass 2'nin temel görevi, Pass 1'de çözülemeyen **forward reference**'ları (ileri referansları) çözmektir.

---

## Pass 2'nin Görevleri

### 1. Forward Reference Çözümleme

- Pass 1'de tanımlanmamış semboller kullanıldığında, bu semboller **Forward Reference Table (FRT)**'a eklenir
- Pass 2, `.s` dosyasındaki her satırı kontrol eder
- Eğer bir satırın Location Counter (LC) değeri FRT'de varsa, bu satırda bir forward reference vardır
- Bu durumda sembolün adresi **Symbol Table (ST)**'dan bulunur ve object code'a yazılır

### 2. Object Code Üretimi (.o dosyası)

- Pass 1'den gelen `.s` dosyasındaki her satır işlenir
- Forward reference yoksa → satır olduğu gibi `.o` dosyasına yazılır
- Forward reference varsa → sembol adresi bulunur ve satır yeniden oluşturularak yazılır

### 3. Tablo Dosyası Üretimi (.t dosyası)

- **DAT (Direct Address Table)**: Relocatable adreslerin listesi
- **HDRM Tabloları**: Header, Define, Reference ve Modify kayıtları
  - **H (Header)**: Modül adı, başlangıç adresi, program uzunluğu
  - **D (Define)**: ENTRY ile tanımlanan semboller ve adresleri
  - **R (Reference)**: EXTREF ile tanımlanan external semboller
  - **M (Modify)**: External sembollerin kullanıldığı adresler

---

## Pass 2'nin Çalışma Akışı

```
1. .s dosyasını oku
   ↓
2. DAT tablosunu .t dosyasına yaz
   ↓
3. HDRM tablolarını .t dosyasına yaz
   ↓
4. .s dosyasını başa sar (rewind)
   ↓
5. Her satır için:
   ├─ LC değerini parse et
   ├─ FRT'de bu LC var mı kontrol et
   │  ├─ Varsa → ST'den sembol adresini bul
   │  │  ├─ Bulundu → Satırı patch et ve .o'ya yaz
   │  │  └─ Bulunamadı → Hata ver, satırı olduğu gibi yaz
   │  └─ Yoksa → Satırı olduğu gibi .o'ya yaz
   └─ Bir sonraki satıra geç
```

---

## Pass 2'nin Diğer Bileşenlerle İlişkisi

### Pass 1 ile İlişki

- **Girdi**: Pass 1'in ürettiği `.s` dosyası
- **Kullanılan Tablolar**:
  - `ST` (Symbol Table) - Pass 1'de oluşturuldu
  - `FRT` (Forward Reference Table) - Pass 1'de oluşturuldu
  - `DAT` (Direct Address Table) - Pass 1'de oluşturuldu
  - `HDRMT` (HDRM Table) - Pass 1'de oluşturuldu
  - `module_name`, `prog_start`, `prog_len` - Pass 1'de set edildi

### Parser ile İlişki

- Pass 2, parser'ı kullanmaz
- `.s` dosyası zaten parse edilmiş formatda (hex formatında)

### Main Program ile İlişki

- `main.c` Pass 1'i çalıştırdıktan sonra Pass 2'yi çağırır
- Pass 2'ye üç dosya pointer'ı verilir:
  - `sin`: `.s` dosyası (okuma)
  - `fobj`: `.o` dosyası (yazma)
  - `ftab`: `.t` dosyası (yazma)

---

## Örnek Senaryo

### Input (.s dosyası - Pass 1 çıktısı):

```
0000  E1  00 00    ; LDA XX (external)
0003  C1  00 00    ; CLL AD5 (external)
0006  A1  00 00    ; ADD ZZ (external)
0009  C1  00 00    ; CLL AD5 (external)
000C  F1  00 46    ; STA 70 (numeric address)
000F  E1  00 00    ; LDA ZZ (external)
0012  A4  01       ; SUB #1 (immediate)
0014  B3  00 00    ; BLT EX (forward reference - FRT'de var)
0017  B4  00 00    ; JMP LOOP (forward reference - FRT'de var)
001A  FE           ; HLT
```

### FRT (Forward Reference Table):

```
EX  -> 0014
LOOP -> 0017
```

### ST (Symbol Table):

```
LOOP -> 0000
EX   -> 001A
```

### Pass 2 İşlemi:

1. `0014 B3 00 00` satırı → FRT'de `EX` sembolü var
   - ST'de `EX` = `001A` bulunur
   - Satır patch edilir: `0014  B3  00 1A`
2. `0017 B4 00 00` satırı → FRT'de `LOOP` sembolü var

   - ST'de `LOOP` = `0000` bulunur
   - Satır patch edilir: `0017  B4  00 00`

3. Diğer satırlar → FRT'de yok, olduğu gibi yazılır

### Output (.o dosyası):

```
0000  E1  00 00    ; LDA XX (external - linker çözecek)
0003  C1  00 00    ; CLL AD5 (external - linker çözecek)
0006  A1  00 00    ; ADD ZZ (external - linker çözecek)
0009  C1  00 00    ; CLL AD5 (external - linker çözecek)
000C  F1  00 46    ; STA 70
000F  E1  00 00    ; LDA ZZ (external - linker çözecek)
0012  A4  01       ; SUB #1
0014  B3  00 1A    ; BLT EX (ÇÖZÜLDÜ!)
0017  B4  00 00    ; JMP LOOP (ÇÖZÜLDÜ!)
001A  FE           ; HLT
```

---

## Önemli Notlar

### Forward Reference vs External Reference

- **Forward Reference**: Aynı modül içinde tanımlanmış ama kullanımdan önce tanımlanmamış semboller
  - Pass 2 tarafından çözülür
  - ST'de mutlaka bulunmalı
- **External Reference**: Başka modülde tanımlanmış semboller (EXTREF ile belirtilir)
  - Pass 2 tarafından çözülmez
  - Linker tarafından çözülecek
  - `.o` dosyasında `00 00` olarak kalır
  - HDRM tablosunda M kaydı olarak işaretlenir

### Hata Durumları

- Eğer FRT'de bir sembol varsa ama ST'de yoksa → **Undefined Symbol** hatası
- Bu durumda satır patch edilmeden olduğu gibi yazılır (hata mesajı verilir)

### DAT Tablosu

- DAT, relocatable adreslerin listesidir
- Pass 1'de direct addressing kullanan instruction'ların operand adresleri DAT'a eklenir
- Pass 2, DAT'ı `.t` dosyasına yazar (linker için)

---

## Kod Yapısı

### `run_pass2()` Fonksiyonu

```c
void run_pass2(FILE *sin, FILE *fobj, FILE *ftab)
```

**Parametreler:**

- `sin`: `.s` dosyası (okuma modu)
- `fobj`: `.o` dosyası (yazma modu)
- `ftab`: `.t` dosyası (yazma modu)

**İşlem Adımları:**

1. DAT tablosunu `.t` dosyasına yaz
2. HDRM tablolarını `.t` dosyasına yaz
3. `.s` dosyasını başa sar
4. Her satırı oku ve işle:
   - LC parse et
   - FRT kontrolü yap
   - Gerekirse patch et
   - `.o` dosyasına yaz

---

## Sonuç

Pass 2, assembler'ın kritik bir parçasıdır ve forward reference'ları çözerek final object code'u üretir. Pass 1'in oluşturduğu tabloları kullanarak, sembol adreslerini bulur ve object code'a yerleştirir. Bu sayede, programcının sembolleri istediği sırada kullanabilmesi sağlanır.
