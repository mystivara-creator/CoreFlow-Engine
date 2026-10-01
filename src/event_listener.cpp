#include "../include/coreflow.hpp"
#include <fstream>
#include <string>
#include <cstdlib>

// ==========================================
// EVENT LISTENER: Implementasi Status Layar (Zero Overhead)
// ==========================================
bool EventListener::isScreenOn() {
    // Membaca langsung dari node tampilan sysfs (Sangat universal untuk mayoritas kernel modern)
    std::ifstream file("/sys/class/drm/card0-DSI-1/status"); 
    if (!file.is_open()) {
        // Jalur alternatif jika menggunakan panel grafis OLED/Frame Buffer lama
        file.open("/sys/class/graphics/fb0/blank");
    }

    if (file.is_open()) {
        std::string status;
        file >> status;
        file.close();
        // Pada card0-DSI-1: "connected" artinya layar hidup. 
        // Pada fb0/blank: "0" artinya layar hidup (tidak blank/mati).
        return (status == "connected" || status == "0");
    }

    // Fallback terakhir jika sistem memblokir akses sysfs mentah (Menggunakan Android Shell)
    FILE* pipe = popen("dumpsys power | grep -q 'Display Power: state=ON'", "r");
    if (!pipe) return true; // Default aman jika gagal
    int res = pclose(pipe);
    return (res == 0);
}
