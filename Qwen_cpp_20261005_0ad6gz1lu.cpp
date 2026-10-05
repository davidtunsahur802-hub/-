#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// === SKIN DATABASE (из дампа s.txt) ===
struct SkinEntry {
    int id;
    std::string name;
    std::string category;
};

std::vector<SkinEntry> g_skinDB;
std::unordered_map<int, bool> g_ownedSkins; // ID -> owned

void LoadSkinDatabase(const std::string& path) {
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        // Парсинг: public const dvs Name = ID;
        auto eq = line.find('=');
        auto semi = line.rfind(';');
        if (eq == std::string::npos || semi == std::string::npos) continue;
        
        std::string namePart = line.substr(line.find("dvs ") + 4, eq - line.find("dvs ") - 4);
        std::string idStr = line.substr(eq + 1, semi - eq - 1);
        
        // Trim whitespace
        namePart.erase(0, namePart.find_first_not_of(" \t"));
        namePart.erase(namePart.find_last_not_of(" \t") + 1);
        idStr.erase(0, idStr.find_first_not_of(" \t"));
        idStr.erase(idStr.find_last_not_of(" \t") + 1);
        
        try {
            int id = std::stoi(idStr);
            std::string cat = "Unknown";
            if (namePart.find("Knife") != std::string::npos) cat = "Knife";
            else if (namePart.find("Gloves") != std::string::npos) cat = "Gloves";
            else if (namePart.find("Sticker") != std::string::npos) cat = "Sticker";
            else if (namePart.find("Charm") != std::string::npos) cat = "Charm";
            else if (namePart.find("Graffiti") != std::string::npos) cat = "Graffiti";
            else if (namePart.find("AWM") != std::string::npos || 
                     namePart.find("M4A1") != std::string::npos ||
                     namePart.find("AKR") != std::string::npos ||
                     namePart.find("Deagle") != std::string::npos ||
                     namePart.find("DesertEagle") != std::string::npos) cat = "Weapon";
            
            g_skinDB.push_back({id, namePart, cat});
        } catch (...) {}
    }
}

// === MEMORY ENGINE ===
class MemoryEngine {
public:
    HANDLE hProcess = nullptr;
    uintptr_t baseAddr = 0;
    
    bool Attach(const wchar_t* procName) {
        PROCESSENTRY32 pe{sizeof(pe)};
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (!Process32First(snap, &pe)) return false;
        
        do {
            if (_wcsicmp(pe.szExeFile, procName) == 0) {
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                MODULEENTRY32 me{sizeof(me)};
                HANDLE mSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe.th32ProcessID);
                if (Module32First(mSnap, &me)) baseAddr = (uintptr_t)me.modBaseAddr;
                CloseHandle(mSnap);
                break;
            }
        } while (Process32Next(snap, &pe));
        
        CloseHandle(snap);
        return hProcess != nullptr;
    }
    
    template<typename T>
    T Read(uintptr_t addr) {
        T val{};
        ReadProcessMemory(hProcess, (LPCVOID)addr, &val, sizeof(T), nullptr);
        return val;
    }
    
    template<typename T>
    void Write(uintptr_t addr, T val) {
        WriteProcessMemory(hProcess, (LPVOID)addr, &val, sizeof(T), nullptr);
    }
    
    // Поиск адреса скина по cheap ID (4-byte scan)
    uintptr_t FindSkinAddress(int cheapId) {
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t addr = 0;
        while (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
                std::vector<uint8_t> buf(mbi.RegionSize);
                SIZE_T read;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buf.data(), mbi.RegionSize, &read)) {
                    for (SIZE_T i = 0; i <= read - sizeof(int); ++i) {
                        if (*(int*)&buf[i] == cheapId) {
                            return (uintptr_t)mbi.BaseAddress + i;
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        }
        return 0;
    }
};

// === CONFIG SYSTEM ===
struct Config {
    int selectedCheapId = 0;
    int selectedTargetId = 0;
    bool autoApply = false;
    float animSpeed = 1.0f;
    
    void Save(const std::string& path) {
        json j;
        j["cheapId"] = selectedCheapId;
        j["targetId"] = selectedTargetId;
        j["autoApply"] = autoApply;
        j["animSpeed"] = animSpeed;
        std::ofstream(path) << j.dump(4);
    }
    
    void Load(const std::string& path) {
        try {
            json j;
            std::ifstream(path) >> j;
            selectedCheapId = j.value("cheapId", 0);
            selectedTargetId = j.value("targetId", 0);
            autoApply = j.value("autoApply", false);
            animSpeed = j.value("animSpeed", 1.0f);
        } catch (...) {}
    }
};

// === MAIN OVERLAY LOOP (ImGui + DX11) ===
// ... [DX11/ImGui init boilerplate omitted for brevity] ...

void RenderUI(MemoryEngine& mem, Config& cfg) {
    static float pulse = 0.f;
    pulse += 0.05f * cfg.animSpeed;
    float alpha = 0.7f + 0.3f * sinf(pulse);
    
    ImGui::SetNextWindowBgAlpha(alpha);
    ImGui::Begin("SO2 Skin Changer v4080", nullptr, ImGuiWindowFlags_NoCollapse);
    
    // Owned skins filter
    ImGui::Text("Owned Skins: %zu / %zu", g_ownedSkins.size(), g_skinDB.size());
    ImGui::Separator();
    
    // Cheap skin selector
    ImGui::Text("Cheap Skin (Source):");
    static char searchBuf[64] = "";
    ImGui::InputText("Search##cheap", searchBuf, sizeof(searchBuf));
    
    if (ImGui::BeginCombo("##cheapCombo", 
        cfg.selectedCheapId ? std::to_string(cfg.selectedCheapId).c_str() : "Select...")) {
        for (auto& s : g_skinDB) {
            if (g_ownedSkins.count(s.id) && 
                (searchBuf[0] == '\0' || s.name.find(searchBuf) != std::string::npos)) {
                if (ImGui::Selectable(s.name.c_str(), cfg.selectedCheapId == s.id))
                    cfg.selectedCheapId = s.id;
            }
        }
        ImGui::EndCombo();
    }
    
    // Target skin selector  
    ImGui::Text("Target Skin (Expensive):");
    if (ImGui::BeginCombo("##targetCombo",
        cfg.selectedTargetId ? std::to_string(cfg.selectedTargetId).c_str() : "Select...")) {
        for (auto& s : g_skinDB) {
            if (s.category == "Weapon" || s.category == "Knife" || s.category == "Gloves") {
                if (ImGui::Selectable(s.name.c_str(), cfg.selectedTargetId == s.id))
                    cfg.selectedTargetId = s.id;
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::Checkbox("Auto-Apply on Sell/Loot", &cfg.autoApply);
    ImGui::SliderFloat("Animation Speed", &cfg.animSpeed, 0.1f, 3.0f);
    
    if (ImGui::Button("SWAP NOW", ImVec2(-1, 40))) {
        if (cfg.selectedCheapId && cfg.selectedTargetId) {
            uintptr_t addr = mem.FindSkinAddress(cfg.selectedCheapId);
            if (addr) {
                mem.Write<int>(addr, cfg.selectedTargetId);
                // Визуальный feedback через ImGui toast
            }
        }
    }
    
    ImGui::End();
}

int main() {
    LoadSkinDatabase("skins_db.txt"); // Экспортированный дамп из s.txt
    
    Config cfg;
    cfg.Load("skinchanger.cfg");
    
    MemoryEngine mem;
    if (!mem.Attach(L"Standoff2.exe")) return 1;
    
    // TODO: Scan inventory to populate g_ownedSkins
    // TODO: DX11 + ImGui overlay init
    // TODO: Main loop calling RenderUI(mem, cfg)
    
    cfg.Save("skinchanger.cfg");
    return 0;
}