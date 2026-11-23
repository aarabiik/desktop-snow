#pragma once
#include <windows.h>
#include <dwmapi.h>
#include <vector>
#include <map>
#include <cmath>
#include <random>
#include <algorithm>
#include "config.h"

#ifndef DWMWA_EXTENDED_FRAME_BOUNDS
#define DWMWA_EXTENDED_FRAME_BOUNDS 9
#endif
#ifndef DWMWA_CLOAKED
#define DWMWA_CLOAKED 14
#endif

struct Particle {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float size = 3.5f;
    float mass = 1.0f;
    float alpha = 0.9f;
    float maxAlpha = 0.9f;
    float swayPhase = 0;
    float swaySpeed = 2.0f;
    float swayAmp = 25.0f;
    int colorMode = 0;

    void Reset(int screenW, int screenH, bool spawnAtTop, float baseSize) {
        x = (float)(rand() % (screenW + 80) - 40);
        y = spawnAtTop ? (float)(-(rand() % 60 + 10)) : (float)(rand() % screenH);
        vx = (float)((rand() % 160 - 80) / 10.0f);
        float sizeVariation = (float)(rand() % 100 - 50) / 100.0f * 1.2f;
        size = std::max(1.5f, baseSize + sizeVariation);
        mass = 0.6f + (size / 6.0f) * 0.7f;
        vy = (float)(65 + rand() % 85) * mass;
        swayPhase = (float)(rand() % 628) / 100.0f;
        swaySpeed = 1.6f + (float)(rand() % 20) / 10.0f;
        swayAmp = 16.0f + (float)(rand() % 26);
        maxAlpha = 0.75f + (float)(rand() % 25) / 100.0f;
        alpha = maxAlpha;
        colorMode = rand() % 4;
    }
};

class WindowSnowdrift {
public:
    static constexpr int BIN_WIDTH = 4;
    HWND hwnd = NULL;
    RECT rect = {0};
    float vx = 0, vy = 0;
    float prevLeft = 0, prevTop = 0;
    int binCount = 0;
    std::vector<float> heights;

    WindowSnowdrift() = default;
    WindowSnowdrift(HWND h, const RECT& r) : hwnd(h), rect(r) {
        prevLeft = (float)r.left;
        prevTop = (float)r.top;
        Resize(r.right - r.left);
    }

    void Resize(int width) {
        int newBins = std::max(8, width / BIN_WIDTH);
        if (newBins != binCount) {
            std::vector<float> newHeights(newBins, 0.0f);
            if (binCount > 0) {
                for (int i = 0; i < newBins; i++) {
                    float srcIdx = ((float)i / newBins) * binCount;
                    int i0 = (int)srcIdx;
                    int i1 = std::min(binCount - 1, i0 + 1);
                    float frac = srcIdx - i0;
                    if (i0 < binCount) {
                        newHeights[i] = heights[i0] * (1.0f - frac) + heights[i1] * frac;
                    }
                }
            }
            heights = std::move(newHeights);
            binCount = newBins;
        }
    }

    void UpdatePosition(const RECT& newRect, float dt) {
        rect = newRect;
        int w = rect.right - rect.left;
        Resize(w);

        if (dt > 0.001f) {
            float curVx = ((float)rect.left - prevLeft) / dt;
            float curVy = ((float)rect.top - prevTop) / dt;
            vx = vx * 0.3f + curVx * 0.7f;
            vy = vy * 0.3f + curVy * 0.7f;
        }
        prevLeft = (float)rect.left;
        prevTop = (float)rect.top;
    }

    float GetHeightAt(float relX) const {
        if (binCount <= 0 || relX < 0) return 0.0f;
        int bin = (int)(relX / BIN_WIDTH);
        if (bin >= binCount) return 0.0f;
        return heights[bin];
    }

    void Deposit(float relX, float amount) {
        if (binCount <= 0 || relX < 0) return;
        int centerBin = (int)(relX / BIN_WIDTH);
        if (centerBin >= binCount) return;

        // Smooth bell curve distribution: builds a soft round mound on the roof
        const float weights[7] = { 0.05f, 0.12f, 0.22f, 0.28f, 0.22f, 0.12f, 0.05f };
        for (int d = -3; d <= 3; d++) {
            int b = centerBin + d;
            // Never deposit on extreme edges (so snow doesn't spill over)
            if (b >= 2 && b < binCount - 2) {
                heights[b] += amount * weights[d + 3];
            }
        }
    }

    void StepPhysics(float dt, float meltSpeed, float maxDriftHeight, float reposeSlope) {
        if (binCount <= 0) return;

        // 1. Natural melting over time
        if (meltSpeed > 0.001f) {
            float meltAmount = meltSpeed * dt;
            for (int i = 0; i < binCount; i++) {
                if (heights[i] > 0.0f) {
                    heights[i] = std::max(0.0f, heights[i] - meltAmount);
                }
            }
        }

        // 2. Slope smoothing (relaxes sharp peaks into a rounded snow hill)
        for (int pass = 0; pass < 2; pass++) {
            for (int i = 0; i < binCount - 1; i++) {
                float diff = heights[i] - heights[i + 1];
                if (diff > reposeSlope) {
                    float shift = (diff - reposeSlope) * 0.4f;
                    heights[i] -= shift;
                    heights[i + 1] += shift;
                }
            }
            for (int i = binCount - 1; i > 0; i--) {
                float diff = heights[i] - heights[i - 1];
                if (diff > reposeSlope) {
                    float shift = (diff - reposeSlope) * 0.4f;
                    heights[i] -= shift;
                    heights[i - 1] += shift;
                }
            }
        }

        // 3. Taper edges cleanly to 0 (ABSOLUTELY NO SPILLING DOWN)
        // Snow that slides to the outer edges simply vanishes!
        heights[0] = 0.0f;
        if (binCount > 1) heights[1] = 0.0f;
        if (binCount > 2) heights[2] = std::min(heights[2], 2.0f);

        heights[binCount - 1] = 0.0f;
        if (binCount > 1) heights[binCount - 2] = 0.0f;
        if (binCount > 2) heights[binCount - 3] = std::min(heights[binCount - 3], 2.0f);

        // 4. Max drift height clamp: extra snow simply disappears
        for (int i = 0; i < binCount; i++) {
            if (heights[i] > maxDriftHeight) {
                heights[i] = maxDriftHeight;
            }
        }
    }

    void ClearDrift() {
        std::fill(heights.begin(), heights.end(), 0.0f);
    }
};

class PhysicsEngine {
public:
    int screenW = 1920;
    int screenH = 1080;
    int taskbarTop = 1040;
    std::vector<Particle> flakes;
    std::map<HWND, WindowSnowdrift> windowDrifts;
    HWND ignoredHwnd = NULL;
    HWND settingsHwnd = NULL;
    float timeAccum = 0;
    float lastScanTime = 0;

    void Init(int w, int h, const SnowConfig& cfg) {
        screenW = w;
        screenH = h;
        taskbarTop = h;
        flakes.resize(cfg.flakeCount);
        for (auto& f : flakes) {
            f.Reset(screenW, screenH, false, cfg.flakeSize);
        }
    }

    void UpdateFlakeCount(const SnowConfig& cfg) {
        int count = cfg.flakeCount;
        int cur = (int)flakes.size();
        if (count > cur) {
            for (int i = cur; i < count; i++) {
                Particle f;
                f.Reset(screenW, screenH, true, cfg.flakeSize);
                flakes.push_back(f);
            }
        } else if (count < cur) {
            flakes.resize(count);
        }
    }

    void RefreshWindowPositions(float dt) {
        RECT r = {0};
        std::vector<HWND> dead;
        for (auto& pair : windowDrifts) {
            HWND h = pair.first;
            if (!IsWindow(h) || !IsWindowVisible(h) || IsIconic(h) || IsZoomed(h)) {
                dead.push_back(h);
                continue;
            }
            HRESULT hr = DwmGetWindowAttribute(h, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r));
            if (FAILED(hr)) {
                GetWindowRect(h, &r);
            }
            pair.second.UpdatePosition(r, dt);
        }
        for (HWND h : dead) {
            windowDrifts.erase(h);
        }
    }

    void ScanWindows() {
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto self = (PhysicsEngine*)lParam;
            if (hwnd == self->ignoredHwnd || hwnd == self->settingsHwnd) return TRUE;
            if (!IsWindowVisible(hwnd) || IsIconic(hwnd) || IsZoomed(hwnd)) return TRUE;

            int cloaked = 0;
            DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
            if (cloaked != 0) return TRUE;

            wchar_t className[128];
            GetClassNameW(hwnd, className, 128);
            if (wcscmp(className, L"Progman") == 0 || wcscmp(className, L"WorkerW") == 0) return TRUE;

            if (wcscmp(className, L"Shell_TrayWnd") == 0) {
                RECT tbRect;
                GetWindowRect(hwnd, &tbRect);
                self->taskbarTop = tbRect.top;
                return TRUE;
            }

            RECT r;
            HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r));
            if (FAILED(hr)) GetWindowRect(hwnd, &r);

            int w = r.right - r.left;
            int h = r.bottom - r.top;
            if (w < 80 || h < 40) return TRUE;
            if (r.left <= 0 && r.top <= 0 && w >= self->screenW && h >= self->screenH) return TRUE;

            if (self->windowDrifts.find(hwnd) == self->windowDrifts.end()) {
                self->windowDrifts[hwnd] = WindowSnowdrift(hwnd, r);
            }
            return TRUE;
        }, (LPARAM)this);
    }

    void Update(float dt, const SnowConfig& cfg) {
        if (cfg.paused || dt <= 0.0001f) return;
        dt = std::min(dt, 0.04f);
        timeAccum += dt;

        // Ultra-fast 60 FPS update of existing window bounds
        RefreshWindowPositions(dt);

        // Scan for new windows every 160ms
        if (timeAccum - lastScanTime > 0.16f) {
            ScanWindows();
            lastScanTime = timeAccum;
        }

        float windGust = std::sin(timeAccum * 0.5f) * 14.0f + std::sin(timeAccum * 1.3f) * 7.0f;
        float curWind = cfg.wind + windGust;

        // Step snowdrifts on all window roofs (edges taper cleanly to 0, NO SPILLING)
        for (auto& pair : windowDrifts) {
            pair.second.StepPhysics(dt, cfg.meltSpeed, cfg.maxDriftHeight, cfg.reposeSlope);
        }

        float floorLimit = (float)std::min(taskbarTop, screenH);

        for (auto& f : flakes) {
            f.swayPhase += f.swaySpeed * dt;
            float swayForce = std::sin(f.swayPhase) * f.swayAmp;

            f.vy = std::min(f.vy + cfg.gravity * dt, 260.0f * f.mass);
            float effVx = f.vx + curWind + swayForce;

            float nextX = f.x + effVx * dt;
            float nextY = f.y + f.vy * dt;

            // 1. BOTTOM CHECK: Immediately disappear at bottom (never accumulate!)
            if (nextY >= floorLimit - 3.0f) {
                f.Reset(screenW, screenH, true, cfg.flakeSize);
                continue;
            }

            // 2. WINDOW COLLISION:
            // When a snowflake hits ANY part of a window or roof:
            // IT IMMEDIATELY DISAPPEARS (Zero bouncing, Zero spilling from edges!)
            bool absorbed = false;

            for (auto& pair : windowDrifts) {
                auto& drift = pair.second;
                const RECT& wr = drift.rect;
                float L = (float)wr.left;
                float T = (float)wr.top;
                float R = (float)wr.right;
                float B = (float)wr.bottom;
                float r = f.size;

                // A. Check collision with roof and snow mound:
                if (nextX >= L && nextX <= R) {
                    float relX = nextX - L;
                    float pileHeight = drift.GetHeightAt(relX);
                    float surfaceY = T - pileHeight;

                    // If flake crosses the surface of the roof mound:
                    if (f.y <= surfaceY + 6.0f && nextY >= surfaceY - r) {
                        // Deposit snow onto the roof mound:
                        if (cfg.enableDrifts) {
                            drift.Deposit(relX, f.size * 0.8f * cfg.driftGrowthRate);
                        }
                        // IMMEDIATELY VANISH / MELT (no bounce, no spilling!)
                        f.Reset(screenW, screenH, true, cfg.flakeSize);
                        absorbed = true;
                        break;
                    }
                }

                // B. Check collision with the rest of the window body / side borders:
                // If a snowflake touches the window ANYWHERE [L - r, R + r, T, B]:
                // IT IMMEDIATELY VANISHES! (Does NOT get shoved to the sides, does NOT spill!)
                if (nextX >= L - r && nextX <= R + r && nextY >= T && nextY <= B) {
                    f.Reset(screenW, screenH, true, cfg.flakeSize);
                    absorbed = true;
                    break;
                }
            }

            if (absorbed) {
                continue;
            }

            f.vx *= 0.94f;
            f.x = nextX;
            f.y = nextY;

            if (f.x < -60.0f) f.x = (float)(screenW + 30);
            else if (f.x > (float)(screenW + 60)) f.x = -30.0f;
        }
    }

    void ClearAllDrifts() {
        for (auto& pair : windowDrifts) {
            pair.second.ClearDrift();
        }
    }
};
