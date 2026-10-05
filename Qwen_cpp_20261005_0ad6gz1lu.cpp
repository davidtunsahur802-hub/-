#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// === ImGui minimal inline (v1.90+) ===
// Для GitHub Actions билда используем vcpkg imgui[dx11-binding]
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// === SKIN DATABASE ===
struct SkinEntry {
    int id;
    std::string name;
    std::string category;
};

std::vector<SkinEntry> g_skinDB;
std::unordered_map<int, bool> g_ownedSkins;

void LoadSkinDatabase() {
    // Встроенный парсер дампа из s.txt
    // Формат: public const dvs Name = ID;
    const char* dumpData = R"RAW(
public const dvs None = 0;
public const dvs G22PixelCamouflage = 11001;
public const dvs G22Nest = 11002;
public const dvs G22Pattern = 11005;
public const dvs G22Inferno = 11006;
public const dvs G22FrostWyrm = 11008;
public const dvs USP_2Years = 12002;
public const dvs USP_2YearsRed = 12003;
public const dvs P350Cyber = 13001;
public const dvs P350Savannah = 13002;
public const dvs P350ForestSpirit = 13003;
public const dvs P350Rally = 13004;
public const dvs P350Skull = 13005;
public const dvs UMP45Cyberpunk = 32001;
public const dvs UMP45Pixel = 32002;
public const dvs UMP45Shark = 32003;
public const dvs UMP45Winged = 32004;
public const dvs UMP45Beast = 32005;
public const dvs UMP45Iron = 32006;
public const dvs MP7Offroad = 34001;
public const dvs MP7Arcade = 34002;
public const dvs MP7_2Years = 34003;
public const dvs MP7_2YearsRed = 34004;
public const dvs P90Radiation = 35001;
public const dvs P90Ghoul = 35002;
public const dvs P90Fury = 35003;
public const dvs P90Pilot = 35004;
public const dvs DeagleCaptainMorgan = 15001;
public const dvs DeagleBlood = 15002;
public const dvs DeaglePredator = 15003;
public const dvs DeagleRedDragon = 15004;
public const dvs DeagleWinner = 15005;
public const dvs DeagleDragonGlass = 15006;
public const dvs DeagleThunder = 15007;
public const dvs AKRTreasureHunter = 44002;
public const dvs AKRTiger = 44003;
public const dvs AKRSport = 44004;
public const dvs AKRNecromancer = 44005;
public const dvs AKRCarbon = 44006;
public const dvs AKR_2Years = 44007;
public const dvs AKR12Railgun = 45001;
public const dvs AKR12PixelCamouflage = 45002;
public const dvs AKR12Mechanic = 45003;
public const dvs AKR12Aurora = 45004;
public const dvs M4Predator = 46001;
public const dvs M4Necromancer = 46002;
public const dvs M4Tiger = 46003;
public const dvs M4Pro = 46006;
public const dvs M4GrandPrix = 46007;
public const dvs M16Camouflage = 47001;
public const dvs M16Winged = 47002;
public const dvs M16Facet = 47003;
public const dvs FamasBeagle = 48001;
public const dvs FamasFury = 48002;
public const dvs FamasHull = 48003;
public const dvs AWMSport = 51001;
public const dvs AWMPhoenix = 51002;
public const dvs AWMGear = 51003;
public const dvs AWMScratch = 51004;
public const dvs AWMGenesis = 51007;
public const dvs AWM_2YearsRed = 51008;
public const dvs M40Quake = 52001;
public const dvs M40Pro = 52002;
public const dvs M40Beagle = 52003;
public const dvs SM1014Facet = 62001;
public const dvs SM1014Pathfinder = 62002;
public const dvs SM1014Necromancer = 62003;
public const dvs SM1014NorthernCamouflage = 62004;
public const dvs SM1014Quake = 62005;
public const dvs SM1014Branches = 62006;
public const dvs M9BayonetBlueBlood = 71001;
public const dvs M9BayonetAncient = 71002;
public const dvs M9BayonetScratch = 71003;
public const dvs M9BayonetUniverse = 71004;
public const dvs M9ByonetDragonGlass = 71005;
public const dvs KarambitClaw = 72002;
public const dvs KarambitGold = 72003;
public const dvs KarambitIceDragon = 72004;
public const dvs KarambitScratch = 72006;
public const dvs KarambitUniverse = 72007;
public const dvs jKommandoAncient = 73002;
public const dvs jKommandoReaper = 73003;
public const dvs jKommandoFloral = 73004;
public const dvs jKommandoLuxury = 73006;
public const dvs Butterfly_Gold = 47502;
public const dvs Butterfly_DragonGlass = 47503;
public const dvs Butterfly_Red = 47504;
public const dvs Butterfly_Starfall = 47505;
public const dvs Deagle_Ace = 41502;
public const dvs FiveSeven_Venom = 41701;
public const dvs FiveSeven_Tactical = 41703;
public const dvs FnFAL_Leather = 44901;
public const dvs FnFAL_AcidCarbon = 44902;
public const dvs FnFAL_Tactical = 44903;
public const dvs G22_Relic = 41101;
public const dvs G22_Starfall = 41102;
public const dvs M4_Lizard = 44601;
public const dvs M4_Samurai = 44603;
public const dvs M110_Cyber = 45301;
public const dvs SM1014_Blaster = 45302;
public const dvs MP7_Thorn = 43401;
public const dvs MP7_Lich = 43402;
public const dvs P90_Jungle = 43502;
public const dvs Tec9_Aurora = 41601;
public const dvs Tec9_Fable = 41605;
public const dvs UMP45_PixelV2 = 43201;
public const dvs UMP45_Cerberus = 43202;
public const dvs USP_Fiend = 41201;
public const dvs USP_Pisces = 41212;
public const dvs GlovesPhoenix = 3000;
public const dvs GlovesAutumn = 3001;
public const dvs GlovesGeometric = 3002;
public const dvs GlovesRetroWave = 3003;
public const dvs GlovesLivingFlame = 3004;
public const dvs GlovesNeuro = 3005;
public const dvs GlovesBurningFists = 3006;
public const dvs GlovesPun = 3007;
public const dvs GlovesChampion = 3008;
public const dvs GlovesSteamRider = 3009;
public const dvs Kunai_Bone = 77813;
public const dvs Kunai_Luxury = 77814;
public const dvs Kunai_Poison = 77815;
public const dvs Kunai_Radiation = 77816;
public const dvs Kunai_Reaper = 77817;
public const dvs Scorpion_Camouflage = 87919;
public const dvs Scorpion_Green = 87920;
public const dvs Scorpion_Scratch = 87921;
public const dvs Scorpion_Sky = 87922;
public const dvs FlipKnife_New1 = 67701;
public const dvs FlipKnife_New2 = 67702;
public const dvs FlipKnife_New3 = 67703;
public const dvs FlipKnife_New4 = 67704;
public const dvs FlipKnife_New5 = 67705;
public const dvs KnifeButterfly_ColdFlam = 97500;
public const dvs KnifeKarambit_ColdFlame = 97200;
public const dvs KnifeKarambit_Frozen = 97201;
public const dvs KnifeKarambit_SnowCamo = 97203;
public const dvs KunaiKnife_ColdFlame = 97800;
public const dvs KunaiKnife_SnowCamo = 97801;
public const dvs KnifeBayonet_Frozen = 97100;
public const dvs FlipKnife_SnowCamo = 97700;
public const dvs ScorpionKnife_ColdFlame = 97900;
public const dvs Ursus_Fade = 8888888;
public const dvs Ursus_Scratch = 8888890;
public const dvs Ursus_Ruby = 8888891;
public const dvs Ursus_Believe = 8888892;
public const dvs Ursus_Claw = 8888893;
public const dvs Ursus_Universe = 8888894;
public const dvs Ursus_DragonGlass = 8888895;
public const dvs Ursus_Iceberg = 8888896;
public const dvs KarambitFANG_Claw = 99972002;
public const dvs KarambitFANG_Gold = 99972003;
public const dvs KarambitFANG_IceDragon = 99972004;
public const dvs KarambitFANG_Scratch = 99972006;
public const dvs KarambitFANG_Universe = 99972007;
)RAW";

    std::istringstream stream(dumpData);
    std::string line;
    while (std::getline(stream, line)) {
        auto eq = line.find('=');
        auto semi = line.rfind(';');
        if (eq == std::string::npos || semi == std::string::npos) continue;
        
        size_t start = line.find("dvs ");
        if (start == std::string::npos) continue;
        start += 4;
        
        std::string namePart = line.substr(start, eq - start);
        std::string idStr = line.substr(eq + 1, semi - eq - 1);
        
        namePart.erase(0, namePart.find_first_not_of(" \t"));
        namePart.erase(namePart.find_last_not_of(" \t") + 1);
        idStr.erase(0, idStr.find_first_not_of(" \t"));
        idStr.erase(idStr.find_last_not_of(" \t") + 1);
        
        try {
            int id = std::stoi(idStr);
            std::string cat = "Other";
            if (namePart.find("Knife") != std::string::npos || 
                namePart.find("Karambit") != std::string::npos ||
                namePart.find("Butterfly") != std::string::npos ||
                namePart.find("Bayonet") != std::string::npos ||
                namePart.find("Kunai") != std::string::npos ||
                namePart.find("Stilet") != std::string::npos ||
                namePart.find("Flip") != std::string::npos ||
                namePart.find("Dagger") != std::string::npos ||
                namePart.find("Ursus") != std::string::npos ||
                namePart.find("Mantis") != std::string::npos ||
                namePart.find("Sting") != std::string::npos ||
                namePart.find("Fang") != std::string::npos ||
                namePart.find("Kukri") != std::string::npos ||
                namePart.find("Scorpion") != std::string::npos ||
                namePart.find("jKommando") != std::string::npos ||
                namePart.find("Tanto") != std::string::npos) cat = "Knife";
            else if (namePart.find("Gloves") != std::string::npos) cat = "Gloves";
            else if (namePart.find("AWM") != std::string::npos || namePart.find("M40") != std::string::npos || namePart.find("M110") != std::string::npos) cat = "Sniper";
            else if (namePart.find("AKR") != std::string::npos || namePart.find("M4") != std::string::npos || namePart.find("M16") != std::string::npos || namePart.find("Famas") != std::string::npos || namePart.find("FnFAL") != std::string::npos || namePart.find("M60") != std::string::npos) cat = "Rifle";
            else if (namePart.find("Deagle") != std::string::npos || namePart.find("USP") != std::string::npos || namePart.find("P350") != std::string::npos || namePart.find("FiveSeven") != std::string::npos || namePart.find("Tec9") != std::string::npos || namePart.find("G22") != std::string::npos) cat = "Pistol";
            else if (namePart.find("UMP") != std::string::npos || namePart.find("MP7") != std::string::npos || namePart.find("MP5") != std::string::npos || namePart.find("P90") != std::string::npos || namePart.find("MAC10") != std::string::npos) cat = "SMG";
            else if (namePart.find("SM1014") != std::string::npos || namePart.find("SPAS") != std::string::npos) cat = "Shotgun";
            
            g_skinDB.push_back({id, namePart, cat});
        } catch (...) {}
    }
}

// === MEMORY ENGINE ===
class MemoryEngine {
public:
    HANDLE hProcess = nullptr;
    DWORD pid = 0;
    
    bool Attach(const wchar_t* procName) {
        PROCESSENTRY32W pe{sizeof(pe)};
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;
        
        if (!Process32FirstW(snap, &pe)) {
            CloseHandle(snap);
            return false;
        }
        
        do {
            if (_wcsicmp(pe.szExeFile, procName) == 0) {
                pid = pe.th32ProcessID;
                hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
                break;
            }
        } while (Process32NextW(snap, &pe));
        
        CloseHandle(snap);
        return hProcess != nullptr;
    }
    
    void Detach() {
        if (hProcess) {
            CloseHandle(hProcess);
            hProcess = nullptr;
        }
    }
    
    template<typename T>
    bool Read(uintptr_t addr, T& val) {
        return ReadProcessMemory(hProcess, (LPCVOID)addr, &val, sizeof(T), nullptr) != 0;
    }
    
    template<typename T>
    bool Write(uintptr_t addr, const T& val) {
        return WriteProcessMemory(hProcess, (LPVOID)addr, &val, sizeof(T), nullptr) != 0;
    }
    
    std::vector<uintptr_t> FindPattern(int value) {
        std::vector<uintptr_t> results;
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t addr = 0;
        
        while (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
                
                SIZE_T regionSize = mbi.RegionSize;
                if (regionSize > 0 && regionSize < 0x10000000) {
                    std::vector<BYTE> buffer(regionSize);
                    SIZE_T bytesRead = 0;
                    
                    if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), regionSize, &bytesRead)) {
                        for (SIZE_T i = 0; i + sizeof(int) <= bytesRead; ++i) {
                            if (*(int*)(buffer.data() + i) == value) {
                                results.push_back((uintptr_t)mbi.BaseAddress + i);
                            }
                        }
                    }
                }
            }
            addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            if (addr < (uintptr_t)mbi.BaseAddress) break;
        }
        
        return results;
    }
};

// === CONFIG SYSTEM ===
struct Config {
    int selectedCheapId = 0;
    int selectedTargetId = 0;
    bool autoApply = false;
    float animSpeed = 1.0f;
    std::string processName = "Standoff2.exe";
    
    void Save(const std::string& path) {
        std::ofstream f(path);
        f << "cheapId=" << selectedCheapId << "\n";
        f << "targetId=" << selectedTargetId << "\n";
        f << "autoApply=" << (autoApply ? 1 : 0) << "\n";
        f << "animSpeed=" << animSpeed << "\n";
        f << "processName=" << processName << "\n";
    }
    
    void Load(const std::string& path) {
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            auto eq = line.find('=');
            if (
