#include <iostream>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_TRANSFORMEDVIEW
#include "olcPGEX_TransformedView.h"

namespace olc {
struct rect {
  olc::vf2d pos; // generic 2d float vector
  olc::vf2d size;


  rect(const olc::vf2d &p = {0.0f, 0.0f}, const olc::vf2d &s = {1.0f, 1.0f}) : pos(p), size(s) {}

  [[nodiscard]] constexpr bool containsPoint(const olc::vf2d &p) const {
    return !(p.x < pos.x || p.y < pos.y || p.x >= pos.x + size.x || p.y >= pos.y + size.y);
  }

  [[nodiscard]] constexpr bool containsRect(const olc::rect &r) const {
    return (r.pos.x >= pos.x) && (r.pos.x + r.size.x < pos.x + size.x) && (r.pos.y >= pos.y)
        && (r.pos.y + r.size.y < pos.y + size.y);
  }

  [[nodiscard]] constexpr bool overlaps(const olc::rect &r) const {
    return (pos.x < r.pos.x + r.size.x && pos.x + size.x >= r.pos.x && pos.y < r.pos.y + r.size.y
        && pos.y + size.y >= r.pos.y);
  }
};
}

class Example_StaticQuadTree : public olc::PixelGameEngine {
 public:
  Example_StaticQuadTree() {
    sAppName = "Example";
  }

 protected:
  olc::TransformedView tv;

  struct Object2d {
    olc::vf2d vPos;
    olc::vf2d vVel;
    olc::vf2d vSize;
    olc::Pixel colour;
  };

  std::vector<Object2d> vecObjects;

  float fArea = 100'000.0f;

  uint wang_hash(uint32_t seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2ed;
    seed = seed ^ (seed >> 15);
    return seed;
  }

  static uint pcg_hash(uint32_t input) {
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
  }

  static float RandomFloat(uint32_t &seed) {
    seed = pcg_hash(seed);
    return (float) seed / (float) std::numeric_limits<uint32_t>::max();
  }
  uint32_t seed = 124124124;
 public:
  bool OnUserCreate() override {
    tv.Initialise({ScreenWidth(), ScreenHeight()});

    auto rand_float = [this](const float l, const float r) {
      return Example_StaticQuadTree::RandomFloat(this->seed) * (r - l) + l;
    };

    for (int i = 0; i < 1'000'000; i++) {
      Object2d ob;
      ob.vPos = {rand_float(0.0f, fArea), rand_float(0.0f, fArea)};
      ob.vSize = {rand_float(0.1f, 100.0f), rand_float(0.1f, 100.0f)};
      ob.colour = olc::Pixel(static_cast<int>(rand_float(0.0f, 256)),
                             static_cast<int>(rand_float(0.0f,
                                                         256)),
                             static_cast<int>(rand_float(
                                 0.0f,
                                 256)));
      vecObjects.push_back(ob);

    }

    return true;
  }
  bool OnUserUpdate(float fElapsedTime) override {
    tv.HandlePanAndZoom(0);

    olc::rect rScreen = {tv.GetWorldTL(), tv.GetWorldBR() - tv.GetWorldTL()};
    size_t nObjectCount = 0;
    auto tpStart = std::chrono::system_clock::now();
    for (auto const &ob : vecObjects) {
      if (rScreen.overlaps({ob.vPos, ob.vSize})) {
        tv.FillRectDecal(ob.vPos, ob.vSize, ob.colour);
        nObjectCount++;
      }
    }
    std::chrono::duration<float> duration = std::chrono::system_clock::now() - tpStart;
    std::string sOutput = "Linear " + std::to_string(nObjectCount) + "/" + std::to_string(vecObjects.size()) + " in " + std::to_string(duration.count());
    DrawStringDecal({ 4, 4 }, sOutput, olc::BLACK, { 4.0f, 8.0f });
    DrawStringDecal({ 2, 2 }, sOutput, olc::WHITE, { 4.0f, 8.0f });

    return true;
  }
};

int main() {
  Example_StaticQuadTree demo;
  if (demo.Construct(1000, 500, 1, 1, false, false)) demo.Start();
  return 0;
}
