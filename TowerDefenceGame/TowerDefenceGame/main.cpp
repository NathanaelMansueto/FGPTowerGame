#include "raylib.h"
#include "raymath.h" 
#include <string>
#include <vector>  
#include <algorithm>
#include <cmath>

using namespace std;

// ------------------------------------------------------------------------
// Global Constants (same as your code)
// ------------------------------------------------------------------------
const int rows = 16;
const int cols = 22;
const int tileSize = 32;

// ------------------------------------------------------------------------
// Defender Types
// ------------------------------------------------------------------------
enum class DefenderType {
    KNIGHT,
    WIZARD,
    ARCHER
};

// ------------------------------------------------------------------------
// New: Enemy Types
// ------------------------------------------------------------------------
enum class EnemyType {
    GOBLIN,
    ORC
};

// ------------------------------------------------------------------------
// Player
// ------------------------------------------------------------------------
struct Player {
    float gold;

    Player(float g) : gold(g) {}
};

// ------------------------------------------------------------------------
// Game Objects as Classes (using pointers)
// ------------------------------------------------------------------------
class Defender {
public:
    DefenderType type;
    float row, col;
    float range;
    float attackCooldown;
    float attackTimer;
    float cost;
    float maxHealth;
    float currentHealth;

    Defender(float c)
        : type(DefenderType::KNIGHT),
          row(0), col(0),
          range(3.0f),
          attackCooldown(1.0f),
          attackTimer(1.0f), // so it fires immediately
          cost(c),
          maxHealth(100.0f),
          currentHealth(100.0f)
    {}
};

class Enemy {
public:
    float row, col;
    int currentWaypoint;
    float speed;
    bool isAlive;
    bool hasActiveBullet;
    Texture2D texture;
    EnemyType type;    // New enemy type
    float health;      // Health value for the enemy

    // Modified constructor based on enemy type
    Enemy(EnemyType t)
        : row(0), col(0),
          currentWaypoint(0),
          speed(2.0f),
          isAlive(true),
          hasActiveBullet(false),
          texture({0}),
          type(t),
          health(100.0f)
    {
        if (type == EnemyType::GOBLIN) {
            speed = 2.0f;    // Goblins are faster
            health = 50.0f;  // Goblins have lower health
        } else { // ORC
            speed = 1.0f;    // Orcs are slower
            health = 150.0f; // Orcs have higher health
        }
    }
};

struct Bullet {
    Vector2 position;
    Vector2 velocity;
    bool active;
};

struct EnemyBullet {
    Vector2 position;
    Vector2 velocity;
    bool active;
    // pointer to the enemy that fired it
    Enemy* owner;
};

// ------------------------------------------------------------------------
// TowerDefenseGame Class (Encapsulates game state, logic, drawing)
// ------------------------------------------------------------------------
class TowerDefenseGame {
public:
    // Game state objects
    Player* player;
    vector<Defender*> defenders;
    vector<Enemy*> enemies;
    vector<Bullet*> bullets;
    vector<EnemyBullet*> enemyBullets;
    Music backgroundMusic;

    // Map and game variables
    int map[rows][cols];
    bool gameOver;
    int enemiesReached;
    int totalEnemiesToSpawn;
    int spawnedEnemiesCount;
    float spawnTimer;
    const float spawnDelay;

    // Textures
    Texture2D pathTexture, torchTexture, leftColumnTexture, rightColumnTexture;
    Texture2D wallTopLeftTexture, wallTopRightTexture, brickWallTexture;
    Texture2D bottomWallTexture, bottomLeftBrickTexture, bottomRightBrickTexture;
    Texture2D bottomWall2Texture, brickBlockCurveTexture, brickBlockCurveTexture2;
    Texture2D doorRightTexture, doorLeftTexture, dotBrickTexture, dotBrickTexture2;
    Texture2D brickBlockCurve3Texture, brickBlockCurve4Texture, brickBlockCurve5Texture;
    Texture2D brick1;
    Texture2D enemyTexture, knightTexture, wizardTexture, archerTexture;
    Texture2D defenderPath, bulletTexture;
    Texture2D bigHeartTexture, fullHeartTexture, halfHeartTexture, emptyHeartTexture;
    // New enemy textures
    Texture2D goblinTexture, orcTexture;

    int screenWidth, screenHeight;
    DefenderType selectedDefenderType;

    // Enemy path 
    vector<Vector2> enemyPathRC;

    // --------------------------------------------------------------------
    // Constructor: initialize game state, load textures, set up map
    // --------------------------------------------------------------------
    TowerDefenseGame()
        : player(nullptr), defenders(), enemies(), bullets(), enemyBullets(),
          gameOver(false), enemiesReached(10), totalEnemiesToSpawn(20),
          spawnedEnemiesCount(0), spawnTimer(0.0f), spawnDelay(2.0f), // spawn delay now 2 sec
          selectedDefenderType(DefenderType::KNIGHT)
    {
        // Copy your original map layout
        int tempMap[rows][cols] = {
            {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
            {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
            {1,1,5,2,7,2,7,2,7,2,7,2,7,7,7,2,7,2,6,1,1,1},
            {1,1,3,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,4,1,1,1},
            {1,1,3,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,4,1,1,1},
            {1,1,8,8,8,8,8,8,8,12,15,22,20,8,8,8,15,22,4,1,1,1},
            {1,1,11,11,11,11,11,11,11,16,3,22,4,17,11,16,3,22,4,1,1,1},
            {1,1,3,22,22,22,22,22,22,4,3,22,4,3,22,4,3,22,4,1,1,1},
            {1,1,3,22,22,22,22,22,22,4,3,22,4,3,22,4,3,22,4,1,1,1},
            {1,1,3,22,22,22,22,22,22,4,9,8,10,3,22,4,3,22,4,1,1,1},
            {1,1,3,22,22,22,22,22,22,18,11,11,11,19,22,4,3,22,4,1,1,1},
            {1,1,3,22,22,22,22,22,22,22,22,22,22,22,22,4,3,22,4,1,1,1},
            {1,1,3,22,22,22,22,22,22,22,22,22,22,22,22,4,3,22,4,1,1,1},
            {1,1,9,8,8,8,8,8,8,8,8,8,8,8,8,14,13,8,10,1,1,1},
            {1,1,21,21,21,21,1,1,1,1,1,1,1,1,1,21,21,21,21,1,1,1},
            {1,1,21,21,21,21,1,1,1,1,1,1,1,1,1,21,21,21,21,1,1,1}
        };
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                map[r][c] = tempMap[r][c];
            }
        }

        // Set up enemy path 
        enemyPathRC.push_back({6, 1});
        enemyPathRC.push_back({6, 10});
        enemyPathRC.push_back({7, 10});
        enemyPathRC.push_back({8, 10});
        enemyPathRC.push_back({10, 10});
        enemyPathRC.push_back({10, 13});
        enemyPathRC.push_back({6, 13});
        enemyPathRC.push_back({6, 15});
        enemyPathRC.push_back({12, 15});

        screenWidth = cols * tileSize;
        screenHeight = rows * tileSize;

        InitWindow(screenWidth, screenHeight, "Tower Defense Game");
        InitAudioDevice();
        backgroundMusic = LoadMusicStream("Assets/BackGroundMusic(2).mp3");
        PlayMusicStream(backgroundMusic);
        SetTargetFPS(60);

        // Load textures (same as your original calls)
        pathTexture = LoadTexture("Assets/TilePath.png");
        torchTexture = LoadTexture("Assets/torchWall.png");
        leftColumnTexture = LoadTexture("Assets/leftColumnTile.png");
        rightColumnTexture = LoadTexture("Assets/rightColumnTile.png");
        wallTopLeftTexture = LoadTexture("Assets/wallTopLeft.png");
        wallTopRightTexture = LoadTexture("Assets/wallTopRight.png");
        brickWallTexture = LoadTexture("Assets/brickWall.png");
        bottomWallTexture = LoadTexture("Assets/bottomWall.png");
        bottomLeftBrickTexture = LoadTexture("Assets/bottomLeftBrick.png");
        bottomRightBrickTexture = LoadTexture("Assets/bottomRightBrick.png");
        bottomWall2Texture = LoadTexture("Assets/bottomWall2.png");
        brickBlockCurveTexture = LoadTexture("Assets/brickBlokCurve.png");
        brickBlockCurveTexture2 = LoadTexture("Assets/brickBlokCurve2.png");
        doorRightTexture = LoadTexture("Assets/doorRight.png");
        doorLeftTexture = LoadTexture("Assets/doorLeft.png");
        dotBrickTexture = LoadTexture("Assets/dotbrick.png");
        dotBrickTexture2 = LoadTexture("Assets/dotbrick2.png");
        brickBlockCurve3Texture = LoadTexture("Assets/brickblokcurve3.png");
        brickBlockCurve4Texture = LoadTexture("Assets/brickblokcurve4.png");
        brickBlockCurve5Texture = LoadTexture("Assets/brickblokcurve5.png");
        brick1 = LoadTexture("Assets/brick1.png");
        enemyTexture = LoadTexture("Assets/enemy.png"); // fallback texture if needed
        knightTexture = LoadTexture("Assets/knight.png");
        wizardTexture = LoadTexture("Assets/wizzard.png");
        archerTexture = LoadTexture("Assets/archer.png");
        defenderPath = LoadTexture("Assets/DefenderPath.png");
        bulletTexture = LoadTexture("Assets/DefenderBullet.png");
        bigHeartTexture = LoadTexture("Assets/DefenderFullHealth.png");
        fullHeartTexture = LoadTexture("Assets/DefenderFullHealth.png");
        halfHeartTexture = LoadTexture("Assets/DefenderHalfHealth.png");
        emptyHeartTexture = LoadTexture("Assets/DefenderHealthDead.png");
        goblinTexture = LoadTexture("Assets/Enemy2.png");
        orcTexture = LoadTexture("Assets/Enemy.png");

        player = new Player(9999.0f);
    }

    // --------------------------------------------------------------------
    // Destructor: free dynamically allocated objects and unload textures
    // --------------------------------------------------------------------
    ~TowerDefenseGame() {
        for (size_t i = 0; i < defenders.size(); i++) {
            delete defenders[i];
        }
        for (size_t i = 0; i < enemies.size(); i++) {
            delete enemies[i];
        }
        for (size_t i = 0; i < bullets.size(); i++) {
            delete bullets[i];
        }
        for (size_t i = 0; i < enemyBullets.size(); i++) {
            delete enemyBullets[i];
        }
        delete player;

        UnloadTexture(pathTexture);
        UnloadTexture(torchTexture);
        UnloadTexture(leftColumnTexture);
        UnloadTexture(rightColumnTexture);
        UnloadTexture(wallTopLeftTexture);
        UnloadTexture(wallTopRightTexture);
        UnloadTexture(brickWallTexture);
        UnloadTexture(bottomWallTexture);
        UnloadTexture(bottomLeftBrickTexture);
        UnloadTexture(bottomRightBrickTexture);
        UnloadTexture(bottomWall2Texture);
        UnloadTexture(brickBlockCurveTexture);
        UnloadTexture(brickBlockCurveTexture2);
        UnloadTexture(doorRightTexture);
        UnloadTexture(doorLeftTexture);
        UnloadTexture(dotBrickTexture);
        UnloadTexture(dotBrickTexture2);
        UnloadTexture(brickBlockCurve3Texture);
        UnloadTexture(brickBlockCurve4Texture);
        UnloadTexture(brickBlockCurve5Texture);
        UnloadTexture(brick1);
        UnloadTexture(enemyTexture);
        UnloadTexture(knightTexture);
        UnloadTexture(wizardTexture);
        UnloadTexture(archerTexture);
        UnloadTexture(defenderPath);
        UnloadTexture(bulletTexture);
        UnloadTexture(bigHeartTexture);
        UnloadTexture(fullHeartTexture);
        UnloadTexture(halfHeartTexture);
        UnloadTexture(emptyHeartTexture);
        UnloadTexture(goblinTexture);
        UnloadTexture(orcTexture);
        UnloadMusicStream(backgroundMusic);
        CloseAudioDevice();

        CloseWindow();
    }

    // --------------------------------------------------------------------
    // Update Enemy: moves enemy along the path
    // --------------------------------------------------------------------
    void UpdateEnemy(Enemy &enemy, float deltaTime, int totalEnemies) {
        if (!enemy.isAlive) return;

        if (enemy.currentWaypoint >= (int)enemyPathRC.size()) {
            enemy.isAlive = false;
            enemiesReached++;
            if (enemiesReached >= totalEnemies) {
                gameOver = true;
            }
            return;
        }

        float targetRow = enemyPathRC[enemy.currentWaypoint].x;
        float targetCol = enemyPathRC[enemy.currentWaypoint].y;
        float dRow = targetRow - enemy.row;
        float dCol = targetCol - enemy.col;
        float distance = sqrtf(dRow * dRow + dCol * dCol);

        if (distance < 0.1f) {
            enemy.currentWaypoint++;
        } else {
            float invDist = 1.0f / distance;
            float dirRow = dRow * invDist;
            float dirCol = dCol * invDist;
            float move = enemy.speed * deltaTime;
            enemy.row += dirRow * move;
            enemy.col += dirCol * move;
        }
    }

    // --------------------------------------------------------------------
    // Draw Enemy
    // --------------------------------------------------------------------
    void DrawEnemy(const Enemy &enemy) {
        if (!enemy.isAlive) return;
        float x = enemy.col * tileSize;
        float y = enemy.row * tileSize;
        DrawTexture(enemy.texture, (int)x, (int)y, WHITE);
    }

    // --------------------------------------------------------------------
    // Update Defender: spawn bullet if enemy in range
    // --------------------------------------------------------------------
    void UpdateDefender(Defender &def, float deltaTime, vector<Enemy*> &enemiesPtr) {
        def.attackTimer += deltaTime;

        if (def.attackTimer >= def.attackCooldown) {
            Enemy* closestEnemy = nullptr;
            float closestDist = 999999.0f;

            for (size_t i = 0; i < enemiesPtr.size(); i++) {
                Enemy* e = enemiesPtr[i];
                if (!e->isAlive) continue;
                float dRow = e->row - def.row;
                float dCol = e->col - def.col;
                float dist = sqrtf(dRow * dRow + dCol * dCol);
                if (dist < closestDist) {
                    closestDist = dist;
                    closestEnemy = e;
                }
            }

            if (closestEnemy) {
                Vector2 defenderCenter = { (def.col + 0.5f) * tileSize, (def.row + 0.5f) * tileSize };
                Vector2 enemyCenter = { (closestEnemy->col + 0.5f) * tileSize, (closestEnemy->row + 0.5f) * tileSize };
                Vector2 direction = Vector2Subtract(enemyCenter, defenderCenter);
                float distance = Vector2Length(direction);
                if (distance > 0.0f) {
                    direction = Vector2Scale(direction, 1.0f / distance);
                }
                Bullet* newBullet = new Bullet;
                newBullet->position = defenderCenter;
                newBullet->velocity = Vector2Scale(direction, 200.0f);
                newBullet->active = true;
                bullets.push_back(newBullet);
            }
            def.attackTimer = 0.0f;
        }
    }

    // --------------------------------------------------------------------
    // Update Bullets (defender bullets)
    // --------------------------------------------------------------------
    void UpdateBullets(float deltaTime, vector<Enemy*> &enemiesPtr, int screenW, int screenH) {
        for (size_t i = 0; i < bullets.size(); i++) {
            Bullet* b = bullets[i];
            if (!b->active) continue;
            b->position = Vector2Add(b->position, Vector2Scale(b->velocity, deltaTime));

            if (b->position.x < 0 || b->position.x > screenW ||
                b->position.y < 0 || b->position.y > screenH) {
                b->active = false;
                continue;
            }

            for (size_t j = 0; j < enemiesPtr.size(); j++) {
                Enemy* e = enemiesPtr[j];
                if (!e->isAlive) continue;
                Vector2 enemyCenter = { (e->col + 0.5f) * tileSize, (e->row + 0.5f) * tileSize };
                float dx = b->position.x - enemyCenter.x;
                float dy = b->position.y - enemyCenter.y;
                float distSqr = dx * dx + dy * dy;
                float collisionRange = 16.0f;
                if (distSqr < collisionRange * collisionRange) {
                    e->isAlive = false;
                    b->active = false;
                    
                    player->gold += 50.0f;
                    break;
                }
            }
        }
        // Remove inactive bullets
        bullets.erase(remove_if(bullets.begin(), bullets.end(),
            [](Bullet* b) { 
                if (!b->active) { 
                    delete b; 
                    return true; 
                } 
                return false; 
            }), bullets.end());
    }

    // --------------------------------------------------------------------
    // Draw Bullets
    // --------------------------------------------------------------------
    void DrawBullets(Texture2D bulletTex) {
        for (size_t i = 0; i < bullets.size(); i++) {
            Bullet* b = bullets[i];
            if (!b->active) continue;
            float angleDeg = atan2f(b->velocity.y, b->velocity.x) * RAD2DEG;
            Vector2 drawPos = { b->position.x - bulletTex.width * 0.5f, b->position.y - bulletTex.height * 0.5f };
            DrawTextureEx(bulletTex, drawPos, angleDeg, 1.0f, WHITE);
        }
    }

    // --------------------------------------------------------------------
    // Draw Defenders (with heart health indicator)
    // --------------------------------------------------------------------
    void DrawDefenders(const vector<Defender*> &defendersPtr,
                       Texture2D knightTex,
                       Texture2D wizardTex,
                       Texture2D archerTex,
                       Texture2D bigHeartTex,
                       Texture2D fullHeartTex,
                       Texture2D halfHeartTex,
                       Texture2D emptyHeartTex)
    {
        for (size_t i = 0; i < defendersPtr.size(); i++) {
            Defender* d = defendersPtr[i];
            float tileX = d->col * tileSize;
            float tileY = d->row * tileSize;

            float healthRatio = d->currentHealth / d->maxHealth;
            Texture2D heartToDraw;
            if (healthRatio >= 1.0f) {
                heartToDraw = fullHeartTex;
            } else if (healthRatio >= 0.5f) {
                heartToDraw = halfHeartTex;
            } else {
                heartToDraw = emptyHeartTex;
            }
            // Draw the heart one tile below
            float heartScale = (float)tileSize / heartToDraw.width;
            Vector2 heartPos = { tileX, tileY + tileSize };
            DrawTextureEx(heartToDraw, heartPos, 0.0f, heartScale, WHITE);

            Texture2D defTex;
            switch (d->type) {
                case DefenderType::KNIGHT: defTex = knightTex; break;
                case DefenderType::WIZARD: defTex = wizardTex; break;
                case DefenderType::ARCHER: defTex = archerTex; break;
            }
            float defScale = (float)tileSize / (defTex.width * 1.25f);
            float offsetX = (tileSize - defTex.width * defScale) * 0.5f;
            float offsetY = tileSize - (defTex.height * defScale);
            Vector2 defPos = { tileX + offsetX, tileY + offsetY };
            DrawTextureEx(defTex, defPos, 0.0f, defScale, WHITE);
        }
    }

    // --------------------------------------------------------------------
    // Enemy Bullet Functionality
    // --------------------------------------------------------------------
    void UpdateEnemyShooting(float deltaTime, vector<Enemy*> &enemiesPtr, vector<Defender*> &defendersPtr) {
        const float enemyAttackRange = 5.0f; // Adjust this value as needed
        for (size_t i = 0; i < enemiesPtr.size(); i++) {
            Enemy* e = enemiesPtr[i];
            if (!e->isAlive) continue;
            if (e->hasActiveBullet) continue;  // Ensures each enemy only has one bullet at a time
    
            Defender* target = nullptr;
            float closestDist = 999999.0f;
            for (size_t j = 0; j < defendersPtr.size(); j++) {
                Defender* d = defendersPtr[j];
                float dRow = d->row - e->row;
                float dCol = d->col - e->col;
                float dist = sqrtf(dRow * dRow + dCol * dCol);
                // Check if the defender is within the enemy's attack range
                if (dist < enemyAttackRange && dist < closestDist) {
                    closestDist = dist;
                    target = d;
                }
            }
            if (target) {
                Vector2 enemyCenter = { (e->col + 0.5f) * tileSize, (e->row + 0.5f) * tileSize };
                Vector2 defenderCenter = { (target->col + 0.5f) * tileSize, (target->row + 0.5f) * tileSize };
                Vector2 direction = Vector2Subtract(defenderCenter, enemyCenter);
                float distance = Vector2Length(direction);
                if (distance > 0.0f) {
                    direction = Vector2Scale(direction, 1.0f / distance);
                }
                EnemyBullet* newBullet = new EnemyBullet;
                newBullet->position = enemyCenter;
                newBullet->velocity = Vector2Scale(direction, 200.0f);
                newBullet->active = true;
                newBullet->owner = e;
                enemyBullets.push_back(newBullet);
                e->hasActiveBullet = true;
            }
        }
    }
    

    void UpdateEnemyBullets(float deltaTime, vector<Defender*> &defendersPtr, int screenW, int screenH) {
        for (size_t i = 0; i < enemyBullets.size(); i++) {
            EnemyBullet* b = enemyBullets[i];
            if (!b->active) continue;
            b->position = Vector2Add(b->position, Vector2Scale(b->velocity, deltaTime));

            if (b->position.x < 0 || b->position.x > screenW ||
                b->position.y < 0 || b->position.y > screenH) {
                if (b->owner)
                    b->owner->hasActiveBullet = false;
                b->active = false;
                continue;
            }
            for (size_t j = 0; j < defendersPtr.size(); j++) {
                Defender* d = defendersPtr[j];
                Vector2 defCenter = { (d->col + 0.5f) * tileSize, (d->row + 0.5f) * tileSize };
                float dx = b->position.x - defCenter.x;
                float dy = b->position.y - defCenter.y;
                float distSqr = dx * dx + dy * dy;
                float collisionRange = 16.0f;
                if (distSqr < collisionRange * collisionRange) {
                    d->currentHealth -= 50.0f;
                    if (b->owner)
                        b->owner->hasActiveBullet = false;
                    b->active = false;
                    break;
                }
            }
        }
        enemyBullets.erase(remove_if(enemyBullets.begin(), enemyBullets.end(),
            [](EnemyBullet* eb) { 
                if (!eb->active) { delete eb; return true; }
                return false;
            }), enemyBullets.end());
    }

    void DrawEnemyBullets(Texture2D bulletTex) {
        for (size_t i = 0; i < enemyBullets.size(); i++) {
            EnemyBullet* b = enemyBullets[i];
            if (!b->active) continue;
            float angleDeg = atan2f(b->velocity.y, b->velocity.x) * RAD2DEG;
            Vector2 drawPos = { b->position.x - bulletTex.width * 0.5f, b->position.y - bulletTex.height * 0.5f };
            DrawTextureEx(bulletTex, drawPos, angleDeg, 1.0f, WHITE);
        }
    }

    // --------------------------------------------------------------------
    // Draw Map and Tower Cost Boxes 
    // --------------------------------------------------------------------
    void DrawMap(Texture2D pathTexture, Texture2D torchTexture, Texture2D leftColumnTexture,
                 Texture2D rightColumnTexture, Texture2D wallTopLeftTexture, Texture2D wallTopRightTexture,
                 Texture2D brickWallTexture, Texture2D bottomWallTexture,
                 Texture2D bottomLeftBrickTexture, Texture2D bottomRightBrickTexture,
                 Texture2D bottomWall2Texture, Texture2D brickBlockCurveTexture,
                 Texture2D brickBlockCurveTexture2, Texture2D doorRightTexture,
                 Texture2D doorLeftTexture, Texture2D dotBrickTexture, Texture2D dotBrickTexture2,
                 Texture2D brickBlockCurve3Texture, Texture2D brickBlockCurve4Texture,
                 Texture2D brickBlockCurve5Texture, Texture2D brick1, Texture2D defenderPath)
    {
        for (int rowIdx = 0; rowIdx < rows; rowIdx++) {
            for (int colIdx = 0; colIdx < cols; colIdx++) {
                Vector2 pos = { (float)(colIdx * tileSize), (float)(rowIdx * tileSize) };
                switch (map[rowIdx][colIdx]) {
                    case 1:
                        DrawTexture(pathTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 2:
                        DrawTexture(torchTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 3:
                        DrawTexture(leftColumnTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 4:
                        DrawTexture(rightColumnTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 5:
                        DrawTexture(wallTopLeftTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 6:
                        DrawTexture(wallTopRightTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 7:
                        DrawTexture(brickWallTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 8:
                        DrawTexture(bottomWallTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 9:
                        DrawTexture(bottomLeftBrickTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 10:
                        DrawTexture(bottomRightBrickTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 11:
                        DrawTexture(bottomWall2Texture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 12:
                        DrawTexture(brickBlockCurveTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 15:
                        DrawTexture(brickBlockCurveTexture2, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 16:
                        DrawTexture(dotBrickTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 17:
                        DrawTexture(dotBrickTexture2, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 13:
                        DrawTexture(doorRightTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 14:
                        DrawTexture(doorLeftTexture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 18:
                        DrawTexture(brickBlockCurve3Texture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 19:
                        DrawTexture(brickBlockCurve4Texture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 20:
                        DrawTexture(brickBlockCurve5Texture, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 21:
                        DrawTexture(brick1, (int)pos.x, (int)pos.y, WHITE);
                        break;
                    case 22:
                        DrawTexture(defenderPath, (int)pos.x, (int)pos.y, WHITE);
                        break;
                }
            }
        }
    }

    void DrawTowerCosts(Texture2D knightTexture, Texture2D wizardTexture, Texture2D archerTexture) {
        int fontSize = 20;
        float scale = 2.0f;
        // Knight cost box
        {
            Rectangle knightCostBox = {610, 150, 100, 30};
            DrawRectangleRec(knightCostBox, RAYWHITE);
            DrawRectangleLinesEx(knightCostBox, 2, BLACK);
            DrawText("Cost:150", (int)knightCostBox.x + 5, (int)knightCostBox.y + 5, fontSize, BLACK);
            int knightWidth  = (int)(knightTexture.width  * scale);
            int knightHeight = (int)(knightTexture.height * scale);
            int knightX = (int)(knightCostBox.x + (knightCostBox.width - knightWidth) / 2);
            int knightY = (int)(knightCostBox.y - knightHeight);
            Vector2 knightPos = {(float)knightX, (float)knightY};
            DrawTextureEx(knightTexture, knightPos, 0.0f, scale, WHITE);
        }
        // Wizard cost box
        {
            Rectangle wizardCostBox = {610, 250, 100, 30};
            DrawRectangleRec(wizardCostBox, RAYWHITE);
            DrawRectangleLinesEx(wizardCostBox, 2, BLACK);
            DrawText("Cost:200", (int)wizardCostBox.x + 5, (int)wizardCostBox.y + 5, fontSize, BLACK);
            int wizardWidth  = (int)(wizardTexture.width  * scale);
            int wizardHeight = (int)(wizardTexture.height * scale);
            int wizardX = (int)(wizardCostBox.x + (wizardCostBox.width - wizardWidth) / 2);
            int wizardY = (int)(wizardCostBox.y - wizardHeight);
            Vector2 wizardPos = {(float)wizardX, (float)wizardY};
            DrawTextureEx(wizardTexture, wizardPos, 0.0f, scale, WHITE);
        }
        // Archer cost box
        {
            Rectangle archerCostBox = {610, 350, 100, 30};
            DrawRectangleRec(archerCostBox, RAYWHITE);
            DrawRectangleLinesEx(archerCostBox, 2, BLACK);
            DrawText("Cost:250", (int)archerCostBox.x + 5, (int)archerCostBox.y + 5, fontSize, BLACK);
            int archerWidth  = (int)(archerTexture.width  * scale);
            int archerHeight = (int)(archerTexture.height * scale);
            int archerX = (int)(archerCostBox.x + (archerCostBox.width - archerWidth) / 2);
            int archerY = (int)(archerCostBox.y - archerHeight);
            Vector2 archerPos = {(float)archerX, (float)archerY};
            DrawTextureEx(archerTexture, archerPos, 0.0f, scale, WHITE);
        }
    }

    void RemoveDeadDefenders(vector<Defender*> &defendersPtr) {
        for (int i = defendersPtr.size() - 1; i >= 0; i--) {
            if (defendersPtr[i]->currentHealth <= 0.0f) {
                delete defendersPtr[i];
                defendersPtr.erase(defendersPtr.begin() + i);
            }
        }
    }

    // --------------------------------------------------------------------
    // Main Game Loop
    // --------------------------------------------------------------------
    void Run() {
        bool exitClicked = false;
        while (!WindowShouldClose() && !exitClicked) {
            float deltaTime = GetFrameTime();

            // Update background music
            UpdateMusicStream(backgroundMusic);
            
            spawnTimer += deltaTime;
            // ----------------------------------------------------------------
            // Spawn enemy using a single spawn timer with random enemy type
            // ----------------------------------------------------------------
            if (spawnedEnemiesCount < totalEnemiesToSpawn && spawnTimer >= spawnDelay) {
                spawnTimer = 0.0f;
                // Randomly select an enemy type: 0 for Goblin, 1 for Orc
                int randVal = GetRandomValue(0, 1);
                EnemyType chosenType = (randVal == 0) ? EnemyType::GOBLIN : EnemyType::ORC;
                Enemy* newEnemy = new Enemy(chosenType);
                newEnemy->row = 6.0f;
                newEnemy->col = 1.0f; // You can adjust the spawn position as needed
                newEnemy->currentWaypoint = 1;
                // Set texture based on enemy type
                if (chosenType == EnemyType::GOBLIN) {
                    newEnemy->texture = goblinTexture;
                } else {
                    newEnemy->texture = orcTexture;
                }
                enemies.push_back(newEnemy);
                spawnedEnemiesCount++;
            }
            // 2) Update enemies
            for (size_t i = 0; i < enemies.size(); i++) {
                UpdateEnemy(*enemies[i], deltaTime, totalEnemiesToSpawn);
            }
            enemies.erase(remove_if(enemies.begin(), enemies.end(),
                [](Enemy* e) { 
                    if (!e->isAlive) { delete e; return true; }
                    return false;
                }), enemies.end());
            // 3) Handle clicks (for placing defenders or selecting types)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mousePos = GetMousePosition();
                Rectangle knightCostBox = {610, 150, 100, 30};
                Rectangle wizardCostBox = {610, 250, 100, 30};
                Rectangle archerCostBox = {610, 350, 100, 30};
                if (CheckCollisionPointRec(mousePos, knightCostBox)) {
                    selectedDefenderType = DefenderType::KNIGHT;
                } else if (CheckCollisionPointRec(mousePos, wizardCostBox)) {
                    selectedDefenderType = DefenderType::WIZARD;
                } else if (CheckCollisionPointRec(mousePos, archerCostBox)) {
                    selectedDefenderType = DefenderType::ARCHER;
                } else {
                    int c = (int)(mousePos.x / tileSize);
                    int r = (int)(mousePos.y / tileSize);
                    if (r >= 0 && r < rows && c >= 0 && c < cols) {
                        if (map[r][c] == 22) { // Defender Path
                            float costNeeded = 0.0f;
                            switch (selectedDefenderType) {
                                case DefenderType::KNIGHT: costNeeded = 150.0f; break;
                                case DefenderType::WIZARD: costNeeded = 200.0f; break;
                                case DefenderType::ARCHER: costNeeded = 250.0f; break;
                            }
                            if (player->gold >= costNeeded) {
                                player->gold -= costNeeded;
                                Defender* newDef = new Defender(costNeeded);
                                newDef->type = selectedDefenderType;
                                newDef->row = (float)r;
                                newDef->col = (float)c;
                                if (selectedDefenderType == DefenderType::KNIGHT) {
                                    newDef->maxHealth = 100.0f;
                                    newDef->currentHealth = 100.0f;
                                } else if (selectedDefenderType == DefenderType::WIZARD) {
                                    newDef->maxHealth = 100.0f;
                                    newDef->currentHealth = 100.0f;
                                } else if (selectedDefenderType == DefenderType::ARCHER) {
                                    newDef->maxHealth = 100.0f;
                                    newDef->currentHealth = 100.0f;
                                }
                                defenders.push_back(newDef);
                            }
                        }
                    }
                }
            }
            // 4) Update defenders (each may spawn a bullet)
            for (size_t i = 0; i < defenders.size(); i++) {
                UpdateDefender(*defenders[i], deltaTime, enemies);
            }
            // 5) Update enemy shooting (one bullet per enemy)
            UpdateEnemyShooting(deltaTime, enemies, defenders);
            // 6) Update defender bullets
            UpdateBullets(deltaTime, enemies, screenWidth, screenHeight);
            // 7) Update enemy bullets
            UpdateEnemyBullets(deltaTime, defenders, screenWidth, screenHeight);

            RemoveDeadDefenders(defenders);
            

            BeginDrawing();
            ClearBackground(DARKPURPLE);

            DrawMap(pathTexture, torchTexture, leftColumnTexture, rightColumnTexture,
                    wallTopLeftTexture, wallTopRightTexture, brickWallTexture, bottomWallTexture,
                    bottomLeftBrickTexture, bottomRightBrickTexture, bottomWall2Texture,
                    brickBlockCurveTexture, brickBlockCurveTexture2, doorRightTexture, doorLeftTexture,
                    dotBrickTexture, dotBrickTexture2, brickBlockCurve3Texture, brickBlockCurve4Texture,
                    brickBlockCurve5Texture, brick1, defenderPath);

            DrawTowerCosts(knightTexture, wizardTexture, archerTexture);

            for (size_t i = 0; i < enemies.size(); i++) {
                DrawEnemy(*enemies[i]);
            }
            DrawDefenders(defenders, knightTexture, wizardTexture, archerTexture,
                         bigHeartTexture, fullHeartTexture, halfHeartTexture, emptyHeartTexture);
            DrawBullets(bulletTexture);
            DrawEnemyBullets(bulletTexture);

            if (gameOver) {
                const char* gameOverText = "Game Over";
                int fontSize = 40;
                int textWidth = MeasureText(gameOverText, fontSize);
                int textX = (screenWidth / 2) - (textWidth / 2);
                int textY = (screenHeight / 2) - (fontSize / 2);
                DrawText(gameOverText, textX, textY, fontSize, RED);
            }

            // EXIT button
            {
                int buttonWidth = 120;
                int buttonHeight = 60;
                int buttonX = screenWidth - buttonWidth - 98;
                int buttonY = screenHeight - buttonHeight - 4;
                const char* exitText = "< EXIT >";
                int exitFontSize = 20;
                int exitTextWidth = MeasureText(exitText, exitFontSize);
                DrawText(exitText,
                         buttonX + (buttonWidth - exitTextWidth) / 2,
                         buttonY + (buttonHeight - exitFontSize) / 2,
                         exitFontSize, BLACK);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 mousePos = GetMousePosition();
                    if (mousePos.x > buttonX && mousePos.x < buttonX + buttonWidth &&
                        mousePos.y > buttonY && mousePos.y < buttonY + buttonHeight)
                    {
                        exitClicked = true;
                    }
                }
            }

            // "X" button for refund
            {
                int xButtonWidth = 60;
                int xButtonHeight = 60;
                int xButtonX = 100;
                int xButtonY = 450;
                const char* xText = "X";
                int xFontSize = 40;
                int xTextWidth = MeasureText(xText, xFontSize);
                float textX = xButtonX + (xButtonWidth - xTextWidth) / 2.0f;
                float textY = xButtonY + (xButtonHeight - xFontSize) / 2.0f;
                DrawTextPro(GetFontDefault(), xText,
                            (Vector2){ textX + xTextWidth / 2.0f, textY + xFontSize / 2.0f },
                            (Vector2){ xTextWidth / 2.0f, xFontSize / 2.0f },
                            90.0f, (float)xFontSize, 1.0f, RED);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 mousePos = GetMousePosition();
                    if (mousePos.x > xButtonX && mousePos.x < xButtonX + xButtonWidth &&
                        mousePos.y > xButtonY && mousePos.y < xButtonY + xButtonHeight)
                    {
                        float totalRefund = DeleteAllDefenders(defenders);
                        player->gold += totalRefund;
                    }
                }
            }

            // Money & Enemies label
            {
                int fontSize = 24;
                Color textColor = YELLOW;
                DrawText(TextFormat("Money: %i", (int)player->gold), 20, 20, fontSize, textColor);
                int enemiesLeft = (totalEnemiesToSpawn - spawnedEnemiesCount) + (int)enemies.size();
                int enemiesLabelWidth = MeasureText(TextFormat("Enemies: %i", enemiesLeft), fontSize);
                int posX = screenWidth - enemiesLabelWidth - 20;
                int posY = 20;
                DrawText(TextFormat("Enemies: %i", enemiesLeft), posX, posY, fontSize, textColor);
            }
            EndDrawing();
        }
    }

    // --------------------------------------------------------------------
    // Delete All Defenders: remove all defenders and return total refund
    // --------------------------------------------------------------------
    float DeleteAllDefenders(vector<Defender*> &defendersPtr) {
        float totalRefund = 0.0f;
        for (size_t i = 0; i < defendersPtr.size(); i++) {
            totalRefund += defendersPtr[i]->cost;
            delete defendersPtr[i];
        }
        defendersPtr.clear();
        return totalRefund;
    }
};

// --------------------------------------------------------------------
// main()
// --------------------------------------------------------------------
int main() {
    TowerDefenseGame game;
    game.Run();
    return 0;
}
