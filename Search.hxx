#pragma once

#include "Tile.hxx"
#include "TileSet.hxx"
#include <utility>
#include <vector>

std::vector<TileSet> getTileSetsIfValid(std::vector<Tile> tiles);
std::pair<std::vector<TileSet>, std::vector<Tile>> solve(std::vector<TileSet>& board, std::vector<Tile>& rack);