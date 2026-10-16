#include "raylib.h"
#include "raymath.h"
#include "raygui.h"

#include <cassert>
#include <array>
#include <vector>
#include <algorithm>

constexpr float SCREEN_SIZE = 800;

constexpr int TILE_COUNT = 20;
constexpr float TILE_SIZE = SCREEN_SIZE / TILE_COUNT;

enum TileType : int
{
    GRASS,      // Marks unoccupied space, can be overwritten 
    DIRT,       // Marks the path, cannot be overwritten
    WAYPOINT,// Marks where the path turns, cannot be overwritten
    TURRET,
    COUNT
   
};

struct Cell
{
    int row;
    int col;
};
// Here I will make the enemy struct to hold the all relevant information 
struct enemy
{
	Vector2 position;
	float speed;
	int health;
	int max_health;
    bool destroy;

    int curr;
    int next;
	bool at_end;
 };

// Here I will add a struct for the turrets to hold all the relevant information
struct turret
{
	Vector2 position;
	float range = 100;
	float fire_rate = 1.0f;
	int damage = 10;
	enemy* target = nullptr;
    float fire_timer = 0.0f; // Timer to track firing intervals
};

struct bullet
{
	Vector2 position;
	Vector2 direction;
	float speed;
	int damage;
	enemy* target = nullptr;
	bool destroy = false;
};


constexpr std::array<Cell, 4> DIRECTIONS{ Cell{ -1, 0 }, Cell{ 1, 0 }, Cell{ 0, -1 }, Cell{ 0, 1 } };

inline bool InBounds(Cell cell, int rows = TILE_COUNT, int cols = TILE_COUNT)
{
    return cell.col >= 0 && cell.col < cols && cell.row >= 0 && cell.row < rows;
}

void DrawTile(int row, int col, Color color)
{
    DrawRectangle(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, color);
}

void DrawTile(int row, int col, int type)
{
    Color colors[4] =
    {
        LIME,
        BEIGE,
        SKYBLUE,
        ORANGE
    };
    //assert(type >= 0 && type < COUNT);
    DrawTile(row, col, colors[type]);
}

Vector2 TileCenter(int row, int col)
{
    float x = col * TILE_SIZE + TILE_SIZE * 0.5f;
    float y = row * TILE_SIZE + TILE_SIZE * 0.5f;
    return { x, y };
}

// Returns a collection of adjacent cells that match the search value.
std::vector<Cell> FloodFill(Cell start, int tiles[TILE_COUNT][TILE_COUNT], TileType searchValue)
{
    // "open" = "places we want to search", "closed" = "places we've already searched".
    std::vector<Cell> result;
    std::vector<Cell> open;
    bool closed[TILE_COUNT][TILE_COUNT]{};
    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            // We don't want to search zero-tiles, so add them to closed!
            closed[row][col] = tiles[row][col] == 0;
        }
    }

    // Add the starting cell to the exploration queue & search till there's nothing left!
    open.push_back(start);
    while (!open.empty())
    {
        // Remove from queue and prevent revisiting
        Cell cell = open.back();
        open.pop_back();
        closed[cell.row][cell.col] = true;

        // Add to result if explored cell has the desired value
        if (tiles[cell.row][cell.col] == searchValue)
            result.push_back(cell);

        // Search neighbours
        for (Cell dir : DIRECTIONS)
        {
            Cell adj = { cell.row + dir.row, cell.col + dir.col };
            if (InBounds(adj) && !closed[adj.row][adj.col] && tiles[adj.row][adj.col] != 0)
                open.push_back(adj);
        }
    }

    return result;
}

int main()
{
    int tiles[TILE_COUNT][TILE_COUNT]
    {
        //col:0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    row:
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 }, // 0
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 1
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 2
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 3
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 4
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 5
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 6
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0 }, // 7
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 9
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 10
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 11
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 12
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 13
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 14
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 15
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 16
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 17
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 18
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // 19
    };
    std::vector<Cell> waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT);
    std::vector<enemy> enemies; // here I added a vector of enemies to hold all the enemies that will be spawned in the game
    std::vector<turret> turrets; // here I added a vector of turrets to hold all the turrets that will be spawned in the game
	std::vector<bullet> bullets; // here I added a vector of bullets to hold all the bullets that will be spawned in the game

    // Here I will add the 5 turrets to the game
    tiles[5][5] = TURRET;
    tiles[10][5] = TURRET;
    tiles[15][14] = TURRET;
    tiles[2][14] = TURRET;
    tiles[15][7] = TURRET;


    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            if (tiles[row][col] == TURRET)
            {
                turret t{};
                t.position = TileCenter(row, col);
                t.range = 100.0f;
                t.fire_rate = 1.0f;
                t.damage = 10;
                turrets.push_back(t);
            }
        }
    }

	// these are the variables that will be used to spawn the enemies in the game

    int SpawnEnemies = 0;
    float SpawnTimer = 0.0f;


    InitWindow(SCREEN_SIZE, SCREEN_SIZE, "Tower Defense");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Update spawn timer and spawn enemies
        SpawnTimer += dt;

		// the if statement below will spawn an enemy every second until there are 10 enemies in the game
        if (SpawnTimer >= 1.0f && SpawnEnemies < 10)
        {
	    // Create a new enemy and set its initial position to the first waypoint
            enemy e;
            e.curr = 0;
            e.next = 1;
            e.position = TileCenter(waypoints[e.curr].row, waypoints[e.curr].col);

		// these are the stats for the enemy that will be spawned in the game
        e.speed = 90.0f;
        e.health = 50;
        e.max_health = 50;
        e.destroy = false;
        e.at_end = false;

        enemies.push_back(e);

        SpawnEnemies++;
        SpawnTimer -= 1.0f;

    }

		// Enemy movement using a for loop talked about in class 
		for (enemy& e : enemies)
		{
			if (!e.at_end)
			{
				Vector2 from = TileCenter(waypoints[e.curr].row, waypoints[e.curr].col);
				Vector2 to = TileCenter(waypoints[e.next].row, waypoints[e.next].col);
				Vector2 direction = Vector2Normalize(to - from);
				e.position += direction * e.speed * dt;

                float min_distance = e.speed * dt * 1.1f;
             
				// Tolerance depends on enemy speed
				if (CheckCollisionPointCircle(e.position, to, min_distance))
				{
					e.position = to;
					e.curr++;
                    
					if (e.curr >= static_cast<int>(waypoints.size()) - 1)
					{
						e.at_end = true;
					}
					else
					{
						e.next = e.curr + 1;
					}

				}
			}
		}

		// Acquire nearest target for each turret to shoot a bullet at the enemy
        for (turret& t : turrets)
        {
			enemy* nearest_enemy = nullptr;
			float nearest_distance = t.range;
			for (enemy& e : enemies)
			{
				if (!e.at_end && !e.destroy)
				{
					float distance = Vector2Distance(t.position, e.position);
					if (distance < nearest_distance)
					{
						nearest_distance = distance;
						nearest_enemy = &e;
					}
				}
			}
			t.target = nearest_enemy;

			t.fire_timer += dt;

			// Fire bullet at target if available
			if (t.target != nullptr && t.fire_timer >= t.fire_rate)
			{
                bullet b{};
				b.position = t.position;
				b.direction = Vector2Normalize(t.target->position - t.position);
				b.speed = 500.0f;
				b.damage = t.damage;
				b.target = t.target;
				bullets.push_back(b);
				t.fire_timer = 0.0f; // Reset the fire timer
			}
        }

		for (bullet& b : bullets) // Update bullet positions and check for collisions with enemies
		{
			b.position += b.direction * b.speed * dt; // Move the bullet in its direction

			if (b.target != nullptr && !b.target->destroy && !b.target->at_end)
			{
				if (CheckCollisionPointCircle(b.position, b.target->position, 20.0f))
				{
                    b.target->health -= b.damage;
					if (b.target->health <= 0)
					{
						b.target->destroy = true;
					}
					b.destroy = true; // Bullet has hit the target and set to true for removal
				}
			}
		}
		bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const bullet& b) { return b.destroy; }), bullets.end());
		enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const enemy& e) { return e.destroy; }), enemies.end());

        BeginDrawing();
        ClearBackground(BLACK);

        // Draw entire grid
        for (int row = 0; row < TILE_COUNT; row++)
        {
            for (int col = 0; col < TILE_COUNT; col++)
            {
                DrawTile(row, col, tiles[row][col]);
            }
        }

		for (const turret& t : turrets) // Draw the turrets as blue circles
		{
			DrawCircleV(t.position, 20.0f, BLUE);
		}
		
		for (const bullet& b : bullets) // Draw the bullets as yellow circles
		{
			DrawCircleV(b.position, 10.0f, YELLOW);
		}

		for (const enemy& e : enemies) // Draw the enemies as red circles
		{
			DrawCircleV(e.position, 20.0f, RED);
		}



        EndDrawing();
    }
    CloseWindow();
    return 0;
}

