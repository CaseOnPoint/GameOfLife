#pragma once
#include <SFML/Graphics.hpp>
#include "Grid.hpp"

// Forward declaration to avoid circular dependency
struct Game;
struct Renderer;

struct InputManager
{
	Grid& grid;
	sf::RenderWindow& window;
	Game& game; // Need reference to Game for large mode toggle
	bool isDragging = false; // Track if mouse is being dragged
	int lastDragX = -1; // Last drag position X (to avoid placing same tile multiple times)
	int lastDragY = -1; // Last drag position Y

	InputManager(Grid& grid, sf::RenderWindow& win, Game& gameRef) : grid(grid), window(win), game(gameRef)
	{
	}

	void handleEvent(const sf::Event& event)
	{
		if (event.type == sf::Event::MouseButtonPressed)
		{
			if (event.mouseButton.button == sf::Mouse::Left)
			{
				// Convert screen coordinates to view coordinates using the window's current view
				sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
				handleMouseClick(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));
				isDragging = true;
				lastDragX = -1; // Reset last drag position
				lastDragY = -1;
			}
		}
		else if (event.type == sf::Event::MouseButtonReleased)
		{
			if (event.mouseButton.button == sf::Mouse::Left)
			{
				isDragging = false;
				lastDragX = -1;
				lastDragY = -1;
			}
		}
		else if (event.type == sf::Event::MouseMoved)
		{
			if (isDragging)
			{
				// Convert screen coordinates to view coordinates
				sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
				handleMouseDrag(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));
			}
		}
		else if (event.type == sf::Event::KeyPressed)
		{
			handleKeyPress(event.key.code);
		}
			
	}

	void placeTilesAt(int centerRow, int centerCol, bool toggle = false)
	{
		// In large mode, place a 7x7 area (like normal mode tile size)
		// In normal mode, just place the single tile
		int brushSize = grid.largeMode ? tileSize : 1;
		int halfBrush = brushSize / 2;
		
		int gridSize = static_cast<int>(grid.tiles.size());
		
		for (int i = -halfBrush; i <= halfBrush; i++)
		{
			for (int j = -halfBrush; j <= halfBrush; j++)
			{
				int rowIdx = centerRow + i;
				int colIdx = centerCol + j;
				
				// Clamp to valid grid bounds
				if (rowIdx >= 0 && rowIdx < gridSize &&
				    colIdx >= 0 && colIdx < gridSize)
				{
					if (toggle)
					{
						grid.tiles[rowIdx][colIdx].toggleState();
					}
					else
					{
						grid.tiles[rowIdx][colIdx].setAlive();
					}
				}
			}
		}
	}

	void handleMouseClick(int mouseX, int mouseY)
	{
		// Get effective tile size (1 for large mode, 7 for normal)
		int effectiveTileSize = grid.largeMode ? 1 : tileSize;
		
		// calc cell indices based on mouse coords in view space
		// mouseX maps to column (j), mouseY maps to row (i)
		// grid.tiles[i][j] where i=row (y), j=col (x)
		int colIdx = mouseX / effectiveTileSize;  // X coordinate = column
		int rowIdx = mouseY / effectiveTileSize;   // Y coordinate = row

		// Clamp to valid grid bounds to prevent out-of-range access
		if (rowIdx >= 0 && rowIdx < static_cast<int>(grid.tiles.size()) &&
		    colIdx >= 0 && colIdx < static_cast<int>(grid.tiles[rowIdx].size()))
		{
			// Place tiles (7x7 area in large mode, single tile in normal mode)
			placeTilesAt(rowIdx, colIdx, true); // toggle mode for click
			// Store position for drag tracking
			lastDragX = rowIdx;
			lastDragY = colIdx;
		}
	}

	void handleMouseDrag(int mouseX, int mouseY)
	{
		// Get effective tile size (1 for large mode, 7 for normal)
		int effectiveTileSize = grid.largeMode ? 1 : tileSize;
		
		// calc cell indices based on mouse coords in view space
		// mouseX maps to column (j), mouseY maps to row (i)
		// grid.tiles[i][j] where i=row (y), j=col (x)
		int colIdx = mouseX / effectiveTileSize;  // X coordinate = column
		int rowIdx = mouseY / effectiveTileSize;   // Y coordinate = row

		// Clamp to valid grid bounds
		if (rowIdx >= 0 && rowIdx < static_cast<int>(grid.tiles.size()) &&
		    colIdx >= 0 && colIdx < static_cast<int>(grid.tiles[rowIdx].size()))
		{
			// Only place tiles if we've moved to a different cell (avoid redundant placements)
			if (rowIdx != lastDragX || colIdx != lastDragY)
			{
				// Place tiles (7x7 area in large mode, single tile in normal mode)
				placeTilesAt(rowIdx, colIdx, false); // place mode for drag
				lastDragX = rowIdx;
				lastDragY = colIdx;
			}
		}
	}

	void handleKeyPress(sf::Keyboard::Key keyCode);
};