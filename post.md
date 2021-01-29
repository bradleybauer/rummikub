I will describe a polynomial time algorithm that solves both problem 1 and problem 2. I learned of the algorithm from this [paper](https://arxiv.org/abs/1604.07553).

I am going to make the following assumptions:
1. No jokers (adding them is a small change).
2. There are only four colors.
3. There are at most two copies of each tile.
4. The face values range from 1 to 13.

To find the maximum value play, complete these steps:
1. Recursively enumerate every valid configuration that can be built from tiles in $hand \cup board$
2. Ignore any configuration where not all tiles from board are used (the **board constraint**).
3. Of the visited configurations, choose one with maximal score. Where score is the sum of values of tiles played or number of tiles played (whichever you prefer to maximize).

Step 1
====
How do we build a configuration from a given set of tiles?
We proceed by playing tiles in order of their face value.
First play all tiles of value 1.
Then play all tiles of value 2, and so on.
We can visit every configuration by recursing for each way to play tiles of the current value.

Let's look at the different ways to form runs.
Assume we are playing tiles of value 5, that we have a red 5, and that we will use it in a run.
Our tile can be used to start a new run or it can be used to extend any existing run (or partial run) that contains a red 4.
If we extend an existing run then there is at most two choices for where to put it since there is at most two runs that contain a red 4 (there are at most two red 4's).
If we have two copies of our tile then we can extend or start two runs.

In forming runs there is an optimization that can be made.
We should not start a new run if we know in advance that there is no way we could finish the run.
This can be implemented by counting the number of tiles of greater value.

Now on to forming groups.
For a given way to play runs we should recurse for every way to play groups using the remaining tiles since we want to enumerate all possible valid configurations.
However this would be very slow and in the end we really only care about finding the best configuration.
So instead we recurse for only the best way to play groups.
Notice, for a given choice of how to form runs, how we form groups does not effect what choices we have in forming groups and runs using tiles of greater value.
So given a choice of how to form runs, we should use as many of the remaining tiles in groups as possible.
To do this enumerate all ways to pick groups and choose the one containing the most tiles.

Finally after choosing how to play all tiles, discard any tile not used in a valid set.

Step 2
======
How do we ensure every valid configuration we visit uses all tiles from board?
After choosing how to play all tiles and discarding invalid sets, do not accept the configuration as valid if not all tiles from board are contained in it.
This is correct but it would be inefficient.
Instead we check at each step, after choosing how to form runs and groups, whether we have used enough tiles of the current value.
For example, if the current value is 5 and we have chosen a way to play the red 5's, then we need to check if we have played at least as many red 5's as are contained in board.
If we have not, then we choose another way to play red 5's before moving to the next value.
Note if we have played $n+m$ red 5's where $n$ is the number of red 5's in board, then we have played $m$ red 5's from our hand.
If there is no way to play red 5's (including not playing red 5's) then this branch of the recursion tree does not lead to a valid configuration and we should return.

However, these checks are not sufficient to guarantee we never violate the board constraint. Why?
Because when forming runs using tiles of value 6 we can choose to not extend a run that has length one or two.
The tiles in that run would have to be discarded which could violate the board constraint due to not having enough 4's or 5's.
In this case we would not be able to catch this error because so far we only know how to check the board constraint for the current value.
The way to fix this is to keep count of how many tiles we have played of the previous two values (for each color separately).
Then when forming runs with red 6's we can check if not extending a red run (of length one or two) puts us in a state where not enough red 4's or 5's have been played.
Note that if a run has length at least three then not extending it does not force us to discard any tiles and so can not violate the board constraint (so we do not have to check in this case).

We do not have to do anything special for choosing groups. In the following proof of this fact I will use 'configuration' to mean a set of groups.
Also, I will use 'maximal configuration' to mean a configuration containing the most tiles possible among all configurations where tiles come from some set $S$.
Note that if we have $n$ tiles of color $c$ in a configuration then there are at least $n$ groups in the configuration.
Also if a subset of the available tiles can be arranged into a configuration of $n$ groups then a maximal configuration contains at least $n$ groups (there are at most three groups, if using two jokers, and at most two groups of four in any configuration).
Now assume a subset $S$ of the available tiles can be arranged into a configuration with $n$ tiles of color $c$ and that we have arranged some maximal configuration $M$.
If $M$ contains $k < n$ tiles of color $c$ then $M$ contains at least $n - k$ groups which do not contain a tile of color $c$ (groups of size three).
In this case we can add $n-k$ tiles of color $c$ from $S$ to $n-k$ groups of three in $M$ which contradicts that $M$ is a maximal configuration.
Thus $k\geq n$.
This shows that if there are $n$ tiles of color $c$ in the set of tiles of current value that remain after playing runs and if those $n$ tiles can be played in some configuration then every maximal configuration contains those $n$ tiles.
So choosing a maximal group guarantees that we satisfy the board constraint if it is possible to satisfy the board constraint by playing groups.

Step 3
======
A simple way to do step 3 would be to compute the score of a configuration when you accept it as valid then update some global variable with the configuration of max score ('configuration' meaning set of runs & groups).
But this solves the same subproblem many times (compute score for sets common to more than one valid configuration).
Instead make use of the fact that we know at each step how many tiles of the current value we will play.
Multiply this number of tiles by the current value to find how much these choices contribute to the score.
Sum the contribution with the return value of the recursive call and then update the max if necessary.
Instead of updating a global max variable, we will update a local (available only in scope of recursive call) max variable which will hold the score of the best way to play tiles of the current value.
The recursive function should return the value of this max variable.
After attempting to play tiles of current value in all ways, if no play leads to a valid configuration (every play violates the board constraint), then return $-\infty$.
Returning $-\infty$ is useful because even if we have a high contribution (from runs & groups), adding that contribution to $-\infty$ is still $-\infty$.

Note that we cannot know the contribution of extending a length zero or length one run. This is because, when playing tiles of the next greater value, we may choose to not extend a run of length one or two and so the tiles in those runs would be discarded.
Instead we say the contribution of extending a length zero run (starting a run) or length one run is 0.
When a run is extended from length two to length three then we say it's contribution is the sum of the values of the three tiles in the run.
When extending a run that has length at least three then the contribution is just the value of the tile played into the run.

There are other things you can do here too. For example, It may be desireable to find a configuration that is maximally similar to the previous configuration of the board.
This can be done by maximizing score and then maximizing some similarity function. This would inflate the dp table size by two.

State, Memoization, & Time Complexity
=======
The only state we need to know is the current value and the length of the runs that we could possibly extend.
The length of runs can be recorded in a list of pairs, for example [(0,0), (0,0), (0,1), (0,3)], where the $i^{th}$ pair is the length of the two possible runs of color $i$.
The order of the numbers in the pairs does not matter so the above state is the same as [(0,0), (0,0), (1,0), (0,3)].
A particular extension (i.e. child state) of those runs could be [(1,1), (0,1), (0,0), (1,3)].
Also, we never need to know if a run is longer than three tiles (so an extension of (3,3) is (3,3)).
We will only need to distinguish between when a run is of length 0, 1, 2, or 3.
A benefit of this is the size of the state space is reduced which makes memoization much more effective.

This algorithm is exponential in the number of distinct values in $hand \cup board$.
However it can be made linear by memoization.

To memoize, store in a dictionary the key-value pair key=(currentTileValue, runLengthsList) and value=localMaxValue.
Other inputs like the board, $hand\cup board$, number of jokers available, number of tiles played per color on the last two turns, etc, are just used to bound the search and so we do not need to memoize them.


Hopefully I have been able to effectively communicate most of the ideas of this algorithm.
For more details see the [paper](https://arxiv.org/abs/1604.07553) and my [implementation](https://github.com/bradleybauer/rummikub/blob/master/Search.cxx).
Note, the paper claims that the dynamic programming state space (number of unique inputs / max size of dp table needed, i think) is of size $n * k * f(m)$.
Where $n$ is the number of possible tiles, $k$ is the number of colors, $m$ is the max number of copies of a tile, and $f(x)$ computes 4 choose $x$ with replacement.
This might be a typo, I do not know.
In any case the state space of the algorithm I've described is $n * f(m)^k$.

Note both the paper and my implementation refer to what you call the board as the table.

By the way, before learning of this algorithm, I tried to understand the algorithm you gave [here](https://cs.stackexchange.com/questions/85954/rummikub-algorithm/85971#85971).
I could not understand how to deal with duplicates.
I can see how breaking into sublists, ordering those sublists, and then combining them into a single list again would allow your recursive formula to handle duplicates for some inputs.
But you did not describe how exactly to break the original list into sublists nor how to combine them into a single list again.
For some inputs, your recursive formula will give different answers depending on how sublists are split/recombined.
Unfortunately, I do not have enough stackoverflow reputation to comment on your answer to ask for clarification.
Thanks for giving the answer though, your recursive formula is nice.

