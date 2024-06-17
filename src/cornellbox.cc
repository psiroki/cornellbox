#include <stdlib.h> // > cbx.ppm
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <iostream>
#ifdef __linux__
#include <unistd.h>
#endif

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

inline uint32_t ablend(uint32_t col, uint8_t alpha) {
	uint64_t v = 
		(((col & 0xff0000ULL) << 16) |
		((col & 0xff00ULL) << 8) |
		col & 0xffULL) * alpha;
	return ((v >> 24) & 0xff0000) |
		((v >> 16) & 0xff00) |
		((v >> 8) & 0xff);
}

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
#ifdef RED_BLUE_SWAP
                attenuation = attenuation * Vec(0.2f, 0.01f, 0.01f);
#else
                attenuation = attenuation * Vec(0.01f, 0.01f, 0.2f);
#endif
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
#ifdef RED_BLUE_SWAP
            attenuation = attenuation * Vec(0.98f*base, 0.72f*base, 0.16f*base);
#else
            attenuation = attenuation * Vec(0.16f*base, 0.72f*base, 0.98f*base);
#endif
        }
        if (hitType == HIT_LIGHT) {
#ifdef RED_BLUE_SWAP
            color = color + attenuation * Vec(50, 80, 100);
#else
            color = color + attenuation * Vec(100, 80, 50);
#endif
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


class Visualizer {
  int w, h;
  SDL_Surface *screen;
  SDL_Surface *rendered;
  SDL_Surface *overlay;
  SDL_Surface *lastText;
  SDL_Joystick *joystick;
  TTF_Font *font;
  const char *diagnosticLine;
public:
  Visualizer(int w, int h):
    w(w), h(h),
    diagnosticLine(nullptr),
    lastText(nullptr) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
    joystick = SDL_JoystickOpen(0);
    SDL_ShowCursor(0);
    screen = SDL_SetVideoMode(w, h, 32, 0);
    rendered = SDL_CreateRGBSurface(0, w, h, 32,
        screen->format->Rmask,
        screen->format->Gmask,
        screen->format->Bmask,
        screen->format->Amask);
    TTF_Init();
    font = TTF_OpenFont("assets/RobotoMono-Regular.ttf", 25);
  }

  ~Visualizer() {
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
    if (joystick) {
      SDL_JoystickClose(joystick);
      joystick = nullptr;
    }
    SDL_Quit();
  }

  inline int getWidth() const {
    return w;
  }

  inline int getHeight() const {
    return h;
  }

  void setDiagnosticLine(const char *str) {
    diagnosticLine = str;
    if (lastText) {
      SDL_FreeSurface(lastText);
      lastText = nullptr;
    }
  }

  void drawRow(int y, uint8_t *row) {
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
        target[0] = source[0];
        target[1] = source[1];
        target[2] = source[2];
        target[3] = source[3];
#ifndef FLIP_SCREEN
        target += 4;
#endif
        source += 4;
      }
      SDL_UnlockSurface(rendered);
    }
  }

  void present() {
    SDL_BlitSurface(rendered, nullptr, screen, nullptr);
    if (diagnosticLine) {
      if (!lastText) {
        SDL_Color col = { 255, 255, 255 };
        lastText = TTF_RenderText_Blended(font, diagnosticLine, col);
        SDL_LockSurface(lastText);
        uint32_t *pixel = reinterpret_cast<uint32_t*>(lastText->pixels);
        const int w = lastText->w;
        const int h = lastText->h;
        const int p = lastText->pitch >> 2;
        const int pc = -p - w;
        // shadow is the shadow source, the last line cannot cast a shadow
        uint32_t *shadow = pixel + p * (h - 2);
        for (int j = h-1; j--;) {
          for (int i = w; i--;) {
            uint32_t *dst = shadow + p;
            uint8_t da = *dst >> 24;
            if (da < 255) {
              uint32_t src = *shadow;
              uint8_t sa = src >> 24;
              uint8_t srcAlpha = (sa * (255 - da)) >> 8;
              uint8_t dstAlpha = da;
              *dst = ablend(*dst, dstAlpha) | ((srcAlpha + dstAlpha) << 24);
            }
            ++shadow;
          }
          shadow += pc;
        }
#ifdef FLIP_SCREEN
        for (int y = h >> 1; y--;) {
          uint32_t *src = pixel + y * p;
          uint32_t *dst = pixel + (h - y) * p;
          for (int x = w; x--;) {
            --dst;
            uint32_t s = *src;
            *src = *dst;
            *dst = s;
            ++src;
          }
        }
#endif
        SDL_UnlockSurface(lastText);
      }
      SDL_BlitSurface(lastText, nullptr, screen, nullptr);
    }
    SDL_Flip(screen);
  }

  bool promptLongPress(const char *q) {
    setDiagnosticLine(q);
    bool keys[static_cast<int>(SDLK_LAST)], buttons[256];
    memset(keys, 0, sizeof(keys));
    memset(buttons, 0, sizeof(buttons));
    int currentlyDown = 0;
    bool pressed = false;
    bool result = false;
    while (!pressed || currentlyDown > 0) {
      present();
      SDL_Event event;
      bool firstEvent = true;
      while (firstEvent && SDL_WaitEvent(&event) || !firstEvent && SDL_PollEvent(&event)) {
        firstEvent = false;
        bool downIncreased = false;
        if (event.type == SDL_KEYDOWN) {
          pressed = true;
          int index = static_cast<int>(event.key.keysym.sym);
          if (!keys[index]) {
            keys[index] = true;
            ++currentlyDown;
            downIncreased = true;
          }
        } else if (event.type == SDL_KEYUP) {
          int index = static_cast<int>(event.key.keysym.sym);
          if (keys[index]) {
            keys[index] = false;
            --currentlyDown;
          }
        }
        if (event.type == SDL_JOYBUTTONDOWN) {
          Uint8 index = event.jbutton.button;
          if (!buttons[index]) {
            buttons[index] = true;
            ++currentlyDown;
            downIncreased = true;
          }
        } else if (event.type == SDL_JOYBUTTONUP) {
          Uint8 index = event.jbutton.button;
          if (buttons[index]) {
            buttons[index] = false;
            --currentlyDown;
          }
        }
        if (downIncreased && currentlyDown >= 2) result = !result;
      }
      if (pressed) {
        SDL_Rect grayRect = {
          .x = static_cast<Sint16>(!result ? rendered->w >> 1 : 0),
          .y = 0,
          .w = static_cast<Uint16>(rendered->w >> 1),
          .h = static_cast<Uint16>(rendered->h),
        };
        SDL_FillRect(rendered, &grayRect, 0xff404040U);
        SDL_Rect blackRect = {
          .x = static_cast<Sint16>(result ? rendered->w >> 1 : 0),
          .y = 0,
          .w = static_cast<Uint16>(rendered->w >> 1),
          .h = static_cast<Uint16>(rendered->h),
        };
        SDL_FillRect(rendered, &blackRect, 0xff000000U);
      }
    }
    SDL_Rect blackRect = {
      .x = 0,
      .y = 0,
      .w = static_cast<Uint16>(rendered->w),
      .h = static_cast<Uint16>(rendered->h),
    };
    SDL_FillRect(rendered, &blackRect, 0xff000000U);
    return result;
  }
};

class Renderer {
    friend Visualizer;
    static const int maxNumThreads = 64;
    friend ThreadLocals;
    const int w, h, samplesCount;
    Vec camera, right, up, forward;
    uint8_t *row;
    float *samples;
    int x, y;
    float focalLength, aperture, focusDistance, imageDistance, ipOffsetMultiplier;
    int numThreads;
    Random commonRandom;
    ThreadLocals threads[maxNumThreads];
    Visualizer &v;

    void renderPixel(Random &r, int x, int y, int numSamples) {
        uint8_t *c = row + (w - 1 - x)*4;
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
        *c++ = 255;
    }
public:
    Renderer(Visualizer &v, int samplesCount, int numThreads) : v(v),
        w(v.getWidth()), h(v.getHeight()),
        samplesCount(samplesCount),
        numThreads(numThreads),
        camera(0.0f, 0.0f, -10.8f),
        right((float) w / h, 0.0f),
        up(0.0f, 1.0f),
        forward(0.0, 0.0, 1.0),
        row(new uint8_t[w*4]),
        samples(new float[w*h*4]),
        focalLength(36.0f / (2.0f * right.x())),
        aperture(1.2f),
        focusDistance(15.8f),
        imageDistance(1.0f),
        ipOffsetMultiplier(focalLength / (18.0f * 2.0f) / aperture) {
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
        delete[] row;
        delete[] samples;
        row = nullptr;
        samples = nullptr;
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
        v.drawRow(y, row);
        return row;
    }

    inline void present() {
      v.present();
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
  static Uint8 lastButton = ~0;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYUP) {
      lastKey = event.key.keysym.sym;
      std::cerr << "Key: " << SDL_GetKeyName(lastKey) << std::endl;
    }
    if (event.type == SDL_JOYBUTTONUP) {
      lastButton = event.jbutton.button;
      std::cerr << "Button: " << event.jbutton.button << std::endl;
    }
    if (event.type == SDL_QUIT ||
        event.type == SDL_JOYBUTTONDOWN && event.jbutton.button == lastButton ||
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
  Visualizer visualizer(640, 480);
  int numThreads = 2;
  int numCpus = -1;
#ifdef __linux__
  numCpus = sysconf(_SC_NPROCESSORS_ONLN);
  if (numCpus > 0) numThreads = numCpus;
  if (visualizer.promptLongPress(" A+B: single core     just A: multicore")) {
    numThreads = 1;
  }
#endif
  Renderer renderer(visualizer, samplesPerPass, numThreads);
  renderer.dumpParameters();
  printf("P6 %d %d 255 ", renderer.getWidth(), renderer.getHeight());
  int last = time(NULL);
  int start = last;
  const int passedHeight = renderer.getHeight() * passes;
  bool quit = false;
  char info[1024];
  int linesLeft = 1;
  for (int pass = passes; pass--;) {
    int passBase = renderer.getHeight() * (passes - pass - 1);
    for (int y=renderer.getHeight(); y--;) {
      --linesLeft;
      bool present = linesLeft <= 0 || y == 0;
      if (present) {
        linesLeft = 64;
        int current = time(NULL);
        int overall = current - start;

        int progress = passBase + (renderer.getHeight() - y);
        int remaining = passedHeight - progress;
        int left = (float) overall * remaining / progress;
        int expected = (float) overall * passedHeight / progress;
        if (left <= 0) {
          left = 0;
          expected = overall;
        }

        snprintf(info, sizeof(info), "%6.2f%% %3d:%02d:%02d %3d:%02d:%02d %3d:%02d:%02d (%d)",
                progress*100.0f/passedHeight,
                overall / 3600, overall / 60 % 60, overall % 60,
                left / 3600, left / 60 % 60, left % 60,
                expected / 3600, expected / 60 % 60, expected % 60, numThreads);
        fprintf(stderr, "\r%s %d %d (%d)", info, progress, passedHeight, passBase);
        last = current;
        visualizer.setDiagnosticLine(info);
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
