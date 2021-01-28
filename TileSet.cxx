#include "TileSet.hxx"
#include <bitset>

bool TileSet::isGroup() const {
  if (tiles.size() < 3 || tiles.size() > 4) {
    return false;
  }
  std::bitset<4> colorBits;
  Tile startTile = tiles[0];
  int numWildCardsInSet = 0;
  for (auto tile : tiles) {
    if (startTile.isJoker) {
      startTile = tile;
    }
    if (tile.isJoker) {
      numWildCardsInSet += 1;
    } else {
      if (tile.faceValue != startTile.faceValue) {
        return false;
      }
      colorBits.set(tile.color);
    }
  }
  return colorBits.count() + numWildCardsInSet == tiles.size();
}

bool TileSet::isRun() const {
  if (tiles.size() < 3 || tiles.size() > 13) {
    return false;
  }
  Tile startTile = tiles[0];
  for (auto tile : tiles) {
    if (startTile.isJoker) {
      startTile = tile;
    }
    if (!tile.isJoker) {
      if (tile.color != startTile.color || tile.faceValue != startTile.faceValue) {
        return false;
      }
    }
    startTile.faceValue += 1;
  }
  return true;
}

bool TileSet::isLegal() const {
  if (size() < 3 || size() > 13) {
    return false;
  }
  return isGroup() || isRun();
}

#include <bitset>
// TODO this sucks
void TileSet::sortRun() {
  // Does nothing if the set is not a run
  std::bitset<13> bits;
  int numJokers = 0;
  int joker1Index = -1;
  int joker2Index = -1;
  int color = 0;
  for (int i = 0; auto& t : tiles) {
    if (!t.isJoker) {
      bits.set(t.faceValue - 1);
      color = t.color;
    } else {
      numJokers += 1;
      if (joker1Index == -1) {
        joker1Index = i;
      } else {
        joker2Index = i;
      }
    }
    i += 1;
  }
  if (tiles.size() == 3 && numJokers == 0) {
    std::sort(tiles.begin(), tiles.end(),
              [](Tile& l, Tile& r) { return l.faceValue <= r.faceValue && l.color <= r.color; });
    return;
  }
  if (tiles.size() == 3 && numJokers == 2) {
    return;
  }
  if (numJokers > 0) {
    // Distribute as many jokers as possible after the tile with smallest face value
    const int8_t first = bits._Find_first();
    for (int8_t i = first + 1; i < 13 && numJokers; ++i) {
      if (!bits.test(i)) { // put joker here
        if (joker1Index != -1) {
          tiles[joker1Index].faceValue = i + 1;
          tiles[joker1Index].color = color;
          joker1Index = -1;
        } else if (joker2Index != -1) {
          tiles[joker2Index].faceValue = i + 1;
          tiles[joker2Index].color = color;
          joker2Index = -1;
        }
        numJokers -= 1;
      }
    }
    // Place any remaining jokers before all other tiles
    for (int8_t i = first - 1; i >= 1 && numJokers; --i) {
      if (!bits.test(i)) { // put joker here
        if (joker1Index != -1) {
          tiles[joker1Index].faceValue = i + 1;
          tiles[joker1Index].color = color;
          joker1Index = -1;
        } else if (joker2Index != -1) {
          tiles[joker2Index].faceValue = i + 1;
          tiles[joker2Index].color = color;
          joker2Index = -1;
        }
        numJokers -= 1;
      }
    }
  }
    std::sort(tiles.begin(), tiles.end(),
              [](Tile& l, Tile& r) { return l.faceValue <= r.faceValue && l.color <= r.color; });

  // std::vector<Tile> jokers;
  // for (auto& t : tiles) { // get all jokers
  //   if (t.isJoker)
  //     jokers.push_back(t);
  // }
  // for (int i = 0; i < jokers.size(); ++i) { // remove them from the vector
  //   for (auto it = tiles.begin(); it != tiles.end(); ++it) {
  //     if (it->isJoker) {
  //       tiles.erase(it);
  //       break;
  //     };
  //   }
  // }
  // int numJokers = jokers.size();
  // for (int j = 0; j < jokers.size(); ++j) {
  //   for (int i = 0; i < tiles.size() - 1; ++i) {
  //     if (tiles[i].faceValue + 1 != tiles[i + 1].faceValue) {
  //       Tile joker = jokers[j];
  //       joker.color = tiles[i].color;
  //       joker.faceValue = tiles[i].faceValue + 1;
  //       numJokers -=1;
  //       tiles.insert(tiles.begin() + i + 1, joker);
  //       break;
  //     }
  //   }
  // }
  // if (numJokers > 0) {
  //   for (int i = 0; i < numJokers; ++i) {
  //   }
  // }

  // if (tiles.size() <= 1) {
  //   return;
  // }
  // int numJokers = 0;
  // for (auto& t : tiles) {
  //   numJokers += int(t.isJoker);
  // }
  // if (numJokers == 2 && tiles.size() <= 3) {
  //   return;
  // }
  // std::sort(tiles.begin(), tiles.end(),
  //           [](Tile l, Tile r) { return l.faceValue <= r.faceValue && !l.isJoker; });
  // if (numJokers == 0) {
  //   return;
  // }
  // // Sort then if there are holes, fill them with jokers
  // const int numNonJokers = tiles.size() - numJokers;
  // for (int i = 0, j = 0; i < numNonJokers - 1; ++i) {
  //   if (tiles[i].faceValue - tiles[i + 1].faceValue == 2 && numJokers >= 1) {
  //     tiles[numNonJokers + j].faceValue = tiles[i].faceValue + 1;
  //     tiles[numNonJokers + j].color = tiles[i].color;
  //     j += 1;
  //   }
  //   else if (tiles[i].faceValue - tiles[i + 1].faceValue == 3 && numJokers == 2) {
  //     tiles[numNonJokers + j].faceValue = tiles[i].faceValue + 1;
  //     tiles[numNonJokers + j].color = tiles[i].color;
  //     j += 1;
  //     tiles[numNonJokers + j].faceValue = tiles[i].faceValue + 2;
  //     tiles[numNonJokers + j].color = tiles[i].color;
  //     j += 1;
  //   }
  // }
  // std::sort(tiles.begin(), tiles.end(), [](Tile l, Tile r) { return l.faceValue <= r.faceValue;
  // });
}
