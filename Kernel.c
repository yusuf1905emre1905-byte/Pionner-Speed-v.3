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
