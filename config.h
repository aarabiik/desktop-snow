#pragma once
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>

struct SnowConfig {
    // Snowfall parameters
    int flakeCount = 220;
    float gravity = 110.0f;
    float wind = 10.0f;
    float flakeSize = 3.6f;
    int style = 0; // 0 = Fluffy, 1 = Glow, 2 = Crystals, 3 = Neon, 4 = Dots
    
    // Snowdrift / Mound parameters (accumulating on window roofs)
    bool enableDrifts = true;
    float driftGrowthRate = 1.0f;   // Multiplier for snow pile growth
    float maxDriftHeight = 32.0f;   // Maximum pile height in pixels (8 to 60)
    float meltSpeed = 0.8f;         // How fast snow melts on roofs (0.0 to 4.0)
    float reposeSlope = 1.4f;       // Steepness limit before snow avalanches/slides
    
    // Game detection & Performance
    bool autoPauseInGames = true;   // Auto freeze to 0.00% CPU when gaming

    // Color
    int colorR = 255;
    int colorG = 255;
    int colorB = 255;
    int colorPreset = 0; // 0=White, 1=Ice Blue, 2=Gold, 3=Cyan, 4=Pink

    bool paused = false;

    void SetColorPreset(int preset) {
        colorPreset = preset;
        switch (preset) {
            case 0: colorR = 255; colorG = 255; colorB = 255; break; // Classic Snow White
            case 1: colorR = 210; colorG = 238; colorB = 255; break; // Cool Ice Blue
            case 2: colorR = 255; colorG = 240; colorB = 200; break; // Warm Golden Holiday
            case 3: colorR = 0;   colorG = 245; colorB = 255; break; // Cyber Cyan
            case 4: colorR = 255; colorG = 80;  colorB = 200; break; // Neon Pink
        }
    }

    void ApplyPreset(int presetId) {
        switch (presetId) {
            case 0: // Heavy Drifts / Winter Tale
                flakeCount = 220;
                gravity = 100.0f;
                wind = 8.0f;
                flakeSize = 4.0f;
                enableDrifts = true;
                driftGrowthRate = 1.5f;
                maxDriftHeight = 38.0f;
                meltSpeed = 0.6f;
                reposeSlope = 1.3f;
                style = 0; // Fluffy
                SetColorPreset(0);
                break;
            case 1: // Blizzard / Real Wind
                flakeCount = 380;
                gravity = 175.0f;
                wind = 32.0f;
                flakeSize = 3.2f;
                enableDrifts = true;
                driftGrowthRate = 1.8f;
                maxDriftHeight = 28.0f;
                meltSpeed = 1.0f;
                reposeSlope = 1.2f;
                style = 0; // Fluffy
                SetColorPreset(1);
                break;
            case 2: // Soft Glow
                flakeCount = 180;
                gravity = 90.0f;
                wind = 10.0f;
                flakeSize = 3.8f;
                enableDrifts = true;
                driftGrowthRate = 1.0f;
                maxDriftHeight = 26.0f;
                meltSpeed = 0.9f;
                reposeSlope = 1.4f;
                style = 1; // Glow
                SetColorPreset(1);
                break;
            case 3: // Cyber Neon
                flakeCount = 260;
                gravity = 125.0f;
                wind = 15.0f;
                flakeSize = 3.8f;
                enableDrifts = true;
                driftGrowthRate = 1.2f;
                maxDriftHeight = 30.0f;
                meltSpeed = 0.8f;
                reposeSlope = 1.3f;
                style = 3; // Neon
                SetColorPreset(3);
                break;
            case 4: // Eco Mode (< 0.1% CPU)
                flakeCount = 80;
                gravity = 95.0f;
                wind = 5.0f;
                flakeSize = 3.0f;
                enableDrifts = true;
                driftGrowthRate = 0.8f;
                maxDriftHeight = 22.0f;
                meltSpeed = 1.2f;
                reposeSlope = 1.5f;
                style = 4; // Dots
                SetColorPreset(0);
                break;
        }
    }

    void Load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "flakeCount") flakeCount = std::stoi(val);
            else if (key == "gravity") gravity = std::stof(val);
            else if (key == "wind") wind = std::stof(val);
            else if (key == "flakeSize") flakeSize = std::stof(val);
            else if (key == "style") style = std::stoi(val);
            else if (key == "enableDrifts") enableDrifts = (val == "1");
            else if (key == "driftGrowthRate") driftGrowthRate = std::stof(val);
            else if (key == "maxDriftHeight") maxDriftHeight = std::stof(val);
            else if (key == "meltSpeed") meltSpeed = std::stof(val);
            else if (key == "reposeSlope") reposeSlope = std::stof(val);
            else if (key == "colorR") colorR = std::stoi(val);
            else if (key == "colorG") colorG = std::stoi(val);
            else if (key == "colorB") colorB = std::stoi(val);
            else if (key == "colorPreset") colorPreset = std::stoi(val);
            else if (key == "autoPauseInGames") autoPauseInGames = (val == "1");
        }
    }

    void Save(const std::string& filepath) {
        std::ofstream file(filepath);
        if (!file.is_open()) return;
        file << "flakeCount=" << flakeCount << "\n";
        file << "gravity=" << gravity << "\n";
        file << "wind=" << wind << "\n";
        file << "flakeSize=" << flakeSize << "\n";
        file << "style=" << style << "\n";
        file << "enableDrifts=" << (enableDrifts ? 1 : 0) << "\n";
        file << "driftGrowthRate=" << driftGrowthRate << "\n";
        file << "maxDriftHeight=" << maxDriftHeight << "\n";
        file << "meltSpeed=" << meltSpeed << "\n";
        file << "reposeSlope=" << reposeSlope << "\n";
        file << "colorR=" << colorR << "\n";
        file << "colorG=" << colorG << "\n";
        file << "colorB=" << colorB << "\n";
        file << "colorPreset=" << colorPreset << "\n";
        file << "autoPauseInGames=" << (autoPauseInGames ? 1 : 0) << "\n";
    }
};
