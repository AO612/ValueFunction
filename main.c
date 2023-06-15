#include <stdlib.h>
#include <stdio.h>
#include <time.h>
// clang -o main main.c libraylib.a -framework IOKit -framework Cocoa -framework OpenGL
// ./main
#include "raylib.h"
#include "raymath.h"

#define COLS 20
#define ROWS 20

typedef enum CellType
{
	OPEN,
	GOAL,
	HOLE,
    OBSTRUCTION
} CellType;

typedef struct Cell
{
	int x;
	int y;
	CellType cellType;
	float value;
	int action;
} Cell;

typedef struct Map
{
	float theta;
	float probability;
	float gamma;
	Cell grid[COLS][ROWS];
	int max_iterations;
	int movementPenalty;
	int collisionPenalty;
    int cellWidth;
	int cellHeight;
} Map;

void CellDraw(Cell*, int, int);
void DrawDirections(Cell*, int, int);
bool IndexIsValid(int, int);
void ChangeCellType(Cell*);
void GridInit(Map*);
void MapInit(Map*);
void ValueIteration(Map*);
void ComputeValueFunction(Map*);
void ExtractPolicy(Map*);
float CalculateValue(Map*, int, int, int);

int main()
{
	srand(time(0));

	int screenWidth = 760;
	int screenHeight = 760;

    Map map;

    map.cellWidth = screenWidth / COLS;
	map.cellHeight = screenHeight / ROWS;

	InitWindow(screenWidth, screenHeight, "Value Iteration");

	MapInit(&map);
	
	while(!WindowShouldClose())
	{

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			Vector2 mPos = GetMousePosition();
			int x = mPos.x / map.cellWidth;
			int y = mPos.y / map.cellHeight;

            if (IndexIsValid(x, y))
			{
				ChangeCellType(&map.grid[x][y]);
			}

		}

		if (IsKeyPressed(KEY_SPACE))
		{
			ValueIteration(&map);
		}

		if (IsKeyPressed(KEY_R))
		{
			MapInit(&map);
		}

		BeginDrawing();

        ClearBackground(RAYWHITE);
        
        for (int x = 0; x < COLS; x++)
        {
            for (int y = 0; y < ROWS; y++)
            {
                CellDraw(&map.grid[x][y], map.cellWidth, map.cellHeight);
            }
        }

		EndDrawing();
	}
	
	CloseWindow();
	
	return 0;
}

void CellDraw(Cell *cell, int cellWidth, int cellHeight)
{
	int font = 12;
    if (cell->cellType == OBSTRUCTION)
    {
        DrawRectangle(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, PURPLE);
    }
	else
	{
		if (cell->cellType == GOAL)
		{
			DrawRectangle(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, (Color){255, 255, 125, 255 } );
		}
		else if (cell->cellType == HOLE)
		{
			DrawRectangle(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, (Color){55, 125, 100, 255 } );
		}
		else
		{
			int max = 100;
			int min = -100;
			int r = 55 + 200*((cell->value-min)/(max-min));
			int g = 125 + 130*((cell->value-min)/(max-min));
			int b = 100 + 25*((cell->value-min)/(max-min));
			r = r < 55 ? 55 : r;
			g = g < 125 ? 125 : g;
			b = b < 100 ? 100 : b;
			DrawRectangle(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, (Color){r, g, b, 255 } );
		}
		DrawDirections(cell, cellWidth, cellHeight);
		DrawText(TextFormat("%0.1f",cell->value), (cell->x + 0.1f) * cellWidth, (cell->y + 0.3f) * cellHeight, font, DARKGRAY);
	}
	DrawRectangleLines(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, BLACK);
}

void DrawDirections(Cell *cell, int cellWidth, int cellHeight)
{
	if (cell->action == 8)
	{
	}
	else
	{
		int x;
		int y;
		Vector2 start;
		Vector2 end;
		if (cell->action == 0)
		{
			x = cell->x*cellWidth + cellWidth/2;
			y = cell->y*cellHeight + cellHeight/4;
			end = (Vector2){x, y + 0.65*cellHeight};
		}
		else if (cell->action == 1)
		{
			x = cell->x*cellWidth + 3*cellWidth/4;
			y = cell->y*cellHeight + cellHeight/4;
			end = (Vector2){x - 0.53*cellWidth, y + 0.53*cellHeight};
		}
		else if (cell->action == 2)
		{
			x = cell->x*cellWidth + 3*cellWidth/4;
			y = cell->y*cellHeight + cellHeight/2;
			end = (Vector2){x - 0.65*cellHeight, y};
		}
		else if (cell->action == 3)
		{
			x = cell->x*cellWidth + 3*cellWidth/4;
			y = cell->y*cellHeight + 3*cellHeight/4;
			end = (Vector2){x - 0.53*cellWidth, y - 0.53*cellHeight};
		}
		else if (cell->action == 4)
		{
			x = cell->x*cellWidth + cellWidth/2;
			y = cell->y*cellHeight + 3*cellHeight/4;
			end = (Vector2){x, y - 0.65*cellHeight};
		}
		else if (cell->action == 5)
		{
			x = cell->x*cellWidth + cellWidth/4;
			y = cell->y*cellHeight + 3*cellHeight/4;
			end = (Vector2){x + 0.53*cellWidth, y - 0.53*cellHeight};
		}
		else if (cell->action == 6)
		{
			x = cell->x*cellWidth + cellWidth/4;
			y = cell->y*cellHeight + cellHeight/2;
			end = (Vector2){x + 0.65*cellHeight, y};
		}
		else if (cell->action == 7)
		{
			x = cell->x*cellWidth + cellWidth/4;
			y = cell->y*cellHeight + cellHeight/4;
			end = (Vector2){x + 0.53*cellWidth, y + 0.53*cellHeight};
		}
		start = (Vector2){x,y};
		DrawCircle(x, y, cellWidth/8, RED);
		DrawLineEx(start, end, 2, RED);
	}
}

void ChangeCellType(Cell *cell)
{
    if (cell->cellType == OPEN)
    {
        cell->cellType = GOAL;
		cell->value = 100;
    }
    else if (cell->cellType == GOAL)
    {
        cell->cellType = HOLE;
		cell->value = -100;
    }
    else if (cell->cellType == HOLE)
    {
        cell->cellType = OBSTRUCTION;
		cell->value = 0;
    }
    else if (cell->cellType == OBSTRUCTION)
    {
        cell->cellType = OPEN;
    }
}


bool IndexIsValid(int x, int y)
{
	return x >= 0 && x < COLS && y >= 0 && y < ROWS;
}

void GridInit(Map *map)
{
	for (int x = 0; x < COLS; x++)
	{
		for (int y = 0; y < ROWS; y++)
		{
			map->grid[x][y] = (Cell)
			{
				.x = x,
				.y = y,
                .cellType = OPEN,
				.value = 0,
				.action = 8
			};
		}
	}

	int obstaclesPresent = (int)(ROWS * COLS * 0.1f);
	int obstaclesToPlace = obstaclesPresent;
	while (obstaclesToPlace > 0)
	{
		int x = rand() % COLS;
		int y = rand() % ROWS;

		if (map->grid[x][y].cellType == OPEN)
		{
			map->grid[x][y].cellType = OBSTRUCTION;
			obstaclesToPlace--;
		}
	}
}

void MapInit(Map *map)
{
    map->theta = 1e-6;
    map->probability = 0.8;
    map->gamma = 1;
    map->max_iterations = 100;
    map->movementPenalty = -10;
    map->collisionPenalty = -50;
	GridInit(map);
}

void ValueIteration(Map *map)
{
	ComputeValueFunction(map);
	ExtractPolicy(map);
}

void ComputeValueFunction(Map *map)
{
	bool loop = true;
	float old_v;
	float new_v;
	float max_v;
	float delta;
	int iterations = 0;
    
	while (loop == true)
	{
		printf("%d\n",iterations);
		delta = 0;
		for (int x = 0; x < COLS; x++)
		{
			for (int y = 0; y < ROWS; y++)
			{	
				if (map->grid[x][y].cellType == OPEN)
				{
					old_v = map->grid[x][y].value;
					max_v = 0;
					for (int action = 0; action < 8; action++)
					{
						new_v = CalculateValue(map, action, x, y);

						if (action == 0 || new_v > max_v)
						{
							max_v = new_v;
						}

					}

					map->grid[x][y].value = max_v;

					if (fabsf(old_v - max_v) > delta)
					{
						delta = fabsf(old_v - max_v);
					}

					BeginDrawing();

					CellDraw(&map->grid[x][y], map->cellWidth, map->cellHeight);

					EndDrawing();

				}
			}
		}
		
		iterations += 1;

		if (delta < map->theta)
		{
			loop = false;
		}
		if (iterations > map->max_iterations)
		{
			loop = false;
		}
	}
}

void ExtractPolicy(Map *map)
{
	int best_action;
	float old_v;
	float new_v;
	float max_v;
	float delta;
	for (int x = 0; x < COLS; x++)
	{
		for (int y = 0; y < ROWS; y++)
		{	
			if (map->grid[x][y].cellType == OPEN)
			{
				old_v = map->grid[x][y].value;
				max_v = 0;
				for (int action = 0; action < 8; action++)
				{
					new_v = CalculateValue(map, action, x, y);

					if (action == 0 || new_v > max_v)
					{
						max_v = new_v;
						best_action = action;
					}

				}
				map->grid[x][y].action = best_action;
			}
		}
	}
}

float CalculateValue(Map *map, int action, int x, int y)
{
	float new_probability;
	int new_x;
	int new_y;
	int new_reward;
	int new_action;
	float new_v = 0;

	for (int i = -1; i < 2; i++)
	{
		if (i == 0)
		{
			new_probability = map->probability;
		}
		else
		{
			new_probability = (1 - map->probability)/2;
		}

		new_action = action + i;

		if (new_action == 8)
		{
			new_action = 0;
		}
		else if (new_action == -1)
		{
			new_action = 7;
		}

		if (new_action > 0 && new_action < 4)
		{
			new_x = x + 1;
		}
		else if (new_action > 4)
		{
			new_x = x - 1;
		}
		else
		{
			new_x = x;
		}

		if (new_action < 2 || new_action > 6)
		{
			new_y = y - 1;
		}
		else if (new_action > 2 && new_action < 6)
		{
			new_y = y + 1;
		}
		else
		{
			new_y = y;
		}

		if (IndexIsValid(new_x, new_y))
		{
			if (map->grid[new_x][new_y].cellType == OBSTRUCTION)
			{
				new_reward = map->collisionPenalty;
				new_x = x;
				new_y = y;
			}
			else
			{
				new_reward = new_x != x && new_y != y ? 1.4 * map->movementPenalty : map->movementPenalty;
			}
		}
		else
		{
			new_reward = new_x != x && new_y != y ? 1.4 * map->movementPenalty : map->movementPenalty;
			new_x = x;
			new_y = y;
		}

		new_v = new_v + new_probability * (new_reward + map->gamma * map->grid[new_x][new_y].value);

	}

	return new_v;
}