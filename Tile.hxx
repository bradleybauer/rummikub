#pragma once

struct Tile {
  int faceValue = 0;
  int color = 0;
  bool isJoker = false;

  Tile() = default;

  Tile(int faceValue, int color, bool isJoker = false)
      : faceValue(faceValue), color(color), isJoker(isJoker) {}
};