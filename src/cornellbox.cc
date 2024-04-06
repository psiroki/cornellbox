#include <stdlib.h> // > cbx.ppm
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <iostream>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "platform.hh"

const float TAU = 6.283185307179586f;

#ifdef MIYOO
#define FLIP_SCREEN
#endif

struct Random {
    int64_t seed;

    int64_t rand() {
        return seed = (seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
    }

    float randomVal() {
        return (rand() & ((1 << 24) - 1)) / (float) (1 << 24);
    }
};

float min(float a, float b) { return a < b ? a : b; }

float boxTest(const Vec &pos, const Vec &mins, const Vec &maxs) {
    Vec d1(pos - mins);
    Vec d2(maxs - pos);
    float f[4];
    d1.min(d2).flatten(f);
    return min(min(f[0], f[1]), f[2]);
}

float columnTest(const Vec &pos, const Vec &bottomCenter, float r, float height) {
    float ymin = bottomCenter.y();
    float ymax = ymin + height;
    float p[4];
    pos.flatten(p);
    float d1(p[1] - ymin);
    d1 = -min(d1, ymax - p[1]);
    Vec v = Vec(p[0], bottomCenter.y(), p[2]) - bottomCenter;
    float d2 = sqrtf(v | v) - r;
    if (d2 > d1)
        return d2;
    return d1;
}

enum HitType {
    HIT_WHITE,
    HIT_RED,
    HIT_GREEN,
    HIT_GOLD,
    HIT_LIGHT,
};

const float rc = 0.8660254f, rs = -0.5f;
const Vec mx(rc, 0.0f, rs);
const Vec mz(-rs, 0.0f, rc);

float scene(const Vec &pos, int &type) {
    type = HIT_WHITE;
    // room and (rotated) box
    float minDist = min(boxTest(pos, Vec(-10, -10, -10), Vec(10, 10, 10)),
	        -boxTest(mx * pos.x() + Vec(0.0f, pos.y()) + mz * pos.z(), Vec(3, 6, -3), Vec(7, 10, 1)));
    // doorway
    minDist = -min(-minDist,
        -boxTest(pos, Vec(-3.5, -3, -12.5), Vec(3.5, 10, -9)));
    // other room
    minDist = -min(-minDist, -boxTest(pos, Vec(-10, -10, -22), Vec(10, 10, -12)));
    // column
//    minDist = min(minDist, columnTest(pos, Vec(0, -10, 0), 1.0f, 3.0f));
    float sphereDist = (pos - Vec(-6, 7, 5)).length() - 3.0f;
    if (sphereDist < minDist) minDist = sphereDist, type = HIT_GOLD;
    float p[4];
    pos.flatten(p);
    if (type == HIT_WHITE && p[2] < 10.0f) {
		if (p[0] < -9.9f)
		    type = HIT_RED;   // red wall
		if (p[0] > 9.9f)
		    type = HIT_GREEN;   // green wall
		if (p[1] < -9.9f) {
            if (fabsf(p[0]) <= 5.0f && fabs(p[2]) <= 5.0f)
                type = HIT_LIGHT;
			// Vec v(fabsf(p[0]), fabsf(p[2]));
			// float d = v | v;
			// if (d < 25.0f && d > 16.0f) { 
			//     type = HIT_LIGHT;
			// }
		}
    }
    return minDist;
}

int march(const Vec &pos, const Vec &dir, Vec &hitPos, Vec &hitNorm) {
    int type = 0;
    int noHitCount = 0;
    float d;
    for (float traveled = 0.0f; traveled < 100.0f; traveled += d) {
        if ((d = scene(hitPos = pos + dir * traveled, type)) < 0.01f || ++noHitCount > 99) {
            // noHitCount is not used anymore, and we don't care about the result
            hitNorm = Vec(
                scene(hitPos + Vec(0.01f, 0.0f, 0.0f), noHitCount) - d,
                scene(hitPos + Vec(0.0f, 0.01f, 0.0f), noHitCount) - d,
                scene(hitPos + Vec(0.0f, 0.0f, 0.01f), noHitCount) - d
            );
            hitNorm.normalize();
            return type;
        }
    }
    return 0;
}

Vec tracePath(Random &r, Vec origin, Vec direction, int bounceCount = 3) {
    Vec sampledPosition, normal, color, attenuation = 1;
    bool goldBounceAdded = false;
    while (bounceCount--) {
        int hitType = march(origin, direction, sampledPosition, normal);
        if (hitType == HIT_WHITE || hitType == HIT_GREEN || hitType == HIT_RED) {
            float n[4];
            normal.flatten(n);
            float p = TAU * r.randomVal();
            float c = r.randomVal();
            float s = sqrtf(1 - c);
            float g = n[2] < 0 ? -1 : 1;
            float u = -1 / (g + n[2]);
            float v = n[0] * n[1] * u;
            direction = Vec(v,
                            g + n[1] * n[1] * u,
                            -n[1]) * (cosf(p) * s)
                        +
                        Vec(1 + g * n[0] * n[0] * u,
                            g * v,
                            -g * n[0]) * (sinf(p) * s) + normal * sqrtf(c);
            origin = sampledPosition + direction * 0.1f;
            direction.normalize();
            if (hitType == HIT_WHITE)
                attenuation = attenuation * 0.3f;//0.2;
            else if (hitType == HIT_RED)
                attenuation = attenuation * Vec(0.2f, 0.01f, 0.01f);
            else if (hitType == HIT_GREEN)
                attenuation = attenuation * Vec(0.01f, 0.2f, 0.01f);
        }
        if (hitType == HIT_GOLD) {
            if (!goldBounceAdded) {
                goldBounceAdded = true;
                ++bounceCount;
            }
            direction = direction - normal * (2.0f * (direction | normal));
            direction.normalize();
            origin = sampledPosition + direction * 0.1f;
            direction = direction + Vec(r.randomVal()*0.2f-0.1f, r.randomVal()*0.2f-0.1f, r.randomVal()*0.2f-0.1f);
            direction.normalize();
            const float base = 0.8f;
            attenuation = attenuation * Vec(0.98f*base, 0.72f*base, 0.16f*base);
        }
        if (hitType == HIT_LIGHT) {
            color = color + attenuation * Vec(50, 80, 100);
            break;
        }
    }
    return color;
}

class Renderer;

struct ThreadLocals {
    pthread_t thread;
    sem_t restart;
    sem_t ready;
    Random random;
    Renderer *renderer;
    int samplesCount;
    bool sync;

    void* renderThread();
};

void* renderThread(void *localsPtr) {
    return static_cast<ThreadLocals*>(localsPtr)->renderThread();
}

class Renderer {
    static const int numThreads = 2;
    friend ThreadLocals;
    const int w, h, samplesCount;
    Vec camera, right, up, forward;
    uint8_t *row;
    float *samples;
    int x, y;
    float focalLength, aperture, focusDistance, imageDistance, ipOffsetMultiplier;
    Random commonRandom;
    ThreadLocals threads[numThreads];
    SDL_Surface *screen;
    SDL_Surface *rendered;
    SDL_Surface *overlay;
    SDL_Surface *lastText;
    TTF_Font *font;
    const char *diagnosticLine;

    void renderPixel(Random &r, int x, int y, int numSamples) {
        uint8_t *c = row + (w - 1 - x)*3;
        Vec color = Vec(0.0f);
        for (int i = numSamples; --i;) {
            // this is the subpixel we are calculating
            float dx = r.randomVal() - 0.5f;
            float dy = r.randomVal() - 0.5f;
            Vec dir = right * (2.0f * (x + dx) / w - 1) + up * (1.0f - 2.0f * (y + dy) / h) + forward;
            // dir is now projected on the focal plane
            Vec focalPoint = camera + dir * focusDistance;
            Vec ip(r.randomVal(), r.randomVal());
            ip = ip.sqrt()*ipOffsetMultiplier;
            float angle = r.randomVal() * TAU;
            ip = ip * Vec(cosf(angle), sinf(angle));
            Vec imagePlanePixel = camera + dir + right * ip.x() + up * ip.y();
            dir = focalPoint - imagePlanePixel;
            dir.normalize();
            color = color + tracePath(r, imagePlanePixel, dir);
        }
        float *sample = samples + (y * w + x) * 4;
        Vec s = Vec(sample[0], sample[1], sample[2]);
        color = s + color;
        sample[0] = color.x();
        sample[1] = color.y();
        sample[2] = color.z();
        uint32_t &samplesAtPixel(*reinterpret_cast<uint32_t*>(sample+3));
        samplesAtPixel += numSamples;
        color = color * (1.0f / samplesAtPixel) + 14.0f / 241.0f;
        Vec o = color + 1.0f;
        color = color / o * 255.0f;
        *c++ = (int) color.x();
        *c++ = (int) color.y();
        *c++ = (int) color.z();
    }
public:
    Renderer(int w, int h, int samplesCount) : w(w), h(h), samplesCount(samplesCount),
        camera(0.0f, 0.0f, -10.8f),
        right((float) w / h, 0.0f),
        up(0.0f, 1.0f),
        forward(0.0, 0.0, 1.0),
        row(new uint8_t[w*3]),
        samples(new float[w*h*4]),
        focalLength(36.0f / (2.0f * right.x())),
        aperture(1.2f),
        focusDistance(15.8f),
        imageDistance(1.0f),
        ipOffsetMultiplier(focalLength / (18.0f * 2.0f) / aperture),
        diagnosticLine(nullptr),
        lastText(nullptr) {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_ShowCursor(0);
        screen = SDL_SetVideoMode(w, h, 32, 0);
        rendered = SDL_CreateRGBSurface(0, w, h, 32,
            screen->format->Rmask,
            screen->format->Gmask,
            screen->format->Bmask,
            screen->format->Amask);
        TTF_Init();
        font = TTF_OpenFont("assets/RobotoMono-Regular.ttf", 25);
        commonRandom.seed = static_cast<int64_t>(clock());
        for (int i = numThreads; i--; ) {
            ThreadLocals &t(threads[i]);
            t.renderer = this;
            t.samplesCount = samplesCount;
            t.random.seed = commonRandom.seed;
            sem_init(&t.ready, 0, 0);
            sem_init(&t.restart, 0, 0);
            t.sync = i == 0;
            if (!t.sync)
                pthread_create(&t.thread, 0, renderThread, &t);
        }
    }

    ~Renderer() {
        if (lastText) {
          SDL_FreeSurface(lastText);
          lastText = nullptr;
        }
        if (rendered) {
          SDL_FreeSurface(rendered);
          rendered = nullptr;
        }
        TTF_CloseFont(font);
        font = nullptr;
        delete[] row;
        delete[] samples;
        row = 0;
        SDL_Quit();
    }

    void setDiagnosticLine(const char *str) {
      diagnosticLine = str;
      if (lastText) {
        SDL_FreeSurface(lastText);
        lastText = nullptr;
      }
    }

    void dumpParameters() {
        fprintf(stderr, "Rendering %dx%d (samples: %d)\n", w, h, samplesCount);
        fprintf(stderr, "Aperture: f/%.2f\n", aperture);
        fprintf(stderr, "Focal length: %.2f\n", focalLength);
        fprintf(stderr, "ipOffsetMultiplier: %f\n", ipOffsetMultiplier);
    }

    uint8_t* renderRow(int y) {
        this->x = w - 1;
        this->y = y;
        ThreadLocals *syncThread = 0;
        for (int i = numThreads; i--; ) {
            ThreadLocals &t(threads[i]);
            if (!t.sync) {
                sem_post(&t.restart);
            } else {
                syncThread = &t;
            }
        }
        if (syncThread) syncThread->renderThread();
        for (int i = numThreads; i--; ) {
            ThreadLocals &t(threads[i]);
            if (!t.sync) sem_wait(&t.ready);
        }
        if (!SDL_LockSurface(rendered)) {
#ifdef FLIP_SCREEN
          int targetY = y;
#else
          int targetY = (h - y - 1);
#endif
          uint8_t *target = reinterpret_cast<uint8_t*>(rendered->pixels) + targetY * w*4;
          uint8_t *source = row;
#ifdef FLIP_SCREEN
          target += w * 4;
#endif          
          for (int i = 0; i < w; ++i) {
#ifdef FLIP_SCREEN
            target -= 4;
#endif          
            target[3] = 255;
            target[2] = source[0];
            target[1] = source[1];
            target[0] = source[2];
#ifndef FLIP_SCREEN
            target += 4;
#endif
            source += 3;
          }
          SDL_UnlockSurface(rendered);
        }
        return row;
    }

    inline void present() {
      SDL_BlitSurface(rendered, nullptr, screen, nullptr);
      if (diagnosticLine) {
        if (!lastText) {
          SDL_Color col = { 255, 255, 255 };
          lastText = TTF_RenderText_Blended(font, diagnosticLine, col);
          SDL_LockSurface(lastText);
          uint8_t *pixel = reinterpret_cast<uint8_t*>(lastText->pixels);
          const int w = lastText->w;
          const int h = lastText->h;
          const int p = lastText->pitch;
          const int pc = p - w * 4;
#ifdef FLIP_SCREEN
          for (int y = h >> 1; y--;) {
            uint8_t *src = pixel + y * p;
            uint8_t *dst = pixel + (h - y) * p;
            for (int x = w; x--;) {
              dst -= 4;
              int r = src[0], g = src[1], b = src[2], a = src[3];
              src[0] = dst[0];
              src[1] = dst[1];
              src[2] = dst[2];
              src[3] = dst[3];
              dst[0] = r;
              dst[1] = g;
              dst[2] = b;
              dst[3] = a;
              src += 4;
            }
          }
#endif
          for (int j = h-1; j--;) {
            for (int i = w; i--;) {
              uint8_t *src = pixel + p;
              uint8_t a = src[3];
              if (i > 0 && pixel[7] > src[3]) a = pixel[7];
              if (a > pixel[3]) {
                pixel[0] = 0;
                pixel[1] = 0;
                pixel[2] = 0;
                pixel[3] = a;
              }
              pixel += 4;
            }
            pixel += pc;
          }
          SDL_UnlockSurface(lastText);
        }
        SDL_BlitSurface(lastText, nullptr, screen, nullptr);
      }
      SDL_Flip(screen);
    }

    inline int getWidth() {
        return w;
    }

    inline int getHeight() {
        return h;
    }
};

void* ThreadLocals::renderThread() {
    while (true) {
        if (!sync) {
            sem_wait(&restart);
        }
        while (true) {
            int x = __sync_fetch_and_sub(&renderer->x, 1);
            if (x < 0) break;
            renderer->renderPixel(random, x, renderer->y, samplesCount);
        }
        if (sync) {
            break;
        } else {
            sem_post(&ready);
        }
    }
    return nullptr;
}

bool shouldQuit() {
  static SDLKey lastKey;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYUP) {
      lastKey = event.key.keysym.sym;
      std::cerr << "Key: " << SDL_GetKeyName(lastKey) << std::endl;
    }
    if (event.type == SDL_QUIT ||
        event.type == SDL_KEYDOWN &&
            (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == lastKey)) {
      return true;
    }
  }
  return false;
}

int main() {
  const int samplesOverall = 1024;
  const int samplesPerPass = 4;
  const int passes = samplesOverall / samplesPerPass;
  Renderer renderer(640, 480, samplesPerPass);
  renderer.dumpParameters();
  printf("P6 %d %d 255 ", renderer.getWidth(), renderer.getHeight());
  int last = time(NULL);
  int start = last;
  const int passedHeight = renderer.getHeight() * passes;
  bool quit = false;
  char info[1024];
  for (int pass = passes; pass--;) {
    int passBase = renderer.getHeight() * (passes - pass + 1);
    for (int y=renderer.getHeight(); y--;) {
      int current = time(NULL);
      bool present = y < renderer.getHeight() && current > last || y == 0;
      if (present) {
        int overall = current - start;

        int progress = passBase + (renderer.getHeight() - y);
        int remaining = passedHeight - progress;
        int left = (float) overall * remaining / progress;
        int expected = (float) overall * passedHeight / progress;

        snprintf(info, sizeof(info), "\r%6.2f%% %3d:%02d:%02d %3d:%02d:%02d %3d:%02d:%02d",
                progress*100.0f/passedHeight,
                overall / 3600, overall / 60 % 60, overall % 60,
                left / 3600, left / 60 % 60, left % 60,
                expected / 3600, expected / 60 % 60, expected % 60);
        fprintf(stderr, "%s", info);
        last = current;
        renderer.setDiagnosticLine(info);
      }
      uint8_t *c = renderer.renderRow(y);
      if (present) renderer.present();
      if (!pass)
        fwrite(c, renderer.getWidth(), 3, stdout);
      quit = shouldQuit();
      if (quit) break;
    }
    if (quit) break;
  }
  fprintf(stderr, "\n");
  while (!shouldQuit());
  return 0;
}
