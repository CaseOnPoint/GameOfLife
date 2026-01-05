#pragma once
#include <SFML/Graphics.hpp>
#include "Grid.hpp"

struct InputManager
{
	Grid& grid;
	sf::RenderWindow& window;

	InputManager(Grid& grid, sf::RenderWindow& win) : grid(grid), window(win)
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
			}
		}
		
		else if (event.type == sf::Event::KeyPressed)
		{
			handleKeyPress(event.key.code);
		}
			
	}

	void handleMouseClick(int mouseX, int mouseY)
	{
		// calc cell indices based on mouse coords in view space
		int rowIdx = mouseX / tileSize;
		int colIdx = mouseY / tileSize;

		// Clamp to valid grid bounds to prevent out-of-range access
		if (rowIdx >= 0 && rowIdx < static_cast<int>(grid.tiles.size()) &&
		    colIdx >= 0 && colIdx < static_cast<int>(grid.tiles[rowIdx].size()))
		{
			grid.tiles[rowIdx][colIdx].toggleState();
		}
	}

	void handleKeyPress(sf::Keyboard::Key keyCode)
	{
		switch (keyCode)
		{
			// pause & unpause the grid update 
			case sf::Keyboard::Space:
				grid.gamePaused = !grid.gamePaused;
				break;
			// restart/reset the simulation with random pattern
			case sf::Keyboard::R:
				grid.reset();
				break;
			// restart/reset the simulation with symmetrical pattern
			case sf::Keyboard::G:
				grid.resetSymmetrical();
				break;
		}
	}
};