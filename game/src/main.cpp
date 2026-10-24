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

enum GameState
{
    PreGame,
	Playing,
	Victory,
	GameOver
};

enum EnemyType
{
	BASIC,
	FAST,
	TANK,
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
	EnemyType type;
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
        LIME
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



void SetEnemyStats(enemy& e, EnemyType type)
{
	switch (type)
	{
	case BASIC:
		e.speed = 90.0f;
		e.health = 50;
		e.max_health = 50;
		break;
	case FAST:
		e.speed = 150.0f;
		e.health = 30;
		e.max_health = 30;
		break;
	case TANK:
		e.speed = 60.0f;
		e.health = 100;
		e.max_health = 100;
		break;
	default:
		break;
	}
}
 


int main()
{
	InitAudioDevice();

    Sound TurretCreateSound = LoadSound("Assets/Audio/TurretPlace.mp3");
    Sound TurretDestroySound = LoadSound("Assets/Audio/TurretRemove.wav");
    Sound TurretShootSound = LoadSound("Assets/Audio/TurretShoot.ogg");
    Sound EnemyHitSound = LoadSound("Assets/Audio/EnemyHit.mp3");
    Sound EnemyDeathSound = LoadSound("Assets/Audio/EnemyDeath.ogg");

    int Level1[TILE_COUNT][TILE_COUNT]
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



    int Level2[TILE_COUNT][TILE_COUNT]
    {
        //col:0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    row:
			{ 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
			{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
			{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
			{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 3
			{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 4
			{ 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0 }, // 5
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 6 
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 7
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 8
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 9
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 10
			{ 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0 }, // 11
			{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 12
			{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 13
			{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 14
			{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 15
			{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 16
			{ 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0 }, // 17
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 18
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 }  // 19
	};
 

    int Level3[TILE_COUNT][TILE_COUNT]
    {
        //col:0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    row:
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0 }, // 0
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // 1
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // 2
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // 3
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // 4
			{ 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0 }, // 5
			{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
			{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
			{ 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0 }, // 9
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, // 10
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, // 11
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, // 12
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, // 13
			{ 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0 }, // 14
			{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 15
			{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 16
			{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 17
			{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 18
			{ 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // 19
    };
	int (*tiles)[TILE_COUNT] = Level1;

    // This is the game state variable that will be used to control the flow of the game and what point of the game we are in
    GameState state = PreGame;

	int currentLevel = 0; // Variable to track the current level
	int waveCounts[3] = { 15, 30, 50 }; // Array to hold the number of enemies for each level

    std::vector<Cell> waypoints = FloodFill({ 0, 12 }, Level1, WAYPOINT);
    std::vector<enemy> enemies; // here I added a vector of enemies to hold all the enemies that will be spawned in the game
    std::vector<turret> turrets; // here I added a vector of turrets to hold all the turrets that will be spawned in the game
    std::vector<bullet> bullets; // here I added a vector of bullets to hold all the bullets that will be spawned in the game

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


        if (state == PreGame)
        {
            // Initialize turrets based on the tiles array
            Vector2 mouse = GetMousePosition();

            int col = static_cast<int>(mouse.x / TILE_SIZE);
            int row = static_cast<int>(mouse.y / TILE_SIZE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && InBounds({ row, col }) && tiles[row][col] == GRASS && turrets.size() < 5)
            {
                turret t{};
                t.position = TileCenter(row, col);
                t.range = 150.0f;
                t.damage = 15;
                t.fire_rate = 0.8f;

                turrets.push_back(t);

                tiles[row][col] = TURRET;
                PlaySound(TurretCreateSound);
            }

            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && InBounds({ row, col }) && tiles[row][col] == TURRET)
            {
                // Remove turret from the vector
                turrets.erase(std::remove_if(turrets.begin(), turrets.end(), [row, col](const turret& t) {
                    Vector2 center = TileCenter(row, col);
                    return t.position.x == center.x && t.position.y == center.y;
                    }), turrets.end());
                PlaySound(TurretDestroySound);
                // Change tile back to GRASS
                tiles[row][col] = GRASS;
            }
            if (turrets.size() == 5)
            {
                state = Playing;
            }
        }


        if (state == Playing)
        {
            // Update spawn timer and spawn enemies
            SpawnTimer += dt;

            if (SpawnEnemies < waveCounts[currentLevel] && SpawnTimer >= 1.0f)
            {
				enemy e{};

				e.curr = 0;
				e.next = 1;
				e.position = TileCenter(waypoints[e.curr].row, waypoints[e.curr].col);

				e.destroy = false;
				e.at_end = false;  

				EnemyType type = BASIC; 

				// Pick enemy type based on the current level and wave count
				if (currentLevel == 0)
				{
					if (SpawnEnemies >= 10) // Spawn a fast enemy after 10 basic enemies
					{
						type = FAST;
					}
					else
					{
						type = BASIC;
					}
				}
				else if (currentLevel == 1)
				{
					if (SpawnEnemies < 15) // First 15 enemies are basic
					{
						type = BASIC;
					}
					else if (SpawnEnemies < 25) // Next 10 enemies are fast
					{
						type = FAST;
					}
					else // Last 5 enemies are tanks
					{
						type = TANK;
					}
				}
				else if (currentLevel == 2)
				{
					if (SpawnEnemies < 20) // First 20 enemies are basic
					{
						type = BASIC;
					}
					else if (SpawnEnemies < 35) // Next 15 enemies are fast
					{
						type = FAST;
					}
					else // Last 15 enemies are tanks
					{
						type = TANK;
					}
				}

				e.type = type;
				SetEnemyStats(e, type);
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
					if (e.at_end)
					{
						state = GameOver; // Change game state to GameOver if any enemy reaches the end
						break; // Exit the loop since we don't need to check other enemies
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
                        PlaySound(TurretShootSound);
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
                                PlaySound(EnemyDeathSound);
                            }
                            else 
                            {
                                PlaySound(EnemyHitSound);
                            }
                            b.destroy = true; // Bullet has hit the target and set to true for removal
                        }
                    }
                }

                bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const bullet& b) { return b.destroy; }), bullets.end());
                enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const enemy& e) { return e.destroy; }), enemies.end()); 

				if (state == Playing && SpawnEnemies >= waveCounts[currentLevel] && enemies.empty())
				{
					currentLevel++;
					if (currentLevel >= 3)
					{
						state = Victory; // Change game state to Victory if all levels are completed
					}
					else
					{
						// Reset for the next level
						SpawnEnemies = 0;
						SpawnTimer = 0.0f;
						turrets.clear(); 
						bullets.clear();
						enemies.clear(); 
						
						if (currentLevel == 1)
						{
							tiles = Level2;							
							waypoints = FloodFill({ 0, 3 }, tiles, WAYPOINT); // Update waypoints for the new level
						}
						else if (currentLevel == 2)
						{
							tiles = Level3;						
							waypoints = FloodFill({ 0, 17 }, tiles, WAYPOINT); // Update waypoints for the new level
						}

						state = PreGame; // Return to PreGame state to allow turret placement for the next level
					}
				}
        }
        if (state == Victory)
        {

            if (IsKeyPressed(KEY_R))
            {
                // Reset the game state and variables
                state = PreGame;
                currentLevel = 0;
                SpawnEnemies = 0;
                SpawnTimer = 0.0f;
                turrets.clear();
                bullets.clear();
                enemies.clear();
                tiles = Level1; // Reset to the first level				
                waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT); // Reset waypoints for the first level
            }
			if (IsKeyPressed(KEY_Q))
			{
				CloseWindow(); // Close the game window
				return 0; // Exit the program
			}
        }
       
        if (state == GameOver)
        {
          

            if (IsKeyPressed(KEY_R))
            {
                // Reset the game state and variables
                state = PreGame;
                currentLevel = 0;
                SpawnEnemies = 0;
                SpawnTimer = 0.0f;
                turrets.clear();
                bullets.clear();
                enemies.clear();		
                tiles = Level1; // Reset to the first level
                waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT); // Reset waypoints for the first level
            }
            if (IsKeyPressed(KEY_C))
            {
                state = PreGame;
                SpawnEnemies = 0;
                SpawnTimer = 0.0f;
                turrets.clear();
                bullets.clear();
                enemies.clear();

				if (currentLevel == 0)
				{
					tiles = Level1; // Reset to the first level
					waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT); // Reset waypoints for the first level
				}
				else if (currentLevel == 1)
				{
					tiles = Level2; // Reset to the second level
					waypoints = FloodFill({ 0, 3 }, tiles, WAYPOINT); // Reset waypoints for the second level
				}
				else if (currentLevel == 2)
				{
					tiles = Level3; // Reset to the third level
					waypoints = FloodFill({ 0, 17 }, tiles, WAYPOINT); // Reset waypoints for the third level
				}
               
            }
        }


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

			// Draw instructions for placing turrets in PreGame state
			if (state == PreGame)
			{
				DrawText("Place turrets with left click, remove with right click.", 100, 400, 25, WHITE);
			}
			
            if (state == GameOver)
            {
                DrawText("Game Over!", 100, 400, 25, RED);
                DrawText("Press R to restart or C to continue.", 100, 450, 20, WHITE);
            }

			if (state == Victory)
			{
				DrawText("Victory!", 100, 400, 25, GREEN);
				DrawText("Press R to restart.", 100, 450, 20, WHITE);
				DrawText("Press Q to quit.", 100, 500, 20, WHITE);
			}

            for (const turret& t : turrets) // Draw the turrets as blue circles
            {
                DrawCircleV(t.position, 20.0f, BLUE);
            }

            for (const bullet& b : bullets) // Draw the bullets as yellow circles
            {
                DrawCircleV(b.position, 10.0f, DARKGRAY);
            }

            for (const enemy& e : enemies) // Draw the enemies based on their type
            {
                if (e.type == BASIC)
                {
                    DrawCircleV(e.position, 20.0f, RED);
                }
                else if (e.type == FAST)
                {
                    DrawCircleV(e.position, 15.0f, YELLOW);
                }
                else if (e.type == TANK)
                {
                    DrawCircleV(e.position, 25.0f, ORANGE);
                }
            }


            EndDrawing();
        
    }

    UnloadSound(TurretCreateSound);
    UnloadSound(TurretDestroySound);
    UnloadSound(EnemyDeathSound);
    UnloadSound(EnemyHitSound);
    UnloadSound(TurretShootSound);

    CloseAudioDevice();

    CloseWindow();
    return 0;
}