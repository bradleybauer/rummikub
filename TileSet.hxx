#pragma once

#include "Tile.hxx"
#include <vector>

struct TileSet {
  std::vector<Tile> tiles;
  TileSet() = default;
  TileSet(const std::vector<Tile>& tiles) : tiles(tiles) {}
  int size() const { return tiles.size(); }
  bool isRun() const;
  bool isGroup() const;
  bool isLegal() const;
  void sortRun();
};