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

constexpr size_t MAX_DEPTH = 8;

template<typename Type>
class StaticQuadTree {
 public:
  StaticQuadTree( size_t nDepth = 0, const olc::rect &rArea = {{0.0f, 0.0f}, {100000.0f, 100000.0f}}) {
    m_depth = nDepth;
    resize(rArea);

  }
  void resize(const olc::rect &rArea) {
    clear();
    m_rect = rArea;
    olc::vf2d vChildSize = m_rect.size / 2.0f;
    m_rChild = {
        olc::rect(m_rect.pos, vChildSize),
        olc::rect({m_rect.pos.x + vChildSize.x, m_rect.pos.y}, vChildSize),
        olc::rect({m_rect.pos.x, m_rect.pos.y + vChildSize.y}, vChildSize),
        olc::rect({m_rect.pos.x + vChildSize.x, m_rect.pos.y + vChildSize.y}, vChildSize),
    };
  }

  void clear() {
    m_pItems.clear();
    for (int i = 0; i < 4; i++) {
      if (m_pChild[i]) m_pChild[i]->clear();
      m_pChild[i].reset();
    }
  }

  size_t size() const {
    size_t nCount = m_pItems.size();
    for (int i = 0; i < 4; i++) if (m_pChild[i]) nCount += m_pChild[i]->size();
    return nCount;
  }

 public:

  void insert(const Type &item, const olc::rect &item_size) {
    for (int i = 0; i < 4; i++) {
      if (m_rChild[i].containsRect(item_size)) {
        if (m_depth + 1 < MAX_DEPTH) {
          if (!m_pChild[i]) {
            m_pChild[i] = std::make_shared<StaticQuadTree<Type>>(m_depth + 1, m_rChild[i]);
          }

          m_pChild[i]->insert(item, item_size);
          return;
        }
      }
    }
    m_pItems.push_back({item_size, item});
  }

  [[nodiscard]] std::list<Type> search(const olc::rect &search_area) const {
    std::list<Type> itemsInside;
    search(search_area, itemsInside);
    return itemsInside;
  }
// Returns the objects in the given search area, by adding to supplied list
  void search(const olc::rect &rArea, std::list<Type> &listItems) const {
    for (auto const &p : m_pItems) {
      if (rArea.overlaps(p.first)) listItems.push_back(p.second);
    }

    for (int i = 0; i < 4; i++) {
      if (m_pChild[i]) {

        if (rArea.containsRect(m_rChild[i])) {
          m_pChild[i]->items(listItems);
        }

        if (rArea.overlaps(m_rChild[i])) {
          m_pChild[i]->search(rArea, listItems);
        }
      }
    }
  }

  void items(std::list<Type> &listItem) const {
    for (auto const &p : m_pItems) listItem.push_back(p.second);

    for (int i = 0; i < 4; i++) if (m_pChild[i]) m_pChild[i]->items(listItem);
  }

  const olc::rect &area() { return m_rect; }

 protected:
  size_t m_depth = 0;
  olc::rect m_rect; // dimensions of the current quadTreeSection
  std::array<olc::rect, 4> m_rChild{}; // dimensions of the children quadTree
  std::array<std::shared_ptr<StaticQuadTree<Type>>, 4> m_pChild{}; // sub QuadTrees in each subsection
  std::vector<std::pair<olc::rect, Type>> m_pItems;
};
template<typename Type>
class StaticQuadTreeContainer {
  // Using a std::list as we dont want pointers to be invalidated to objects stored in the
  // tree should the contents of the tree change
  using QuadTreeContainer = std::list<Type>;

 protected:
  // The actual container
  QuadTreeContainer m_allItems;

  // Use our StaticQuadTree to store "pointers" instead of objects - this reduces
  // overheads when moving or copying objects
  StaticQuadTree<typename QuadTreeContainer::iterator> root;

 public:
  StaticQuadTreeContainer(const olc::rect &size = {{0.0f, 0.0f}, {100.0f, 100.0f}}, const size_t nDepth = 0)
      : root(nDepth, size) {

  }

  // Sets the spatial coverage area of the quadtree
  // Invalidates tree
  void resize(const olc::rect &rArea) {
    root.resize(rArea);
  }

  // Returns number of items within tree
  size_t size() const {
    return m_allItems.size();
  }

  // Returns true if tree is empty
  bool empty() const {
    return m_allItems.empty();
  }

  // Removes all items from tree
  void clear() {
    root.clear();
    m_allItems.clear();
  }

  // Convenience functions for ranged for loop
  typename QuadTreeContainer::iterator begin() {
    return m_allItems.begin();
  }

  typename QuadTreeContainer::iterator end() {
    return m_allItems.end();
  }

  typename QuadTreeContainer::const_iterator cbegin() {
    return m_allItems.cbegin();
  }

  typename QuadTreeContainer::const_iterator cend() {
    return m_allItems.cend();
  }

  void insert(const Type &item, const olc::rect &itemsize) {
    // Item is stored in container
    m_allItems.push_back(item);

    // Pointer/Area of item is stored in quad tree
    root.insert(std::prev(m_allItems.end()), itemsize);
  }

  // Returns a std::list of pointers to items within the search area
  [[nodiscard]] std::list<typename QuadTreeContainer::iterator> search(const olc::rect &rArea) const {
    std::list<typename QuadTreeContainer::iterator> listItemPointers;
    root.search(rArea, listItemPointers);
    return listItemPointers;
  }

};

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
  StaticQuadTreeContainer<Object2d> treeObjects;

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

  bool bUseQuadTree = true;
 public:
  bool OnUserCreate() override {
    tv.Initialise({ScreenWidth(), ScreenHeight()});
    treeObjects.resize(olc::rect({0.0f, 0.0f}, {fArea, fArea}));

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
      treeObjects.insert(ob, olc::rect(ob.vPos, ob.vSize));

    }

    return true;
  }
  bool OnUserUpdate(float fElapsedTime) override {
    if (GetKey(olc::Key::TAB).bPressed)
      bUseQuadTree = !bUseQuadTree;
    tv.HandlePanAndZoom(0);
    olc::rect rScreen = {tv.GetWorldTL(), tv.GetWorldBR() - tv.GetWorldTL()};
    size_t nObjectCount = 0;

    if (bUseQuadTree) {

      auto tpStart = std::chrono::system_clock::now();
      for (auto const &ob : treeObjects.search(rScreen)) {
        tv.FillRectDecal(ob->vPos, ob->vSize, ob->colour);
        nObjectCount++;
      }
      std::chrono::duration<float> duration = std::chrono::system_clock::now() - tpStart;
      std::string
          sOutput = "QuadTree " + std::to_string(nObjectCount) + "/" + std::to_string(vecObjects.size()) + " in "
          + std::to_string(duration.count());
      DrawStringDecal({4, 4}, sOutput, olc::BLACK, {2.0f, 4.0f});
      DrawStringDecal({2, 2}, sOutput, olc::WHITE, {2.0f, 4.0f});
    } else {

      auto tpStart = std::chrono::system_clock::now();
      for (auto const &ob : vecObjects) {
        if (rScreen.overlaps({ob.vPos, ob.vSize})) {
          tv.FillRectDecal(ob.vPos, ob.vSize, ob.colour);
          nObjectCount++;
        }
      }
      std::chrono::duration<float> duration = std::chrono::system_clock::now() - tpStart;
      std::string sOutput = "Linear " + std::to_string(nObjectCount) + "/" + std::to_string(vecObjects.size()) + " in "
          + std::to_string(duration.count());
      DrawStringDecal({4, 4}, sOutput, olc::BLACK, {2.0f, 4.0f});
      DrawStringDecal({2, 2}, sOutput, olc::WHITE, {2.0f, 4.0f});
    }
    return true;
  }
};

int main() {
  Example_StaticQuadTree demo;
  if (demo.Construct(1260, 600, 1, 1, false, false)) demo.Start();
  return 0;
}
