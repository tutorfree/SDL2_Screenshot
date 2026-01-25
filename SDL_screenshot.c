#include <SDL2/SDL.h>
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define HANDLE_SIZE 10

typedef struct {
    int x, y, w, h;
} CaptureRegion;

// Fonte bitmap minimalista (8x8) para desenhar texto sem SDL_ttf
unsigned char font8x8[128][8] = {
    // Organizada em ordem alfabética
    ['A'] = {0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x00},
    ['C'] = {0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0x00},
    ['E'] = {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0x00},
    ['G'] = {0x3C, 0x42, 0x40, 0x4E, 0x42, 0x42, 0x3C, 0x00},
    ['H'] = {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['L'] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x00},
    ['N'] = {0x42, 0x62, 0x52, 0x4A, 0x46, 0x42, 0x42, 0x00},
    ['O'] = {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00},
    ['P'] = {0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40, 0x40, 0x00},
    ['R'] = {0x7C, 0x42, 0x42, 0x7C, 0x48, 0x44, 0x42, 0x00},
    ['S'] = {0x7E, 0x40, 0x40, 0x3E, 0x02, 0x02, 0x7C, 0x00},
    ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['U'] = {0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00},
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

void DrawChar(SDL_Renderer* renderer, char c, int x, int y, int scale) {
    unsigned char* glyph = font8x8[(int)c];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (glyph[i] & (1 << (7 - j))) {
                SDL_Rect r = { x + j * scale, y + i * scale, scale, scale };
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
}

void DrawString(SDL_Renderer* renderer, const char* str, int x, int y, int scale) {
    while (*str) {
        DrawChar(renderer, *str++, x, y, scale);
        x += 9 * scale;
    }
}

SDL_Surface* CaptureRegionGDI(int x, int y, int w, int h) {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
    BitBlt(hMemoryDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY);
    BITMAPINFOHEADER bi = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (surface) {
        GetDIBits(hMemoryDC, hBitmap, 0, h, surface->pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        // GDI retorna BGRA, SDL_PIXELFORMAT_RGBA32 espera RGBA, vamos ajustar se necessário ou usar o formato correto
        // Para o stb_image_write, o formato RGBA (4 canais) é ideal.
    }
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    return surface;
}

void SaveAsJpg(SDL_Surface* surface, const char* filename) {
    // stb_image_write espera pixels em formato RGBA
    // Como o GDI captura em BGRA, precisamos trocar R e B
    unsigned char* pixels = (unsigned char*)surface->pixels;
    for (int i = 0; i < surface->w * surface->h * 4; i += 4) {
        unsigned char temp = pixels[i]; // B
        pixels[i] = pixels[i + 2];     // R
        pixels[i + 2] = temp;          // B
    }
    stbi_write_jpg(filename, surface->w, surface->h, 4, surface->pixels, 90);
}

CaptureRegion SelectRegionAdvanced() {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    SDL_Window* overlay = SDL_CreateWindow("Selecao", 0, 0, screenW, screenH, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_SetWindowOpacity(overlay, 0.4f);
    SDL_Renderer* renderer = SDL_CreateRenderer(overlay, -1, SDL_RENDERER_ACCELERATED);
    SDL_Cursor* cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    SDL_SetCursor(cursor);

    CaptureRegion reg = {screenW/4, screenH/4, screenW/2, screenH/2};
    bool dragging = false, done = false, confirmed = false;
    int resizingHandle = -1;
    SDL_Event e;

    while (!done) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) done = true;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) done = true;
                if (e.key.keysym.sym == SDLK_RETURN) { confirmed = true; done = true; }
            }
            int mx = e.button.x, my = e.button.y;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (abs(mx - reg.x) < HANDLE_SIZE && abs(my - reg.y) < HANDLE_SIZE) resizingHandle = 0;
                else if (abs(mx - (reg.x + reg.w)) < HANDLE_SIZE && abs(my - reg.y) < HANDLE_SIZE) resizingHandle = 1;
                else if (abs(mx - reg.x) < HANDLE_SIZE && abs(my - (reg.y + reg.h)) < HANDLE_SIZE) resizingHandle = 2;
                else if (abs(mx - (reg.x + reg.w)) < HANDLE_SIZE && abs(my - (reg.y + reg.h)) < HANDLE_SIZE) resizingHandle = 3;
                else if (mx > reg.x && mx < reg.x + reg.w && my > reg.y && my < reg.y + reg.h) dragging = true;
            }
            if (e.type == SDL_MOUSEMOTION) {
                if (resizingHandle == 0) { reg.w += (reg.x - mx); reg.h += (reg.y - my); reg.x = mx; reg.y = my; }
                else if (resizingHandle == 1) { reg.w = mx - reg.x; reg.h += (reg.y - my); reg.y = my; }
                else if (resizingHandle == 2) { reg.x = mx; reg.w += (reg.x - mx); reg.h = my - reg.y; }
                else if (resizingHandle == 3) { reg.w = mx - reg.x; reg.h = my - reg.y; }
                else if (dragging) { reg.x = mx - reg.w/2; reg.y = my - reg.h/2; }
            }
            if (e.type == SDL_MOUSEBUTTONUP) { dragging = false; resizingHandle = -1; }
        }
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        SDL_Rect mainRect = {reg.x, reg.y, reg.w, reg.h};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 40);
        SDL_RenderFillRect(renderer, &mainRect);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &mainRect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect h[4] = {{reg.x-5, reg.y-5, 10, 10}, {reg.x+reg.w-5, reg.y-5, 10, 10}, {reg.x-5, reg.y+reg.h-5, 10, 10}, {reg.x+reg.w-5, reg.y+reg.h-5, 10, 10}};
        for(int i=0; i<4; i++) SDL_RenderFillRect(renderer, &h[i]);
        DrawString(renderer, "ENTER PARA CAPTURAR", reg.x, reg.y - 25, 2);
        SDL_RenderPresent(renderer);
    }
    SDL_FreeCursor(cursor); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(overlay);
    if (!confirmed) reg.w = 0;
    return reg;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("SDL2 Capture JPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 450, 250, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    SDL_Rect btnFull = {50, 50, 350, 60};
    SDL_Rect btnReg = {50, 130, 350, 60};

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                if (mx > btnFull.x && mx < btnFull.x + btnFull.w && my > btnFull.y && my < btnFull.y + btnFull.h) {
                    SDL_HideWindow(win); SDL_Delay(300);
                    SDL_Surface* s = CaptureRegionGDI(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                    SaveAsJpg(s, "captura_total.jpg");
                    SDL_FreeSurface(s); SDL_ShowWindow(win);
                } else if (mx > btnReg.x && mx < btnReg.x + btnReg.w && my > btnReg.y && my < btnReg.y + btnReg.h) {
                    SDL_HideWindow(win);
                    CaptureRegion r = SelectRegionAdvanced();
                    if (r.w > 0) {
                        SDL_Surface* s = CaptureRegionGDI(r.x, r.y, r.w, r.h);
                        SaveAsJpg(s, "captura_regiao.jpg");
                        SDL_FreeSurface(s);
                    }
                    SDL_ShowWindow(win);
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255);
        SDL_RenderFillRect(renderer, &btnFull);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        DrawString(renderer, "TELA CHEIA", btnFull.x + 80, btnFull.y + 20, 2);

        SDL_SetRenderDrawColor(renderer, 16, 124, 16, 255);
        SDL_RenderFillRect(renderer, &btnReg);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        DrawString(renderer, "SELECIONAR REGIAO", btnReg.x + 35, btnReg.y + 20, 2);

        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
