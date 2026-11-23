#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include "physics.h"
#include "config.h"

using namespace Gdiplus;

class SnowRenderer {
public:
    int screenX = 0;
    int screenY = 0;
    int screenW = 1920;
    int screenH = 1080;
    HDC hdcMem = NULL;
    HBITMAP hBitmap = NULL;
    HBITMAP hOldBitmap = NULL;
    void* pBits = NULL;

    void Init(HDC hdcScreen, int x, int y, int w, int h) {
        screenX = x;
        screenY = y;
        screenW = w;
        screenH = h;
        hdcMem = CreateCompatibleDC(hdcScreen);

        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h; // Top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    }

    void Cleanup() {
        if (hdcMem) {
            SelectObject(hdcMem, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcMem);
            hdcMem = NULL;
            pBits = NULL;
        }
    }

    void Clear(HWND hwndOverlay) {
        if (!pBits || !hdcMem) return;
        memset(pBits, 0, screenW * screenH * 4);
        POINT ptSrc = {0, 0};
        SIZE sizeWnd = {screenW, screenH};
        BLENDFUNCTION blend = {0};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        POINT ptDst = {screenX, screenY};
        UpdateLayeredWindow(hwndOverlay, NULL, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    }

    void Render(HWND hwndOverlay, const PhysicsEngine& physics, const SnowConfig& cfg) {
        if (!pBits || !hdcMem) return;

        // 1. Clear buffer to 0 (fully transparent RGBA: 0x00000000)
        memset(pBits, 0, screenW * screenH * 4);

        // 2. Render within an explicit scope using PixelFormat32bppPARGB (premultiplied alpha)
        // so Gdiplus flushes and releases the memory buffer before UpdateLayeredWindow is called!
        {
            Bitmap bmp(screenW, screenH, screenW * 4, PixelFormat32bppPARGB, (BYTE*)pBits);
            Graphics g(&bmp);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

            // DRAW SNOWDRIFTS (Mounds of snow resting on window roofs)
            if (cfg.enableDrifts) {
                SolidBrush driftBody(Color(250, cfg.colorR, cfg.colorG, cfg.colorB));
                SolidBrush driftFluff(Color(220, cfg.colorR, cfg.colorG, cfg.colorB));
                Pen crestPen(Color(255, 255, 255, 255), 1.8f);

                for (const auto& pair : physics.windowDrifts) {
                    const auto& drift = pair.second;
                    if (drift.binCount <= 0) continue;

                    bool hasSnow = false;
                    for (float h : drift.heights) {
                        if (h > 0.6f) { hasSnow = true; break; }
                    }
                    if (!hasSnow) continue;

                    // Build polygon for the snow pile on the window roof
                    std::vector<PointF> pts;
                    pts.push_back(PointF((float)drift.rect.left, (float)drift.rect.top));
                    pts.push_back(PointF((float)drift.rect.right, (float)drift.rect.top));

                    for (int i = drift.binCount - 1; i >= 0; i--) {
                        float px = (float)drift.rect.left + i * WindowSnowdrift::BIN_WIDTH + WindowSnowdrift::BIN_WIDTH / 2.0f;
                        float py = (float)drift.rect.top - drift.heights[i];
                        pts.push_back(PointF(px, py));
                    }

                    if (pts.size() >= 3) {
                        GraphicsPath path;
                        path.AddPolygon(pts.data(), (int)pts.size());
                        g.FillPath(&driftBody, &path);

                        // Draw soft fluffy bumps along the top surface
                        for (int i = 0; i < drift.binCount; i += 2) {
                            if (drift.heights[i] > 1.5f) {
                                float px = (float)drift.rect.left + i * WindowSnowdrift::BIN_WIDTH;
                                float py = (float)drift.rect.top - drift.heights[i];
                                float puffR = 3.5f + (float)(i % 3);
                                g.FillEllipse(&driftFluff, px - puffR, py - puffR * 0.7f, puffR * 2.0f, puffR * 1.4f);
                            }
                        }

                        // Crest curve highlight
                        std::vector<PointF> crest;
                        for (int i = 0; i < drift.binCount; i++) {
                            float px = (float)drift.rect.left + i * WindowSnowdrift::BIN_WIDTH + WindowSnowdrift::BIN_WIDTH / 2.0f;
                            float py = (float)drift.rect.top - drift.heights[i];
                            crest.push_back(PointF(px, py));
                        }
                        if (crest.size() >= 2) {
                            g.DrawCurve(&crestPen, crest.data(), (int)crest.size());
                        }
                    }
                }
            }

            // DRAW FALLING SNOWFLAKES
            if (cfg.style == 0) {
                // Fluffy Snow (soft edges + core)
                SolidBrush coreBrush(Color(245, cfg.colorR, cfg.colorG, cfg.colorB));
                SolidBrush rimBrush(Color(70, cfg.colorR, cfg.colorG, cfg.colorB));
                for (const auto& f : physics.flakes) {
                    g.FillEllipse(&rimBrush, f.x - f.size * 1.4f, f.y - f.size * 1.4f, f.size * 2.8f, f.size * 2.8f);
                    g.FillEllipse(&coreBrush, f.x - f.size, f.y - f.size, f.size * 2.0f, f.size * 2.0f);
                }
            } else if (cfg.style == 1) {
                // Soft Glow
                for (const auto& f : physics.flakes) {
                    int a = (int)(f.alpha * 240);
                    SolidBrush coreBrush(Color(a, cfg.colorR, cfg.colorG, cfg.colorB));
                    SolidBrush haloBrush(Color(a / 3, cfg.colorR, cfg.colorG, cfg.colorB));
                    g.FillEllipse(&haloBrush, f.x - f.size * 1.9f, f.y - f.size * 1.9f, f.size * 3.8f, f.size * 3.8f);
                    g.FillEllipse(&coreBrush, f.x - f.size, f.y - f.size, f.size * 2.0f, f.size * 2.0f);
                }
            } else if (cfg.style == 2) {
                // Crystals (Snowflake glyphs)
                FontFamily ff(L"Segoe UI Symbol");
                Gdiplus::Font font(&ff, (float)(int)(cfg.flakeSize * 2.8f + 6.0f), FontStyleBold, UnitPixel);
                StringFormat sf;
                sf.SetAlignment(StringAlignmentCenter);
                sf.SetLineAlignment(StringAlignmentCenter);

                const wchar_t* glyphs[3] = { L"❄", L"❅", L"✻" };
                for (const auto& f : physics.flakes) {
                    int a = (int)(f.alpha * 255);
                    SolidBrush brush(Color(a, cfg.colorR, cfg.colorG, cfg.colorB));
                    const wchar_t* gStr = glyphs[f.colorMode % 3];
                    RectF layoutRect(f.x - 12, f.y - 12, 24, 24);
                    g.DrawString(gStr, 1, &font, layoutRect, &sf, &brush);
                }
            } else if (cfg.style == 3) {
                // Cyber Neon
                const Color neonPalette[4] = {
                    Color(240, 0, 240, 255),    // Cyan
                    Color(240, 255, 0, 180),    // Magenta
                    Color(240, 140, 90, 255),   // Violet
                    Color(240, 60, 255, 200)    // Ice Mint
                };
                for (const auto& f : physics.flakes) {
                    Color c = neonPalette[f.colorMode % 4];
                    SolidBrush b(c);
                    SolidBrush halo(Color(60, c.GetRed(), c.GetGreen(), c.GetBlue()));
                    g.FillEllipse(&halo, f.x - f.size * 1.7f, f.y - f.size * 1.7f, f.size * 3.4f, f.size * 3.4f);
                    g.FillEllipse(&b, f.x - f.size, f.y - f.size, f.size * 2.0f, f.size * 2.0f);
                }
            } else {
                // Minimal Dots
                SolidBrush b(Color(220, cfg.colorR, cfg.colorG, cfg.colorB));
                for (const auto& f : physics.flakes) {
                    g.FillEllipse(&b, f.x - f.size, f.y - f.size, f.size * 2.0f, f.size * 2.0f);
                }
            }

            g.Flush(FlushIntentionSync);
        } // g and bmp cleanly finalize and flush here!

        // 3. Blit to layered desktop window
        POINT ptSrc = {0, 0};
        SIZE sizeWnd = {screenW, screenH};
        BLENDFUNCTION blend = {0};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        POINT ptDst = {screenX, screenY};
        UpdateLayeredWindow(hwndOverlay, NULL, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    }
};
