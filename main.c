#include <stdlib.h>
#include <stdio.h>
#include <time.h>
// clang -o main main.c libraylib.a -framework IOKit -framework Cocoa -framework OpenGL
// ./main
#include "raylib.h"
#include "raymath.h"

// Map dimensions
#define COLS 20
#define ROWS 20

// Cell property
typedef enum CellType
{
	OPEN, // Free movement through cell
	GOAL, // Destination
	HOLE, // Should be avoided
    	OBSTRUCTION // Wall
} CellType;

// Contains information about each cell
typedef struct Cell
{
	int x;
	int y; // Position
	CellType cellType; // Cell property
	float value; // Desirability of location in cell
	int action; // Integer represents best direction to move out of cell, clockwise with 0 at top
} Cell;

// Information about map
typedef struct Map
{
	float theta; // Sets ccuracy of estimation and tests for covergence, loop until value of change less than theta or max iterations
	float probability; // Probability of moving to the correct cell, models uncertainty in action
	// Example: If probability = 0.9, and chosen action is to move right, there is a 0.05 chance it moves top right
	// and a 0.05 chance it moves bottom right.
	float gamma; // Discount factor
	Cell grid[COLS][ROWS];
	int max_iterations; // Maximum number of loops
	int movementPenalty; // Cost of movement
	int collisionPenalty; // Cost of colliding with wall
    	int cellWidth;
	int cellHeight;
} Map;

// Draws cell borders, interior colour and value to screen
void CellDraw(Cell*, int, int);
// Draws direction arrow in cell to screen
void DrawDirections(Cell*, int, int);
// Checks that the index is suitable
bool IndexIsValid(int, int);
// Cycles a cell through the different cell types
void ChangeCellType(Cell*);
// Initialises each grid and adds random obstacles
void GridInit(Map*);
// Initialises the map
void MapInit(Map*);
// Value Iteration function calls the two following functions
void ValueIteration(Map*);
// Loops through grid updating cell values
void ComputeValueFunction(Map*);
// Calculates best action to take given surrounding cell values
void ExtractPolicy(Map*);
// Calculates new cell value
float CalculateValue(Map*, int, int, int);

int main()
{
	// Generate random seed
	srand(time(0));

	int screenWidth = 760;
	int screenHeight = 760;

    	Map map;

    	map.cellWidth = screenWidth / COLS;
	map.cellHeight = screenHeight / ROWS;

	// Create window
	InitWindow(screenWidth, screenHeight, "Value Iteration");

	MapInit(&map);
	
	while(!WindowShouldClose())
	{
		// A left mouse click cycles through cell types
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

		// The space bar starts the value iteration process
		if (IsKeyPressed(KEY_SPACE))
		{
			ValueIteration(&map);
		}

		// The R key resets the map
		if (IsKeyPressed(KEY_R))
		{
			MapInit(&map);
		}

		BeginDrawing();

	        ClearBackground(RAYWHITE);

		// Draw each cell in the grid
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

// Draws cell borders, interior colour and value to screen
void CellDraw(Cell *cell, int cellWidth, int cellHeight)
{
	int font = 12;
	if (cell->cellType == OBSTRUCTION) // Obstructions are purple
	{
	DrawRectangle(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, PURPLE);
	}
	else
	{
		if (cell->cellType == GOAL) // Goals are yellow
		{
			DrawRectangle(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, (Color){255, 255, 125, 255 } );
		}
		else if (cell->cellType == HOLE) // Holes are green
		{
			DrawRectangle(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, (Color){55, 125, 100, 255 } );
		}
		else // Open cells are given a value on a gradient depending on their value
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
		// Draw arrows
		DrawDirections(cell, cellWidth, cellHeight);
		// Write value on cell
		DrawText(TextFormat("%0.1f",cell->value), (cell->x + 0.1f) * cellWidth, (cell->y + 0.3f) * cellHeight, font, DARKGRAY);
	}
	// Draw borders
	DrawRectangleLines(cell->x * cellWidth, cell->y * cellHeight, cellWidth, cellHeight, BLACK);
}

// Draws direction arrow in cell to screen
void DrawDirections(Cell *cell, int cellWidth, int cellHeight)
{
	if (cell->action == 8) // Action initially set to 8, no direction
	{
	}
	else
	{
		int x;
		int y;
		Vector2 start;
		Vector2 end;
		if (cell->action == 0) // Point up
		{
			x = cell->x*cellWidth + cellWidth/2;
			y = cell->y*cellHeight + cellHeight/4;
			end = (Vector2){x, y + 0.65*cellHeight};
		}
		else if (cell->action == 1) // Point up and right
		{
			x = cell->x*cellWidth + 3*cellWidth/4;
			y = cell->y*cellHeight + cellHeight/4;
			end = (Vector2){x - 0.53*cellWidth, y + 0.53*cellHeight};
		}
		else if (cell->action == 2) // Point right
		{
			x = cell->x*cellWidth + 3*cellWidth/4;
			y = cell->y*cellHeight + cellHeight/2;
			end = (Vector2){x - 0.65*cellHeight, y};
		}
		else if (cell->action == 3) // Point down and right
		{
			x = cell->x*cellWidth + 3*cellWidth/4;
			y = cell->y*cellHeight + 3*cellHeight/4;
			end = (Vector2){x - 0.53*cellWidth, y - 0.53*cellHeight};
		}
		else if (cell->action == 4) // Point down
		{
			x = cell->x*cellWidth + cellWidth/2;
			y = cell->y*cellHeight + 3*cellHeight/4;
			end = (Vector2){x, y - 0.65*cellHeight};
		}
		else if (cell->action == 5) // Point down and left
		{
			x = cell->x*cellWidth + cellWidth/4;
			y = cell->y*cellHeight + 3*cellHeight/4;
			end = (Vector2){x + 0.53*cellWidth, y - 0.53*cellHeight};
		}
		else if (cell->action == 6) // Point left
		{
			x = cell->x*cellWidth + cellWidth/4;
			y = cell->y*cellHeight + cellHeight/2;
			end = (Vector2){x + 0.65*cellHeight, y};
		}
		else if (cell->action == 7) // Point up and left
		{
			x = cell->x*cellWidth + cellWidth/4;
			y = cell->y*cellHeight + cellHeight/4;
			end = (Vector2){x + 0.53*cellWidth, y + 0.53*cellHeight};
		}
		start = (Vector2){x,y};
		// Draw the arrow
		DrawCircle(x, y, cellWidth/8, RED);
		DrawLineEx(start, end, 2, RED);
	}
}

// Cycles a cell through the different cell types
void ChangeCellType(Cell *cell)
{
    if (cell->cellType == OPEN)
    {
        cell->cellType = GOAL;
	cell->value = 100; // Goals have a high value
    }
    else if (cell->cellType == GOAL)
    {
        cell->cellType = HOLE;
	cell->value = -100; // Holes have a low value
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

// Checks that the index is suitable
bool IndexIsValid(int x, int y)
{
	return x >= 0 && x < COLS && y >= 0 && y < ROWS;
}

// Initialises each grid and adds random obstacles
void GridInit(Map *map)
{
	for (int x = 0; x < COLS; x++)
	{
		for (int y = 0; y < ROWS; y++)
		{
			map->grid[x][y] = (Cell) // Gives initial value to each cell
			{
				.x = x,
				.y = y,
                		.cellType = OPEN,
				.value = 0,
				.action = 8
			};
		}
	}

	// Randomly adds obstacles
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

// Initialises the map
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
	// Value function calculated first
	ComputeValueFunction(map);
	// Then optimal actions are found
	ExtractPolicy(map);
}

// Loops through grid updating cell values
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
		// Sweep systematically over the cells
		for (int x = 0; x < COLS; x++)
		{
			for (int y = 0; y < ROWS; y++)
			{	
				// Skip obstructions, holes and goals
				if (map->grid[x][y].cellType == OPEN)
				{
					// Store the previous value
					old_v = map->grid[x][y].value;
					max_v = 0;
					// Loop over all eight actions
					for (int action = 0; action < 8; action++)
					{
						// Calculate a new value given the action
						new_v = CalculateValue(map, action, x, y);

						// First or highest value stored in max_v
						if (action == 0 || new_v > max_v)
						{
							max_v = new_v;
						}

					}
					
					// New value is the maximum value
					map->grid[x][y].value = max_v;

					// Update the maximum deviation
					if (fabsf(old_v - max_v) > delta)
					{
						delta = fabsf(old_v - max_v);
					}

					// Update the cell on screen
					BeginDrawing();

					CellDraw(&map->grid[x][y], map->cellWidth, map->cellHeight);

					EndDrawing();

				}
			}
		}

		// Increment iteration count
		iterations += 1;

		// Terminate the loop if the change was very small -> convergence
		if (delta < map->theta)
		{
			loop = false;
		}
		
		// Terminate the loop if the maximum number of iterations is met
		if (iterations > map->max_iterations)
		{
			loop = false;
		}
	}
}

// Calculates best action to take given surrounding cell values
void ExtractPolicy(Map *map)
{
	int best_action;
	float old_v;
	float new_v;
	float max_v;
	float delta;
	// Sweep systematically over the cells
	for (int x = 0; x < COLS; x++)
	{
		for (int y = 0; y < ROWS; y++)
		{	
			// Skip obstructions, holes and goals
			if (map->grid[x][y].cellType == OPEN)
			{
				// Store the previous value
				old_v = map->grid[x][y].value;
				max_v = 0;
				// Loop over all eight actions
				for (int action = 0; action < 8; action++)
				{
					// Calculate a new value given the action
					new_v = CalculateValue(map, action, x, y);

					// First or highest value stored in max_v along with corresponding action
					if (action == 0 || new_v > max_v)
					{
						max_v = new_v;
						best_action = action;
					}

				}
				// Best action is the action that corresponds with the maximum value
				map->grid[x][y].action = best_action;
			}
		}
	}
}

// Calculates new cell value
float CalculateValue(Map *map, int action, int x, int y)
{
	float new_probability;
	int new_x;
	int new_y;
	int new_reward;
	int new_action;
	float new_v = 0;

	// Probability that action is diverted to the either side, therefore three actions instead of one
	for (int i = -1; i < 2; i++)
	{
		if (i == 0) // Correct action
		{
			new_probability = map->probability;
		}
		else // Sideways action
		{
			new_probability = (1 - map->probability)/2;
		}

		new_action = action + i;

		// New actions loop around to stay between 0 and 7
		if (new_action == 8)
		{
			new_action = 0;
		}
		else if (new_action == -1)
		{
			new_action = 7;
		}

		// Find new cell position given action
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

		// If the new cell position is in the grid
		if (IndexIsValid(new_x, new_y))
		{
			// If the new cell position is an obstruction give collision penalty to reward and do not change position
			if (map->grid[new_x][new_y].cellType == OBSTRUCTION)
			{
				new_reward = map->collisionPenalty;
				new_x = x;
				new_y = y;
			}
			else
			{
				// If both the x and y position are changed, apply a larger 1.4 * diagonal movement penalty,
				// otherwise apply a normal 1 * movement penalty
				new_reward = new_x != x && new_y != y ? 1.4 * map->movementPenalty : map->movementPenalty;
			}
		}
		else // If the new cell position is off the grid, apply movement penalty to reward but do not change position
		{
			new_reward = new_x != x && new_y != y ? 1.4 * map->movementPenalty : map->movementPenalty;
			new_x = x;
			new_y = y;
		}

		// Collect values from different actions to calculate the new value using the Bellman equation
		new_v = new_v + new_probability * (new_reward + map->gamma * map->grid[new_x][new_y].value);

	}
	
	// Return the new value
	return new_v;
}
