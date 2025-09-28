// =========================================================
// PIONNEROS V3.0: BELLEK İZOLASYONU FONKSİYONU (kernel.c)
// BÖLÜM 58: set_user_memory()
// =========================================================

// Sayfa Dizini Girişindeki Paging Bayrakları (Daha önce tanımladınız)
// us : 1 (Kullanıcı/Süpervizör seviyesi)
// rw : 1 (Okuma/Yazma)

// Bir bellek bölgesini Kullanıcı Modu (Ring 3) için ayarlar.
void set_user_memory(unsigned int virt_addr, unsigned int phys_addr, int num_pages) {
    
    // Page Directory ve Page Table Index'lerini hesapla
    unsigned int pd_idx = virt_addr >> 22; // 31. bitten 22. bite kadar
    unsigned int pt_idx = (virt_addr >> 12) & 0x3FF; // 21. bitten 12. bite kadar

    // 1. Sayfa Tablosunu Hazırla (Gerekirse yeni Sayfa Tablosu oluşturulur)
    page_entry_t* pt;
    
    // Basitlik için, şu an sadece ilk tabloyu kullanıyoruz (pd_idx=0)
    if (pd_idx != 0) {
         puts("[WARN]: Birden fazla sayfa dizini kullanilmadi. Basitlik için pd_idx=0 kullaniliyor.");
         return; 
    }
    pt = first_page_table;


    // 2. Sayfa Girişlerini Kullanıcı Alanı Olarak Ayarla
    for (int i = 0; i < num_pages; i++) {
        // Fiziksel adresi ve izinleri ayarla
        pt[pt_idx + i].frame = (phys_addr + (i * 0x1000)) >> 12; // Fiziksel adresi yaz
        
        // ÖNEMLİ: Bu iki bayrak, V3.0'ın kalbidir!
        pt[pt_idx + i].us = 1;     // [Kullanıcı Modu] Erişebilir. (Ring 3)
        pt[pt_idx + i].rw = 1;     // Yazılabilir.
        
        pt[pt_idx + i].present = 1; // Bellek RAM'de Mevcut.
        
        // İzolasyon logu
        puts("[MEM ISO]: Kullanici alani sayfa adresi ayarlandi.\n");
    }
}
// =========================================================
// PIONNEROS V3.0: KULLANICI UYGULAMASI BAŞLATICI (kernel.c)
// BÖLÜM 60: start_user_app()
// =========================================================

// Harici Assembly fonksiyonları
extern void go_to_user_mode(unsigned int entry_point, unsigned int user_stack);

// Basitlik için, kullanıcı kodunun 1MB'dan başladığını varsayalım.
#define USER_APP_ENTRY_POINT 0x100000 
// Kullanıcı uygulaması için Stack (Yüksek adresten başlar ve aşağı doğru büyür)
#define USER_STACK_TOP 0x200000 

void start_user_app() {
    puts("\n[SCHEDULER]: Yeni Kullanici Gorevi Olusturuluyor...\n");

    // 1. Kullanıcı Belleğini Ayır ve Eşle
    // Basitlik için, 1MB'lık alanı (yaklaşık 256 sayfa) kullanıcı alanı olarak işaretle
    set_user_memory(USER_APP_ENTRY_POINT, USER_APP_ENTRY_POINT, 256);
    
    // Uygulama kodu, USER_APP_ENTRY_POINT'e yüklenmiş olmalıdır.
    // (Gerçek OS'ta buraya diskten okuma kodu gelirdi, biz simülasyon yapıyoruz.)
    
    // 2. CPU'yu Ring 3'e Geçir ve Uygulamayı Başlat
    puts("[RING 3]: Ayrıcalık Seviyesi Düşürülüyor. Pong basliyor...\n");
    
    // Assembly fonksiyonunu çağır: 
    // go_to_user_mode(Uygulamanın başlangıç adresi, Kullanıcının Stack Adresi)
    go_to_user_mode(USER_APP_ENTRY_POINT, USER_STACK_TOP);

    // DİKKAT: Bu noktadan sonra kod çekirdeğe geri dönmez,
    // Kullanıcı Modunda Pong veya AI çalışmaya başlar.
    // Geri dönüş ancak bir Kesme (Interrupt) veya Hata (Exception) ile olur.
}

// kernel_main'i güncelleyelim: Paging'i açtıktan sonra uygulamayı başlat.
void kernel_main() {
    // ... (GDT, IDT, PIC init kodlarınız buraya gelir) ...

    // V3.0: Paging'i aç
    init_paging(); 

    // V3.0: İlk uygulamayı güvenli modda (Ring 3) başlat
    start_user_app(); 

    // Bu satıra asla ulaşılmamalıdır!
    while(1) {} 
}
// =========================================================
// PIONNEROS V3.0: SİSTEM ÇAĞRISI SUNUCUSU (kernel.c)
// BÖLÜM 61: syscall_handler()
// =========================================================

// Uygulamaların isteyeceği hizmetlerin ID'leri (Örn: putstr)
#define SYSCALL_ID_PUTS 1 
#define SYSCALL_ID_EXIT 2
// ... (Gelecekte buraya DISK OKUMA, MOUSE AL gibi ID'ler eklenecek)

// Bu fonksiyon, INT 0x80 Assembly kesmesinden çağrılır (Assembly ile bağlanacak)
void syscall_handler(unsigned int service_id, unsigned int arg1) {
    
    // Yüksek güvenlikli bir yerde (Ring 0) olduğumuzdan emin ol!
    // (Bu kontrolü gelecekte Paging ile pekiştireceğiz.)

    switch (service_id) {
        case SYSCALL_ID_PUTS:
            // Arg1'in bir string adresi olduğunu varsayalım
            // puts((char*)arg1); // puts fonksiyonu güvenli bir şekilde çağrılır
            puts("[SYSCALL]: Uygulama metin yazdirmayi istedi.\n");
            break;
        
        case SYSCALL_ID_EXIT:
            puts("[SYSCALL]: Uygulama cikis istedi. Gorev sonlandiriliyor.\n");
            // Çekirdek burada görevi sonlandırma mekanizmasını çalıştırır.
            break;

        default:
            puts("[SYSCALL_ERROR]: Gecersiz hizmet ID'si alindi!\n");
            break;
    }
}
// kernel.c - init_gdt_idt() fonksiyonu içinde
// ...
// Kesme Girişi 0x80'i Sistem Çağrısı için kur
idt_set_gate(0x80, (unsigned int)asm_syscall_handler, 0x08, 0xEE); 
// NOT: 0xEE bayrağı, bu kesmenin Kullanıcı Modu'ndan (Ring 3) çağrılabileceği anlamına gelir!
// ...
// load_idt(); // IDT'yi yükle

// kernel.c - init_gdt_idt() fonksiyonu içinde
// ...
// Kesme Girişi 0x80'i Sistem Çağrısı için kur
idt_set_gate(0x80, (unsigned int)asm_syscall_handler, 0x08, 0xEE); 
// NOT: 0xEE bayrağı, bu kesmenin Kullanıcı Modu'ndan (Ring 3) çağrılabileceği anlamına gelir!
// ...
// load_idt(); // IDT'yi yükle

// =========================================================
// PIONNEROS V3.0: SÜRÜCÜ AYIRMA İÇİN IPC TEMELİ (kernel.c)
// BÖLÜM 63: IPC_MESSAGE_T
// =========================================================

// Görevler (Task) ve Sürücüler arası iletişim için mesaj yapısı
typedef struct {
    unsigned int sender_pid;     // Mesajı gönderen uygulamanın ID'si
    unsigned int receiver_pid;   // Mesajı alması gereken sürücü/sunucunun ID'si
    unsigned int type;           // İletişimin tipi (DISK_READ, MOUSE_EVENT, VFS_LOOKUP)
    unsigned int status;         // Mesajın durumu (Başarılı, Hata kodu vb.)
    char data[64];               // Kısa veri yükü (Dosya adı, tuş kodu vb.)
} ipc_message_t;

// Yeni Mesajlaşma Sistemi Çağrıları (Prototip)
// Bu fonksiyonlar, gelecekte uygulamalar tarafından kullanılacaktır.
void send_message(ipc_message_t *msg);
void receive_message(ipc_message_t *msg);

// =========================================================
// PIONNEROS V3.0: IPC GÖNDERME/ALMA MEKANİZMASI (kernel.c)
// BÖLÜM 64: send_message() ve receive_message()
// =========================================================

// Mesaj kuyruğu (Basitlik için sabit boyutlu bir dizi kullanıyoruz)
#define MAX_IPC_MESSAGES 32
ipc_message_t message_queue[MAX_IPC_MESSAGES];
int queue_head = 0;
int queue_tail = 0;

// Görevler (Task) arasında mesaj gönderme
void send_message(ipc_message_t *msg) {
    
    // Kuyruk Dolu Kontrolü
    if (((queue_tail + 1) % MAX_IPC_MESSAGES) == queue_head) {
        puts("[IPC ERROR]: Mesaj kuyrugu dolu. Mesaj gonderilemedi.\n");
        return;
    }

    // 1. Mesajı kuyruğa kopyala
    // (Gerçek OS'ta buraya bellek kopyalama fonksiyonu gelirdi)
    message_queue[queue_tail] = *msg; 
    
    // 2. Kuyruk işaretçisini ilerlet
    queue_tail = (queue_tail + 1) % MAX_IPC_MESSAGES;
    
    puts("[IPC]: Mesaj gonderildi.\n");

    // NOT: Gerçek bir OS'ta, burada Scheduler'a gidilip alıcının uyanması sağlanırdı.
}

// Mesaj bekleme ve alma
void receive_message(ipc_message_t *msg) {
    
    // Kuyruk Boş Kontrolü
    if (queue_head == queue_tail) {
        puts("[IPC]: Mesaj kuyrugu bos. Gorev beklemeye aliniyor...\n");
        
        // ÖNEMLİ: Gerçek OS'ta, görev burada BLOKLANIR (uyku moduna geçer)
        // ve Scheduler (Zamanlayıcı) başka bir görevi çalıştırırdı.
        // Mesaj geldiğinde, bu görev uyanırdı.
        
        // Simülasyon için: Boş mesaj döndür
        msg->type = 0; 
        return;
    }
    
    // 1. Mesajı kuyruktan kopyala
    *msg = message_queue[queue_head];

    // 2. Kuyruk işaretçisini ilerlet
    queue_head = (queue_head + 1) % MAX_IPC_MESSAGES;
    
    puts("[IPC]: Mesaj alindi.\n");
}
// =========================================================
// PIONNEROS V3.0: SİSTEM ÇAĞRISI SUNUCUSU (kernel.c)
// BÖLÜM 65: syscall_handler() GÜNCELLEMESİ (IPC Desteği)
// =========================================================

// Uygulamaların isteyeceği hizmetlerin ID'leri (IPC Hizmetleri Eklendi)
#define SYSCALL_ID_PUTS      1 
#define SYSCALL_ID_EXIT      2
#define SYSCALL_ID_SEND_MSG  3 // YENİ: Mesaj Gönderme Hizmeti
#define SYSCALL_ID_RECV_MSG  4 // YENİ: Mesaj Alma Hizmeti

// Bu fonksiyon, INT 0x80 Assembly kesmesinden çağrılır
// arg1 = Geleneksel olarak ilk argüman (Örn: String adresi veya Mesaj Yapısı adresi)
void syscall_handler(unsigned int service_id, unsigned int arg1) {
    
    // arg1'in Mesaj Yapısı adresi olduğunu varsayalım
    ipc_message_t *msg_ptr = (ipc_message_t *)arg1;

    switch (service_id) {
        case SYSCALL_ID_PUTS:
            // puts((char*)arg1); // puts fonksiyonu güvenli bir şekilde çağrılır
            puts("[SYSCALL]: Uygulama metin yazdirmayi istedi.\n");
            break;
        
        case SYSCALL_ID_EXIT:
            puts("[SYSCALL]: Uygulama cikis istedi. Gorev sonlandiriliyor.\n");
            break;
            
        case SYSCALL_ID_SEND_MSG:
            // Gelen mesajı güvenle çekirdek içi IPC mekanizmasına gönder.
            send_message(msg_ptr);
            puts("[SYSCALL]: Uygulama IPC Mesaj Gonderdi.\n");
            break;

        case SYSCALL_ID_RECV_MSG:
            // Çekirdekten mesaj almayı dene.
            receive_message(msg_ptr);
            puts("[SYSCALL]: Uygulama IPC Mesaj Bekliyor.\n");
            break;

        default:
            puts("[SYSCALL_ERROR]: Gecersiz hizmet ID'si alindi!\n");
            break;
    }
}


