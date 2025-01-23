#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define u8 uint8_t
#define i32 int32_t
#define u64 uint64_t

// Solutions are 8x8 matrices in row-major order. If the value is 1, there's a wall, if it's 0
// there isn't. It's represented by a single 64-bit unsigned integer. Each binary digit is a 1
// or a 0 which is the value for a slot in the matrix.

// Print the solution in a matrix format, helpful for debugging.
void print_grid(u64 result) {
    printf("grid: %llu\n", result);
    for (u64 i = 0; i < 64; i++) {
        printf("%llu", (result >> (63 - i)) & 1);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
}

// We refer the elements in the solution by either
// * slot, the index 0-63 of the space.
// * pos, the row (0-7) and column (0-7) of the space.

typedef struct {
    i32 row;
    i32 col;
} Pos;

// Some helpers for converting positions and slots.
// @note(steve): Slots and positions have slightly different interpretations when they're
// out-of-bounds, but they still round trip correctly, and we don't rely on out-of-bounds values
// "meaning" anything other than just being out of bounds.

static inline Pos pos_from_slot(i32 slot) {
    return (Pos){.row = slot / 8, .col = slot % 8};
}

static inline u64 slot_set(u64 m, i32 slot) {
    if (slot < 0 || slot >= 64) {
        return m;
    }
    return m | (u64)1 << (63 - slot);
}

static inline u64 slot_unset(u64 m, i32 slot) {
    if (slot < 0 || slot >= 64) {
        return m;
    }
    return m & ~((u64)1 << (63 - slot));
}

static inline u64 slot_is_set(u64 m, i32 slot) {
    if (slot < 0 || slot >= 64) {
        return 0;
    }
    return m & (u64)1 << (63 - slot);
}

static inline u64 pos_set(u64 m, Pos pos) {
    if (pos.row < 0 || pos.row >= 8 || pos.col < 0 || pos.col >= 8) {
        return m;
    }
    return m | (u64)1 << (63 - (pos.row * 8 + pos.col));
}

static inline u64 pos_unset(u64 m, Pos pos) {
    if (pos.row < 0 || pos.row >= 8 || pos.col < 0 || pos.col >= 8) {
        return m;
    }
    return m & ~((u64)1 << (63 - (pos.row * 8 + pos.col)));
}

static inline u64 pos_is_set(u64 m, Pos pos) {
    if (pos.row < 0 || pos.row >= 8 || pos.col < 0 || pos.col >= 8) {
        return 0;
    }
    return m & (u64)1 << (63 - (pos.row * 8 + pos.col));
}

i32 count_set_bits(u64 n) {
    i32 count = 0;
    while (n) {
        n &= (n - 1);
        count++;
    }
    return count;
}

// An easier to read and write representation of a puzzle.
typedef struct {
    u8 row_wall_counts[8];
    u8 col_wall_counts[8];
    Pos monsters[64];
    u8 monsters_count;
    Pos treasures[64];
    u8 treasures_count;
} PuzzleArgs;

// The representation of a puzzle used internally. Monsters and treasures
// are stored in a "solution" which makes comparing their positions to other solutions
// via bitwise operations fast and easy.
typedef struct {
    u8 row_wall_counts[8];
    u8 col_wall_counts[8];
    u64 monsters;
    u64 treasures;
} Puzzle;

Puzzle puzzle(PuzzleArgs args) {
    assert(args.monsters_count <= 64);
    assert(args.treasures_count <= 64);
    u64 monsters = 0;
    for (i32 i = 0; i < args.monsters_count; i++) {
        monsters = pos_set(monsters, args.monsters[i]);
    }
    u64 treasures = 0;
    for (i32 i = 0; i < args.treasures_count; i++) {
        treasures = pos_set(treasures, args.treasures[i]);
    }
    Puzzle p = (Puzzle){};
    for (i32 i = 0; i < 8; i++) {
        p.row_wall_counts[i] = args.row_wall_counts[i];
        p.col_wall_counts[i] = args.col_wall_counts[i];
    }
    p.monsters = monsters;
    p.treasures = treasures;
    return p;
}

void print_puzzle(Puzzle puzzle, u64 solution) {
    for (u64 i = 0; i < 64; i++) {
        u64 monster = (puzzle.monsters >> (63 - i)) & 1;
        u64 treasure = (puzzle.treasures >> (63 - i)) & 1;
        u64 wall = (solution >> (63 - i)) & 1;
        if ((monster && treasure) || (monster && wall) || (treasure && wall)) {
            printf("?");
        } else if (monster) {
            printf("M");
        } else if (treasure) {
            printf("T");
        } else if (wall) {
            printf("X");
        } else {
            printf(".");
        }
        if (i % 8 == 7) {
            printf("\n");
        }
    }
}

// Constraints.
// We only check constraints that would apply to the current slot.
// If they're written correctly and the search proceeds in slot order, then we know they've all been
// checked for previous placements.

// Check that the slot doesn't overlap a monster or treasure.
bool check_doesnt_overlap(Puzzle puzzle, u64 solution) {
    // note(steve): This actually checks the whole solution but is faster than just checking the
    // slot.
    return !(solution & puzzle.treasures || solution & puzzle.monsters);
}

i32 count_walls_in_row(u64 solution, i32 row) {
    u64 row_mask = 0b1111111100000000000000000000000000000000000000000000000000000000 >> row * 8;
    u64 row_walls = row_mask & solution;
    i32 walls_in_row = count_set_bits(row_walls);
    return walls_in_row;
}

i32 count_walls_in_col(u64 solution, i32 col) {
    u64 col_mask = 0b1000000010000000100000001000000010000000100000001000000010000000 >> col;
    u64 col_walls = col_mask & solution;
    i32 walls_in_col = count_set_bits(col_walls);
    return walls_in_col;
}

bool check_row_count(Puzzle puzzle, u64 solution, i32 slot) {
    assert(0 <= slot);
    assert(slot < 64);

    Pos pos = pos_from_slot(slot);
    i32 walls_in_row = count_walls_in_row(solution, pos.row);
    if (walls_in_row > puzzle.row_wall_counts[pos.row]) {
        return false;
    }
    // If we're at the last column, then the count must match exactly.
    if (pos.col == 7 && walls_in_row != puzzle.row_wall_counts[pos.row]) {
        return false;
    }
    return true;
}

bool check_col_count(Puzzle puzzle, u64 solution, i32 slot) {
    assert(0 <= slot);
    assert(slot < 64);

    Pos pos = pos_from_slot(slot);
    i32 walls_in_col = count_walls_in_col(solution, pos.col);
    if (walls_in_col > puzzle.col_wall_counts[pos.col]) {
        return false;
    }
    // If we're in the last row, then the count must match exactly.
    if (pos.row == 7 && walls_in_col != puzzle.col_wall_counts[pos.col]) {
        return false;
    }
    return true;
}

bool is_dead_end(Puzzle puzzle, u64 solution, Pos p) {
    if (p.row < 0 || p.row >= 8 || p.col < 0 || p.col >= 8) {
        return false;
    }
    u64 slot_mask = 0;
    slot_mask = pos_set(slot_mask, p);
    // If there's a wall here it's not a dead end.
    if (slot_mask & solution) {
        return false;
    }
    // If there's a puzzle or a monster here, it's not a dead end.
    if (slot_mask & puzzle.treasures || slot_mask & puzzle.monsters) {
        return false;
    }
    u64 border_mask = 0;
    border_mask = pos_set(border_mask, (Pos){.row = p.row - 1, .col = p.col});
    border_mask = pos_set(border_mask, (Pos){.row = p.row + 1, .col = p.col});
    border_mask = pos_set(border_mask, (Pos){.row = p.row, .col = p.col - 1});
    border_mask = pos_set(border_mask, (Pos){.row = p.row, .col = p.col + 1});
    u64 border_walls = border_mask & solution;
    // If there are 1 or less open spaces around the slot, this is a dead end.
    return count_set_bits(border_walls) >= count_set_bits(border_mask) - 1;
}

bool check_dead_ends(Puzzle puzzle, u64 solution, i32 slot) {
    // Check this slot, the slot to the left and the slot above for dead ends.
    Pos p = pos_from_slot(slot);
    Pos above = (Pos){.row = p.row - 1, .col = p.col};
    Pos left = (Pos){.row = p.row, .col = p.col - 1};
    if (is_dead_end(puzzle, solution, above) || is_dead_end(puzzle, solution, left) ||
        is_dead_end(puzzle, solution, p)) {
        return false;
    }
    return true;
}

bool is_invalid_monster(Puzzle puzzle, u64 solution, Pos p) {
    if (p.row < 0 || p.row >= 8 || p.col < 0 || p.col >= 8) {
        return false;
    }
    u64 slot_mask = 0;
    slot_mask = pos_set(slot_mask, p);
    // If there's no monster here, it's valid.
    if (!(slot_mask & puzzle.monsters)) {
        return false;
    }
    u64 border_mask = 0;
    border_mask = pos_set(border_mask, (Pos){.row = p.row - 1, .col = p.col});
    border_mask = pos_set(border_mask, (Pos){.row = p.row + 1, .col = p.col});
    border_mask = pos_set(border_mask, (Pos){.row = p.row, .col = p.col - 1});
    border_mask = pos_set(border_mask, (Pos){.row = p.row, .col = p.col + 1});
    u64 border_walls = border_mask & solution;
    // Monsters can't border monsters or treasures.
    if (border_mask & puzzle.monsters || border_mask & puzzle.treasures) {
        return true;
    }
    return count_set_bits(border_walls) != count_set_bits(border_mask) - 1;
}

bool check_monsters(Puzzle puzzle, u64 solution, i32 slot) {
    // Check monster above.
    Pos p = pos_from_slot(slot);
    Pos above = (Pos){.row = p.row - 1, .col = p.col};
    if (is_invalid_monster(puzzle, solution, above)) {
        return false;
    }
    if (p.row == 7) {
        // Check monster to the left.
        Pos left = (Pos){.row = p.row, .col = p.col - 1};
        if (is_invalid_monster(puzzle, solution, left)) {
            return false;
        }
        if (p.col == 7) {
            // Check monster here.
            if (is_invalid_monster(puzzle, solution, p)) {
                return false;
            }
        }
    }
    return true;
}

bool check_wide_space(Puzzle puzzle, u64 solution, i32 slot) {
    Pos p = pos_from_slot(slot);
    u64 space_mask = 0;
    space_mask = pos_set(space_mask, p);
    space_mask = pos_set(space_mask, (Pos){.row = p.row, .col = p.col - 1});
    space_mask = pos_set(space_mask, (Pos){.row = p.row - 1, .col = p.col});
    space_mask = pos_set(space_mask, (Pos){.row = p.row - 1, .col = p.col - 1});
    if (count_set_bits(space_mask) == 4 && !(space_mask & solution) &&
        !(space_mask & puzzle.monsters) && !(space_mask & puzzle.treasures)) {
        // If we're in a treasure room, then a wide space is ok.
        // Check neighbors for a treasure, if there isn't one, then this is a violation.
        u64 neighbors = 0;
        neighbors = pos_set(neighbors, (Pos){.row = p.row - 2, .col = p.col - 2});
        neighbors = pos_set(neighbors, (Pos){.row = p.row - 2, .col = p.col - 1});
        neighbors = pos_set(neighbors, (Pos){.row = p.row - 2, .col = p.col});
        neighbors = pos_set(neighbors, (Pos){.row = p.row - 2, .col = p.col + 1});
        neighbors = pos_set(neighbors, (Pos){.row = p.row - 1, .col = p.col - 2});
        neighbors = pos_set(neighbors, (Pos){.row = p.row - 1, .col = p.col + 1});
        neighbors = pos_set(neighbors, (Pos){.row = p.row, .col = p.col - 2});
        neighbors = pos_set(neighbors, (Pos){.row = p.row, .col = p.col + 1});
        neighbors = pos_set(neighbors, (Pos){.row = p.row + 1, .col = p.col - 2});
        neighbors = pos_set(neighbors, (Pos){.row = p.row + 1, .col = p.col - 1});
        neighbors = pos_set(neighbors, (Pos){.row = p.row + 1, .col = p.col});
        neighbors = pos_set(neighbors, (Pos){.row = p.row + 1, .col = p.col + 1});
        if (neighbors & puzzle.treasures) {
            return true;
        }
        return false;
    }
    return true;
}

bool is_invalid_treasure_room(Puzzle puzzle, u64 solution, Pos treasure, Pos center, i32 slot) {
    u64 room_mask = 0;
    room_mask = pos_set(room_mask, (Pos){.row = center.row - 1, .col = center.col - 1});
    room_mask = pos_set(room_mask, (Pos){.row = center.row - 1, .col = center.col});
    room_mask = pos_set(room_mask, (Pos){.row = center.row - 1, .col = center.col + 1});
    room_mask = pos_set(room_mask, (Pos){.row = center.row, .col = center.col - 1});
    room_mask = pos_set(room_mask, (Pos){.row = center.row, .col = center.col});
    room_mask = pos_set(room_mask, (Pos){.row = center.row, .col = center.col + 1});
    room_mask = pos_set(room_mask, (Pos){.row = center.row + 1, .col = center.col - 1});
    room_mask = pos_set(room_mask, (Pos){.row = center.row + 1, .col = center.col});
    room_mask = pos_set(room_mask, (Pos){.row = center.row + 1, .col = center.col + 1});
    if (count_set_bits(room_mask) != 9) {
        return true;
    }
    u64 other_treasures = pos_unset(puzzle.treasures, treasure);
    if (room_mask & puzzle.monsters || room_mask & other_treasures) {
        return true;
    }
    if (room_mask & solution) {
        return true;
    }
    u64 walls_mask = 0;
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row - 2, .col = center.col - 1});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row - 2, .col = center.col});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row - 2, .col = center.col + 1});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row + 2, .col = center.col - 1});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row + 2, .col = center.col});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row + 2, .col = center.col + 1});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row - 1, .col = center.col - 2});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row, .col = center.col - 2});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row + 1, .col = center.col - 2});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row - 1, .col = center.col + 2});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row, .col = center.col + 2});
    walls_mask = pos_set(walls_mask, (Pos){.row = center.row + 1, .col = center.col + 2});
    if (walls_mask & puzzle.monsters || walls_mask & puzzle.treasures) {
        return true;
    }
    Pos checked_pos = pos_from_slot(slot);
    if ((checked_pos.row >= center.row + 2 && checked_pos.col >= center.col + 2) ||
        (checked_pos.row == 7 && checked_pos.col == 7)) {
        // Strict checking of 1 opening if we're far enough away from the treasure or at the end.
        if (count_set_bits(walls_mask & solution) != count_set_bits(walls_mask) - 1) {
            return true;
        }
    } else {
        // Only know it's invalid if there's no opening.
        if (count_set_bits(walls_mask & solution) == count_set_bits(walls_mask)) {
            return true;
        }
    }
    return false;
}

bool is_invalid_treasure(Puzzle puzzle, u64 solution, Pos treasure, i32 slot) {
    // Each treasure has 9 possible rooms.
    bool result = true;
    for (i32 row = treasure.row - 1; row <= treasure.row + 1; row++) {
        for (i32 col = treasure.col - 1; col <= treasure.col + 1; col++) {
            Pos center = (Pos){.row = row, .col = col};
            result = result && is_invalid_treasure_room(puzzle, solution, treasure, center, slot);
        }
    }
    return result;
}

// @opt(steve): This one is more expensive so we could pre-compute these masks.
bool check_treasure_rooms(Puzzle puzzle, u64 solution, i32 slot) {
    if (puzzle.treasures == 0) {
        return true;
    }
    // @opt(steve): Probably don't have to check every treasure room each time.
    for (i32 row = 0; row < 8; row++) {
        for (i32 col = 0; col < 8; col++) {
            Pos treasure_pos = (Pos){.row = row, .col = col};
            if (pos_is_set(puzzle.treasures, treasure_pos)) {
                if (is_invalid_treasure(puzzle, solution, treasure_pos, slot)) {
                    return false;
                }
            }
        }
    }
    return true;
}

// Solve a puzzle
// Pushes all found solutions into the passed in `solutions` pointer which is assumed to
// be an empty array with length max_solutions. Returns the number of solutions found.
// The solution space has 2^64 possible solutions, this is a big number that can't fit in a 64-bit
// int.
// The search for solutions uses all the constraints as it goes so it doesn't have to check all
// of these.
u64 solve(Puzzle puzzle, u64 *solutions, u64 max_solutions) {
    u64 solution_i = 0;

    u64 solution = 0;
    i32 slot = 0;

    while (0 <= slot && slot < 64) {
        if (!slot_is_set(solution, slot)) {
            solution = slot_set(solution, slot);
        } else {
            solution = slot_unset(solution, slot);
        }

        // Check constraints.
        if (check_doesnt_overlap(puzzle, solution) && check_row_count(puzzle, solution, slot) &&
            check_col_count(puzzle, solution, slot) && check_dead_ends(puzzle, solution, slot) &&
            check_monsters(puzzle, solution, slot) && check_wide_space(puzzle, solution, slot) &&
            check_treasure_rooms(puzzle, solution, slot)) {

            // Move on to the next slot.
            if (slot < 63) {
                slot++;
                continue;
            }

            // This is a valid solution!
            // Record it but don't increment slot, we want to backtrack and keep searching for
            // more.
            // @opt(steve): Could pass a flag to stop at the first solution if that's all we want.
            if (solution_i >= max_solutions) {
                if (solution_i == max_solutions) {
                    printf("Hit max solutions, no longer recording them.\n");
                }
            } else {
                solutions[solution_i] = solution;
            }
            solution_i++;
        }

        // Backtrack to the last set slot (which could be this one).
        while (slot >= 0 && !slot_is_set(solution, slot)) {
            slot--;
        }
    }

    return solution_i;
}

typedef struct {
    Puzzle puzzle;
    u64 num_solutions;
} GeneratedPuzzle;

// Generate valid puzzles.
// Pushes all found puzzles into the passed in `puzzles` pointer which is assumed to
// be an empty array with length max_puzzles.
// Returns the number of puzzles found.
// Uses a similar strategy to the solver, searches through the puzzle space for valid puzzles.
// Puzzle space here is an empty tile, a wall, a monster or a treasure for every slot.
// The row and col counts are simply derived from a valid puzzle.
// This means there are 4^64 elements in puzzle space, which is much larger than the solution space
// for a specific puzzle, though obviously most of them are not valid.
u64 generate(GeneratedPuzzle *puzzles, u64 max_puzzles) {
    typedef enum { EMPTY = 0, WALL = 1, MONSTER = 2, TREASURE = 3 } Tile;
    u64 puzzle_i = 0;

    Tile puzzle_tiles[64] = {0};
    Puzzle puzzle = {.row_wall_counts = {0, 0, 0, 0, 0, 0, 0},
                     .col_wall_counts = {0, 0, 0, 0, 0, 0, 0},
                     .monsters = 0,
                     .treasures = 0};
    u64 solution = 0;
    i32 slot = 0;

    while (0 <= slot && slot < 64) {
        // Undo previous tile and choose next option.
        switch (puzzle_tiles[slot]) {
        case TREASURE: {
            puzzle_tiles[slot] = MONSTER;
            puzzle.treasures = slot_unset(puzzle.treasures, slot);
            puzzle.monsters = slot_set(puzzle.monsters, slot);
        } break;
        case MONSTER: {
            puzzle_tiles[slot] = WALL;
            puzzle.monsters = slot_unset(puzzle.monsters, slot);
            solution = slot_set(solution, slot);
        } break;
        case WALL: {
            puzzle_tiles[slot] = EMPTY;
            solution = slot_unset(solution, slot);
        } break;
        case EMPTY: {
            puzzle_tiles[slot] = TREASURE;
            puzzle.treasures = slot_set(puzzle.treasures, slot);
        } break;
        }

        // @opt(steve): Broken out so they're easier to debug. Ideally should short circuit.
        bool invalid_monster = !is_invalid_monster(puzzle, solution, pos_from_slot(slot));
        bool overlap = check_doesnt_overlap(puzzle, solution);
        bool dead_ends = check_dead_ends(puzzle, solution, slot);
        bool monsters = check_monsters(puzzle, solution, slot);
        bool wide_space = check_wide_space(puzzle, solution, slot);
        bool treasure = check_treasure_rooms(puzzle, solution, slot);

        // Check constraints.
        if (invalid_monster && overlap && dead_ends && monsters && wide_space && treasure) {

            // Move on to the next slot.
            if (slot < 63) {
                slot++;
                continue;
            }

            // This is a valid puzzle!
            // Record it but don't increment slot, we want to backtrack and keep searching for
            // more.
            Puzzle valid_puzzle = {.row_wall_counts = {0, 0, 0, 0, 0, 0, 0},
                                   .col_wall_counts = {0, 0, 0, 0, 0, 0, 0},
                                   .monsters = puzzle.monsters,
                                   .treasures = puzzle.treasures};
            // Calculate row and col counts.
            for (i32 i = 0; i < 8; i++) {
                valid_puzzle.row_wall_counts[i] = (u8)count_walls_in_row(solution, i);
                valid_puzzle.col_wall_counts[i] = (u8)count_walls_in_col(solution, i);
            }
            // Check number of solutions.
            u64 valid_puzzle_solutions[128];
            u64 num_valid_puzzle_solutions = solve(valid_puzzle, valid_puzzle_solutions, 128);

#if 0
            // @note(steve): Good place to debug stuff. For example this code looks at puzzles with more than one solution.
            if (num_valid_puzzle_solutions != 1) {
                printf("found puzzle with %llu solutions\n", num_valid_puzzle_solutions);
                printf("Generated Solution\n");
                print_puzzle(valid_puzzle, solution);
                printf("Solver Solution 0\n");
                print_puzzle(valid_puzzle, valid_puzzle_solutions[0]);
                printf("Solver Solution 1\n");
                print_puzzle(valid_puzzle, valid_puzzle_solutions[1]);
            }
#endif

            GeneratedPuzzle gen_puzzle = {.puzzle = valid_puzzle,
                                          .num_solutions = num_valid_puzzle_solutions};
            if (puzzle_i < max_puzzles) {
                puzzles[puzzle_i++] = gen_puzzle;
            } else {
                // @note(Steve): Just stopping when the buffer is full, I haven't tried generating
                // or counting them all.
                return puzzle_i;
            }
        }

        // Backtrack to the last set slot (which could be this one).
        while (slot >= 0 && puzzle_tiles[slot] == 0) {
            slot--;
        }
    }
    return puzzle_i;
}

int main(void) {
    PuzzleArgs args = {.row_wall_counts = {1, 4, 3, 2, 4, 5, 3, 3},
                       .col_wall_counts = {1, 3, 6, 2, 4, 2, 3, 4},
                       .monsters = {{.row = 7, .col = 5}},
                       .monsters_count = 1,
                       .treasures = {},
                       .treasures_count = 0};
    Puzzle p = puzzle(args);
    u64 solutions[32];
    u64 num_solutions = solve(p, solutions, 32);
    printf("num solutions: %llu\n", num_solutions);
    for (u64 i = 0; i < num_solutions; i++) {
        printf("Solution %llu\n", i);
        print_puzzle(p, solutions[i]);
    }

    printf("\nGenerating first 8 Puzzles\n");

    // @note(steve): There are a TON of puzzles. I haven't tried counting them all I suspect it'd
    // take a long time.
    GeneratedPuzzle puzzles[8];
    u64 num_puzzles = generate(puzzles, 8);
    printf("Num generated puzzles: %llu\n\n", num_puzzles);
    for (u64 i = 0; i < num_puzzles; i++) {
        printf("Puzzle: %llu\n", i);
        printf("Has %llu solutions\n", puzzles[i].num_solutions);
        print_puzzle(puzzles[i].puzzle, 0);
        printf("\n");
    }
}
