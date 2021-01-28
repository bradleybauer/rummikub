#include "Search.hxx"

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <utility>
#include <vector>
using std::array;
using std::cout;
using std::endl;
using std::fill;
using std::get;
using std::map;
using std::max;
using std::min;
using std::pair;
using std::swap;
using std::tie;
using std::tuple;
using std::vector;

// These must not change
static const int N = 13;
static const int K = 4;
static const int M = 2;
static const int EMPTY = -9999999; // denotes that an entry in a table has not been computed yet
static const int INVALID = -1000000; // denotes an invalid configuration
static const int JOKER_VALUE = 25;

static const int MAX_NUM_JOKERS = 2;
static const int NUM_WAYS_TO_CHOOSE_JOKERS = MAX_NUM_JOKERS * (MAX_NUM_JOKERS + 1) / 2;
static const int MAX_NUM_GROUPS = (M * K + MAX_NUM_JOKERS) / 3;

using RunsT = array<array<int, 2>, K>; // type for recording which runs can be extended

using MemoKeyT = tuple<int,   // value
                       RunsT, // runs
                       int>;  // number of jokers used

using MemoValT = tuple<RunsT,                       // next runs
                       int,                         // next number of jokers used
                       array<int, MAX_NUM_JOKERS>,  // joker color assignments
                       array<int, MAX_NUM_GROUPS>>; // groups

// return value type of the makeRuns function
using MakeRunsReturnT =
    vector<tuple<RunsT,           // run extensions
                 int,             // run scores
                 array<int, K>,   // sliding window update info
                 array<int, K>>>; // number of tiles played this turn in runs per suit

// a PathT is used in converting the memo table into a new board configuration
using PathT = vector<tuple<int,                          // value
                           RunsT,                        // run
                           array<int, MAX_NUM_JOKERS>,   // joker color assignment
                           array<int, MAX_NUM_GROUPS>>>; // group representation

// f(m) computes 4 choose m, with replacement
constexpr int f(const int m) { return (m + 1) * (m + 2) * (m + 3) / 6; }

// convert an element of RunsT into an index (runs 2 index)
int r2i(const auto& run) {
  auto [i, j] = run;
  j -= i;
  i = 4 - i;
  return (f(M) - i * (i + 1) / 2) + j; // f(M) == 10
}

// remove the tiles played into runs from the hand
void removeFromHand(const auto& runs, auto& hand, const int value) {
  for (int color = -1; auto [a, b] : runs) {
    color += 1;
    if (a != 0) {
      hand[color][value - 1] -= 1;
    }
    if (b != 0) {
      hand[color][value - 1] -= 1;
    }
  }
}

// add the tiles played into runs back into the hand
void addToHand(const auto& runs, auto& hand, const int value) {
  for (int color = -1; auto [a, b] : runs) {
    color += 1;
    if (a != 0) {
      hand[color][value - 1] += 1;
    }
    if (b != 0) {
      hand[color][value - 1] += 1;
    }
  }
}

// scratch work area for makeRuns
static array<array<array<int, 2>, K>, K> extensions;
static array<array<int, K>, K> runscores;
static array<array<int, K>, K> slidingWindowUpdates;
static array<array<int, K>, K> numInRunsBySuits;
// end scratch work area
// Adds the extension and associated information to the list of possible extensions for the color k
inline void addExtension(const array<int, 2>& extension, const int score,
                         const int slidingWindowUpdate, const int numInRun, auto& counts,
                         const int k) {
  extensions[k][counts[k]] = extension;
  runscores[k][counts[k]] = score;
  slidingWindowUpdates[k][counts[k]] = slidingWindowUpdate;
  numInRunsBySuits[k][counts[k]] = numInRun;
  counts[k] += 1;
}
// compute the score we get for adding a tile of value = value to a run of length a
inline int getScoreForExtension(int a, int value) {
  if (a == 2) {
    // 3 * (value - 1) == (value - 2) + (value - 1) + value
    return 3 * (value - 1);
  } else if (a == 3) {
    return value;
  }
  return 0;
}

// returns all ways that we can play tiles of the current value into runs
MakeRunsReturnT makeRuns(const int value, const auto& runs, const auto& slidingWindow,
                         const auto& tiles, const auto& table) {
  array<int, K> counts{};

  for (int k = -1; auto [a, b] : runs) {
    k += 1;

    // whether we can end a run(w / o violating the table constraint) is based on run length and the sliding window
    bool canEndRunB = true;
    if (b == 1)
      canEndRunB = slidingWindow[k][0] - 1 >= table[k][value - 2];
    else if (b == 2)
      canEndRunB = slidingWindow[k][0] - 1 >= table[k][value - 2] &&
                   slidingWindow[k][1] - 1 >= table[k][value - 3];

    bool canEndRunA = true;
    if (a == 1)
      canEndRunA = slidingWindow[k][0] - 1 >= table[k][value - 2];
    else if (a == 2)
      canEndRunA = slidingWindow[k][0] - 1 >= table[k][value - 2] &&
                   slidingWindow[k][1] - 1 >= table[k][value - 3];

    bool canEndBothRuns = true;
    if (a == 0 || a == 3)
      canEndBothRuns = canEndRunB;
    else if (b == 0 || b == 3)
      canEndBothRuns = canEndRunA;
    else if (a == 1 && b == 1)
      canEndBothRuns = slidingWindow[k][0] - 2 >= table[k][value - 2];
    else if (a == 2 && b == 2)
      canEndBothRuns = slidingWindow[k][0] - 2 >= table[k][value - 2] &&
                       slidingWindow[k][1] - 2 >= table[k][value - 3];
    else
      canEndBothRuns = slidingWindow[k][0] - 2 >= table[k][value - 2] &&
                       slidingWindow[k][1] - 1 >= table[k][value - 3];

    // Do not start runs if we do not have enough time (or enough tiles) to finish them.
    // NOTE there is an interaction with jokers and implicitly ending runs when value==13?
    const bool canStartRunA =
        a != 0 || (value < 12 && tiles[k][value] >= 1 && tiles[k][value + 1] >= 1);
    const bool canStartRunB =
        b != 0 || (value < 12 && tiles[k][value] >= 1 && tiles[k][value + 1] >= 1);
    const bool canStartBothRuns =
        canStartRunA && canStartRunB &&
        (!(a == 0 && b == 0) || (value < 12 && tiles[k][value] >= 2 && tiles[k][value + 1] >= 2));

    // Do not start run A and end run B if run B is already started and run A is empty.
    // In this case just extend run B. This makes my tests a bit faster.
    const bool endBToStartOnlyAIsUseless = a == 0 && b != 0;
    const bool endAToStartOnlyBIsUseless = b == 0 && a != 0;

    // extend neither run
    if (canEndBothRuns) {
      addExtension({0, 0}, 0, slidingWindow[k][0] - int(a == 1 || a == 2) - int(b == 1 || b == 2), 0, counts, k);
    }

    // extend only one run
    if (tiles[k][value - 1] >= 1) {
      // if a == 0 then we should check canStartRunA (not + or is how you get the truth value of an
      // implication/if)
      if (canEndRunB && canStartRunA && !endBToStartOnlyAIsUseless) {
        const int score = getScoreForExtension(a, value);
        addExtension({0, min(3, a + 1)}, score, slidingWindow[k][0] - int(b == 1 || b == 2), 1, counts, k);
      }

      // extend right run if a != b
      if (a != b && canEndRunA && canStartRunB && !endAToStartOnlyBIsUseless) {
        const int score = getScoreForExtension(b, value);
        addExtension({0, min(3, b + 1)}, score, slidingWindow[k][0] - int(a == 1 || a == 2), 1, counts, k);
      }
    }

    // extend both runs
    if (tiles[k][value - 1] >= 2 && canStartBothRuns) {
      const int score = getScoreForExtension(a, value) + getScoreForExtension(b, value);
      addExtension({min(3, a + 1), min(3, b + 1)}, score, slidingWindow[k][0], 2, counts, k);
    }
  }

  // compute cartesian product and zip
  MakeRunsReturnT result;
  result.reserve(counts[0] * counts[1] * counts[2] * counts[3]);
  for (int i = 0; i < counts[0]; ++i) {
    for (int j = 0; j < counts[1]; ++j) {
      for (int k = 0; k < counts[2]; ++k) {
        for (int l = 0; l < counts[3]; ++l) {
          // run extension
          RunsT runs{};
          runs[0] = extensions[0][i];
          runs[1] = extensions[1][j];
          runs[2] = extensions[2][k];
          runs[3] = extensions[3][l];

          // run score
          const int score = runscores[0][i] + runscores[1][j] + runscores[2][k] + runscores[3][l];

          // sliding window update
          array<int, K> slidingWindowUpdate{};
          slidingWindowUpdate[0] = slidingWindowUpdates[0][i];
          slidingWindowUpdate[1] = slidingWindowUpdates[1][j];
          slidingWindowUpdate[2] = slidingWindowUpdates[2][k];
          slidingWindowUpdate[3] = slidingWindowUpdates[3][l];

          // num in runs by suit
          array<int, K> numInRunsBySuit{};
          numInRunsBySuit[0] = numInRunsBySuits[0][i];
          numInRunsBySuit[1] = numInRunsBySuits[1][j];
          numInRunsBySuit[2] = numInRunsBySuits[2][k];
          numInRunsBySuit[3] = numInRunsBySuits[3][l];

          result.push_back({runs, score, slidingWindowUpdate, numInRunsBySuit});
        }
      }
    }
  }
  return result;
}

// returns number of tiles in groups, number of tiles in groups by suit, and group representations
array<int, 1 + MAX_NUM_GROUPS> _totalGroupSize(const array<int, 4>& tileCounts,
                                               auto& totalGroupSizeTable, int prev_group = -1) {
  // Note every permutation of colors has essentially the same return value.
  // So there are only 15 (n choose 4, with replacement, order doesnt matter) essentially different
  // inputs to this function. But, trying to take advantage of that fact makes my tests slower.

  array<int, 1 + MAX_NUM_GROUPS>& answer =
      totalGroupSizeTable[tileCounts[0]][tileCounts[1]][tileCounts[2]][tileCounts[3]];
  if (answer[0] >= 0) {
    return answer;
  }
  answer[0] = 0;

  // Iterate over all possible groups of size 3 and 4.
  // l is the color that is not included in the group, and l == -1 denotes the group of 4.
  for (int l = prev_group; l < K; ++l) {
    auto newTileCounts = tileCounts;
    int k = 0;
    for (k = 0; k < K; ++k) {
      if (k == l) {
        continue;
      }
      if (newTileCounts[k] == 0) {
        break;
      }
      newTileCounts[k] -= 1;
    }
    if (k != K) { // If not every color in this choice has an available tile
      continue;
    }

    auto choice = _totalGroupSize(newTileCounts, totalGroupSizeTable, l);
    choice[0] += l >= 0 ? 3 : 4;
    choice[3] = choice[2];
    choice[2] = choice[1];
    choice[1] = l;
    answer = max(answer, choice);
  }

  return answer;
}

tuple<int,                        // total number of tiles in groups
      array<int, K>,              // number of tiles in groups by suit
      array<int, MAX_NUM_GROUPS>> // a representation for each group
totalGroupSize(const auto& tiles, const int value, auto& totalGroupSizeTable) {
  array<int, K> tileCounts;
  for (int k = 0; k < K; ++k) {
    tileCounts[k] = tiles[k][value - 1];

    // It is not possible to use 4 tiles of the same color in forming groups.
    // So discard a tile of color k if there is 4 of them.
    // If discarding is a violation of the table constraint, then we catch that later.
    tileCounts[k] = min(3, tileCounts[k]);
  }

  const auto result = _totalGroupSize(tileCounts, totalGroupSizeTable);
  const int totalNumInGroups = result[0];
  const array<int, MAX_NUM_GROUPS> groups{result[1], result[2], result[3]};

  array<int, K> numInGroupsBySuit{};
  for (const int l : groups) {
    if (l == EMPTY) {
      break;
    }
    for (int i = 0; i < K; ++i) {
      if (l == i) {
        continue;
      }
      numInGroupsBySuit[i] += 1;
    }
  }

  return {totalNumInGroups, numInGroupsBySuit, groups};
}

bool tableConstraint(const auto& table, const auto& numInRunsBySuit, const auto& numInGroupsBySuit,
                     const int value) {
  for (int k = 0; k < K; ++k) {
    if (numInRunsBySuit[k] + numInGroupsBySuit[k] < table[k][value - 1]) {
      return false;
    }
  }
  return true;
}

inline MemoValT& memoRef(auto& memo, const int value, const auto& runs, const int numJokersUsed) {
  const auto index = MemoKeyT{value, runs, numJokersUsed};
  return memo[index];
}
inline int& scoreRef(auto& score, const int value, const auto& runs, const int numJokersUsed) {
  const int index =
      ((r2i(runs[0]) * f(M) + r2i(runs[1])) * f(M) + r2i(runs[2])) * f(M) + r2i(runs[3]);
  return score[value - 1][index][numJokersUsed];
}
int _maxScore(const int value,                //
              const auto& runs,               //
              const int numJokersUsed,        //
              const auto& slidingWindow,      //
              auto& tiles,                    //
              auto& score,                    //
              auto& totalGroupSizeTable,      //
              const int minNumJokersRequired, //
              const int totalNumJokers,       //
              auto& table,                    //
              auto& memo) {                   //
  if (value > 13) {
    if (numJokersUsed < minNumJokersRequired) {
      return INVALID;
    }
    return 0;
  }

  int& answer = scoreRef(score, value, runs, numJokersUsed);
  if (answer != EMPTY) {
    return answer;
  }
  answer = INVALID;

  const int numJokersAvailable = totalNumJokers - numJokersUsed;
  for (int numJokers = 0; numJokers <= numJokersAvailable; ++numJokers) {
    // choose a color assignment for the jokers
    for (int joker1 = 0; joker1 <= (K - 1) * int(numJokers >= 1); ++joker1) {
      for (int joker2 = joker1 * int(numJokers == 2); joker2 <= (K - 1) * int(numJokers == 2);
           ++joker2) {
        // if we decide to discard a tile when making runs or forming groups then we assume it is
        // not a joker. if it must be a joker (there are no other regular tiles) then the run
        // extension / group is not a valid state and discarding the tile is a violation of the
        // table constraint.
        //
        // this modification of table ensures that it would be a violation of the table constraint
        // to not use the numJokers jokers.
        if (numJokers >= 1) {
          table[joker1][value - 1] += 1;
          tiles[joker1][value - 1] += 1;
        }
        if (numJokers == 2) {
          table[joker2][value - 1] += 1;
          tiles[joker2][value - 1] += 1;
        }
        for (const auto& tupl : makeRuns(value, runs, slidingWindow, tiles, table)) {
          // unpack tupl
          RunsT newRuns;
          int runScores;
          array<int, K> updatedSlidingWindow;
          array<int, K> numInRunsBySuit;
          tie(newRuns, runScores, updatedSlidingWindow, numInRunsBySuit) = tupl;

          // play remaining tiles into groups
          removeFromHand(newRuns, tiles, value);
          int totalNumInGroups;
          array<int, K> numInGroupsBySuit;
          array<int, MAX_NUM_GROUPS> groups;
          tie(totalNumInGroups, numInGroupsBySuit, groups) =
              totalGroupSize(tiles, value, totalGroupSizeTable);
          addToHand(newRuns, tiles, value);

          // Check that the number of tiles (of current value) in the chosen run extension and
          // groups is enough
          if (!tableConstraint(table, numInRunsBySuit, numInGroupsBySuit, value)) {
            continue;
          }

          RunsT newSlidingWindow{};
          for (int k = 0; k < K; ++k) {
            newSlidingWindow[k] = {numInRunsBySuit[k] + numInGroupsBySuit[k],
                                   updatedSlidingWindow[k]};
          }

          // Because we do not discard jokers, we can add the value for them right now
          const int jokerScores = -value * numJokers + numJokers * JOKER_VALUE;
          const int groupScores = totalNumInGroups * value;

          const int newValue = value + 1;
          const int newNumJokersUsed = numJokersUsed + numJokers;
          const int result =
              groupScores + runScores + jokerScores +
              _maxScore(newValue, newRuns, newNumJokersUsed, newSlidingWindow, tiles, score,
                        totalGroupSizeTable, minNumJokersRequired, totalNumJokers, table, memo);

          // Memoize
          if (result > answer) {
            answer = result;
            array<int, MAX_NUM_JOKERS> jokerColorAssignemnts{EMPTY, EMPTY};
            if (numJokers >= 1) {
              jokerColorAssignemnts[0] = joker1;
            }
            if (numJokers == 2) {
              jokerColorAssignemnts[1] = joker2;
            }
            memoRef(memo, value, runs, numJokersUsed) = {newRuns, newNumJokersUsed,
                                                         jokerColorAssignemnts, groups};
          }
        }
        if (numJokers >= 1) {
          table[joker1][value - 1] -= 1;
          tiles[joker1][value - 1] -= 1;
        }
        if (numJokers == 2) {
          table[joker2][value - 1] -= 1;
          tiles[joker2][value - 1] -= 1;
        }
      }
    }
  }

  // The current state is invalid if we are at a node where makeRuns returns an empty list.
  // So the return value should be s.t. result can not contribute to the max.
  if (answer < 0) {
    answer = INVALID;
  }

  return answer;
}

// When solving for the max score configuration, we do not care about the order of the numbers in the inner tuples of RunsT.
// But, order matters when we try to decipher what runs were played in the configuration.
// This function makes sure that everything lines up correctly. lol.
bool eq(const array<int, 2>& l, const array<int, 2>& r) { return l == r; }
array<int, 2> getAlignedRun(auto run, auto nextRun) {
  // check easy case
  if (run[0] == run[1] || nextRun[0] == nextRun[1]) {
    return nextRun;
  }

  // make sure run is ascending (reduces size of if statement below)
  const bool reverse = run[0] > run[1];
  if (reverse) {
    swap(run[0], run[1]);
    swap(nextRun[0], nextRun[1]);
  }

  // do transformation
  if (eq(run, {0, 1})) {
    if (eq(nextRun, {2, 1}) || eq(nextRun, {2, 0})) {
      swap(nextRun[0], nextRun[1]);
    }
  } else if (eq(run, {0, 2}) || eq(run, {0, 3})) {
    if (eq(nextRun, {3, 1}) || eq(nextRun, {3, 0})) {
      swap(nextRun[0], nextRun[1]);
    }
  } else if (eq(run, {1, 2})) {
    if (eq(nextRun, {3, 2})) {
      swap(nextRun[0], nextRun[1]);
    }
  } else if (eq(run, {1, 3})) {
    if (eq(nextRun, {3, 2}) || eq(nextRun, {0, 2})) {
      swap(nextRun[0], nextRun[1]);
    }
  } else if (eq(run, {2, 3})) {
    if (eq(nextRun, {0, 3})) {
      swap(nextRun[0], nextRun[1]);
    }
  }

  // if we reversed the input then undo that
  if (reverse) {
    swap(nextRun[0], nextRun[1]);
  }
  return nextRun;
}

// The memo sometimes lets us record a partial run.
// So this funciton removes any partial runs.
void removeInvalidRun(auto& path) {
  for (int k = 0; k < 4; ++k) {
    for (int i = 0; i < 2; ++i) {
      int prev = 0;
      int numInRun = 0;
      for (int n = 1; n <= 13; ++n) {
        array<int, 2>& run = get<1>(path[n - 1])[k];
        if (run[i] == 1) {
          numInRun += 1;
        } else if (prev == 1 && run[i] == 0) {
          if (numInRun == 1 || numInRun == 2) {
            for (int rev_n = n - 1; rev_n >= n - numInRun; --rev_n) {
              get<1>(path[rev_n - 1])[k][i] = 0;
            }
          }
          numInRun = 0;
        }

        prev = run[i];
      }
      if (prev == 1 && (numInRun == 1 || numInRun == 2)) {
        for (int rev_n = 13; rev_n >= 13 - numInRun; --rev_n) {
          get<1>(path[rev_n - 1])[k][i] = 0;
        }
      }
      numInRun = 0;
    }
  }
}

tuple<int, PathT> maxScore(auto& table, auto& hand, const int numJokersOnTable,
                           const int numJokersInHand) {
  const int value = 1;
  RunsT runs{};
  RunsT slidingWindow{};
  constexpr int width = f(M) * f(M) * f(M) * f(M);
  array<array<array<int, NUM_WAYS_TO_CHOOSE_JOKERS>, width>, N> score;
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < width; ++j) {
      for (int k = 0; k < NUM_WAYS_TO_CHOOSE_JOKERS; ++k) {
        score[i][j][k] = EMPTY;
      }
    }
  }
  decltype(hand) tiles;
  for (int i = 0; i < K; ++i) {
    for (int j = 0; j < N; ++j) {
      tiles[i][j] = table[i][j] + hand[i][j];
    }
  }
  array<array<array<array<array<int, 1 + MAX_NUM_GROUPS>, K>, K>, K>, K> totalGroupSizeTable;
  for (int i = 0; i < K; ++i) {
    for (int j = 0; j < K; ++j) {
      for (int k = 0; k < K; ++k) {
        for (int l = 0; l < K; ++l) {
          for (int m = 0; m < 1 + MAX_NUM_GROUPS; ++m) {
            totalGroupSizeTable[i][j][k][l][m] = EMPTY;
          }
        }
      }
    }
  }
  const int numJokersUsed = 0;
  const int minNumJokersRequired = numJokersOnTable;
  const int totalNumJokers = numJokersOnTable + numJokersInHand;
  map<MemoKeyT, MemoValT> memo;

  const int result =
      _maxScore(value, runs, numJokersUsed, slidingWindow, tiles, score, totalGroupSizeTable,
                minNumJokersRequired, totalNumJokers, table, memo);

  if (result <= 0) {
    return {0, {}};
  }

  runs = {};
  MemoKeyT parent = {1, runs, 0};
  PathT path;
  while (memo.count(parent) > 0) {
    RunsT nextRuns;
    int nextNumJokersUsed;
    array<int, MAX_NUM_JOKERS> jokerColorAssignemnts;
    array<int, MAX_NUM_GROUPS> groups;
    tie(nextRuns, nextNumJokersUsed, jokerColorAssignemnts, groups) = memo[parent];
    RunsT runDiff{};
    for (int k = 0; k < K; ++k) {
      nextRuns[k] = getAlignedRun(runs[k], nextRuns[k]);
      runDiff[k][0] = int(nextRuns[k][0] == 3 || nextRuns[k][0] > runs[k][0]);
      runDiff[k][1] = int(nextRuns[k][1] == 3 || nextRuns[k][1] > runs[k][1]);
    }
    const int value = get<0>(parent);
    path.push_back({value, runDiff, jokerColorAssignemnts, groups});

    // We may have permuted nextRuns in the above loop.
    // But memo is memoized on a sorted version of nextRuns.
    runs = nextRuns;
    RunsT _nextRuns;
    for (int k = 0; k < K; ++k) {
      _nextRuns[k] = {min(nextRuns[k][0], nextRuns[k][1]), max(nextRuns[k][0], nextRuns[k][1])};
    }
    parent = {value + 1, _nextRuns, nextNumJokersUsed};
  }

  // This may be neccessary for some inputs?
  if (path.size() > 0) {
    removeInvalidRun(path);
  }

  // cout << result << endl;
  // for (const auto& tup : path) {
  //   int value;
  //   RunsT runDiff;
  //   array<int, MAX_NUM_JOKERS> jCA;
  //   array<int, MAX_NUM_GROUPS> groups;
  //   tie(value, runDiff, jCA, groups) = tup;
  //   cout << value << '\t';
  //   for (int k = 0; k < K; ++k) {
  //     if (runDiff[k][0] != 0) {
  //       cout << '(' << runDiff[k][0] << ' ';
  //     } else {
  //       cout << "(  ";
  //     }
  //     if (runDiff[k][1] != 0) {
  //       cout << runDiff[k][1] << ") ";
  //     } else {
  //       cout << " ) ";
  //     }
  //   }
  //   cout << "J ";
  //   if (jCA[0] != EMPTY) {
  //     cout << jCA[0] << " ";
  //   } else {
  //     cout << "  ";
  //   }
  //   if (jCA[1] != EMPTY) {
  //     cout << jCA[1] << " ";
  //   } else {
  //     cout << "  ";
  //   }
  //   cout << "G ";
  //   for (int i = 0; i < MAX_NUM_GROUPS; ++i) {
  //     if (groups[i] == EMPTY) {
  //       break;
  //     }
  //     cout << groups[i] << " ";
  //   }
  //   cout << endl;
  // }
  // if (path.size() < 2)
  // exit(0);

  // for (const auto& tup : path) {
  //     int value;
  //     RunsT runDiff;
  //     array<int, MAX_NUM_JOKERS> jCA;
  //     array<int, MAX_NUM_GROUPS> groups;
  //     tie(value, runDiff, jCA, groups) = tup;
  //     cout << value << '\t';
  //     for (int k = 0; k < K; ++k) {
  //         if (runDiff[k][0] != 0) {
  //             cout << '(' << runDiff[k][0] << ' ';
  //         } else {
  //             cout << "(  ";
  //         }
  //         if (runDiff[k][1] != 0) {
  //             cout << runDiff[k][1] << ") ";
  //         } else {
  //             cout << " ) ";
  //         }
  //     }
  //     cout << "J ";
  //     if (jCA[0] != EMPTY) {
  //         cout << jCA[0] << " ";
  //     } else {
  //         cout << "  ";
  //     }
  //     if (jCA[1] != EMPTY) {
  //         cout << jCA[1] << " ";
  //     } else {
  //         cout << "  ";
  //     }
  //     cout << "G ";
  //     for (int i = 0; i < MAX_NUM_GROUPS; ++i) {
  //         if (groups[i] == EMPTY) {
  //             break;
  //         }
  //         cout << groups[i] << " ";
  //     }
  //     cout << endl;
  // }

  return {result, path};
}

// This function assumes that the input path has only valid runs and groups in it.
pair<vector<TileSet>, vector<Tile>>
getTileSetsFromMemo(auto& table, auto& path, int numJokersOnTable, int numJokersInHand) {
  vector<TileSet> tileSets;
  vector<Tile> handSubset;
  // collect runs
  for (int k = 0; k < 4; ++k) {
    for (int i = 0; i < 2; ++i) {
      TileSet s;
      int prev = 0;
      for (int n = 1; n <= 13; ++n) {
        bool used = false;
        bool usedJoker = false;
        array<int, 2>& run = get<1>(path[n - 1])[k];
        if (run[i] == 1) {
          array<int, MAX_NUM_JOKERS>& jokers = get<2>(path[n - 1]);
          if (jokers[0] != EMPTY && jokers[0] == k) {
            s.tiles.push_back(Tile{n, k, true});
            jokers[0] = EMPTY;
            usedJoker = true;
          } else if (jokers[1] != EMPTY && jokers[1] == k) {
            s.tiles.push_back(Tile{n, k, true});
            jokers[1] = EMPTY;
            usedJoker = true;
          } else {
            s.tiles.push_back(Tile{n, k, false});
          }
          used = true;
        } else if (prev == 1 && run[i] == 0) {
          tileSets.push_back(s);
          s.tiles.clear();
        }
        prev = run[i];
        if (used) {
          if (usedJoker && numJokersOnTable > 0) {
            numJokersOnTable -= 1;
          } else if (usedJoker) {
            handSubset.push_back(Tile{n, k, usedJoker});
          } else if (table[k][n - 1] > 0) {
            table[k][n - 1] -= 1;
          } else {
            handSubset.push_back(Tile{n, k, usedJoker});
          }
        }
      }
      if (prev == 1) {
        tileSets.push_back(s);
        s.tiles.clear();
      }
    }
  }
  // collect groups
  for (int n = 1; n <= 13; ++n) {
    array<int, MAX_NUM_JOKERS>& jokers = get<2>(path[n - 1]);
    array<int, MAX_NUM_GROUPS>& groups = get<3>(path[n - 1]);
    for (auto l : groups) {
      if (l == EMPTY) {
        break;
      }
      TileSet s;
      for (int k = 0; k < 4; ++k) {
        if (k == l) {
          continue;
        }
        bool usedJoker = false;
        if (jokers[0] != EMPTY && jokers[0] == k) {
          s.tiles.push_back(Tile{n, k, true});
          jokers[0] = EMPTY;
          usedJoker = true;
        } else if (jokers[1] != EMPTY && jokers[1] == k) {
          s.tiles.push_back(Tile{n, k, true});
          jokers[1] = EMPTY;
          usedJoker = true;
        } else {
          s.tiles.push_back(Tile{n, k, false});
        }
        if (usedJoker && numJokersOnTable > 0) {
          numJokersOnTable -= 1;
        } else if (usedJoker) {
          handSubset.push_back(Tile{n, k, usedJoker});
        } else if (table[k][n - 1] > 0) {
          table[k][n - 1] -= 1;
        } else {
          handSubset.push_back(Tile{n, k, usedJoker});
        }
      }
      tileSets.push_back(s);
    }
  }
  return {tileSets, handSubset};
}

// Used for testing. Gets the tilesets if the input tiles can be arranged into a valid configuration
vector<TileSet> getTileSetsIfValid(vector<Tile> tiles) {
  int numJokersOnTable = 0;
  array<array<int, N>, K> table{};
  for (auto tile : tiles) {
    if (tile.isJoker) {
      numJokersOnTable += 1;
    } else {
      table[tile.color][tile.faceValue - 1] += 1;
    }
  }
  array<array<int, N>, K> hand{};
  int maxscore;
  PathT path;
  tie(maxscore, path) = maxScore(table, hand, numJokersOnTable, 0);
  if (maxscore <= 0) {
    return {};
  }
  if (maxscore > 0) {
    vector<TileSet> tileSets;
    vector<Tile> handSubset;
    tie(tileSets, handSubset) = getTileSetsFromMemo(table, path, numJokersOnTable, 0);
    return tileSets;
  }
  return {};
}

tuple<array<array<int, N>, K>, array<array<int, N>, K>, int, int>
getArraysFromTileSets(vector<TileSet>& board, vector<Tile>& rack) {
  int numJokersOnTable = 0;
  int numJokersInHand = 0;

  // rack
  array<array<int, N>, K> hand{};
  for (auto& tile : rack) {
    if (tile.isJoker) {
      numJokersInHand += 1;
    } else {
      hand[tile.color][tile.faceValue - 1] += 1;
    }
  }
  // table
  array<array<int, N>, K> table{};
  for (auto& s : board) {
    for (auto tile : s.tiles) {
      if (tile.isJoker) {
        numJokersOnTable += 1;
      } else {
        table[tile.color][tile.faceValue - 1] += 1;
      }
    }
  }

  return {table, hand, numJokersOnTable, numJokersInHand};
}

// Find maximum value play from rack
pair<vector<TileSet>, vector<Tile>> solve(vector<TileSet>& board, vector<Tile>& rack) {
  array<array<int, N>, K> table;
  array<array<int, N>, K> hand;
  int numJokersOnTable;
  int numJokersInHand;
  tie(table, hand, numJokersOnTable, numJokersInHand) = getArraysFromTileSets(board, rack);

  // get tiles in best play
  int maxscore;
  PathT path;
  tie(maxscore, path) = maxScore(table, hand, numJokersOnTable, numJokersInHand);

  if (maxscore <= 0) {
    return {{}, {}};
  }

  vector<TileSet> tileSets;
  vector<Tile> handSubset;
  tie(tileSets, handSubset) =
      getTileSetsFromMemo(table, path, numJokersOnTable, numJokersInHand);

  // cout << "T:";
  // for (auto s : tileSets) {
  //   for (auto t : s.tiles) {
  //     if (t.isJoker)
  //       cout << "<" << t.color << "," << t.faceValue << "J>,";
  //     else
  //       cout << "<" << t.color << "," << t.faceValue << ">,";
  //   }
  // }
  // cout << endl;
  // cout << "H:";
  // for (auto t : handSubset) {
  //   if (t.isJoker)
  //     cout << "<" << t.color << "," << t.faceValue << "J>,";
  //   else
  //     cout << "<" << t.color << "," << t.faceValue << ">,";
  // }
  // cout << endl;

  return {tileSets, handSubset};
}
