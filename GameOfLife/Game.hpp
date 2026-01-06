#pragma once
#include "Renderer.hpp"
#include "Tile.hpp"
#include "Grid.hpp"
#include "Constants.hpp"
#include "InputManager.hpp"

struct Game 
{
    sf::RenderWindow& window;
    Grid grid;
    InputManager ip;
    Renderer* renderer; // Need to store reference for InputManager

    Game(sf::RenderWindow& win) : window(win), grid(), ip(grid, window, *this)
	{
        Run();
	}

    void Run()
    {
        Renderer rendererInstance(window, grid);
        renderer = &rendererInstance; // Store pointer for InputManager access
        
        // Create view that maintains game's logical coordinate system
        // Window size stays constant (1100x1100), tiles scale down in large mode
        sf::View gameView(sf::FloatRect(0, 0, static_cast<float>(gameWidth), static_cast<float>(gameHeight)));
        updateView(gameView, gameWidth, gameHeight);
        window.setView(gameView);

        // run the main loop
        while (window.isOpen())
        {
            // handle events
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.close();
                else if (event.type == sf::Event::Resized)
                {
                    // Update view to maintain aspect ratio when window is resized
                    // Window size stays constant, so use gameWidth/gameHeight
                    updateView(gameView, gameWidth, gameHeight);
                    window.setView(gameView);
                }
                else
                    ip.handleEvent(event);
            }
            
            // View stays constant - window size doesn't change, only tile size changes
            
            rendererInstance.render(); // general render function which calls all render functions in renderer.hpp
            grid.update(); // updates all tile states with logic applied
        }
    }

    void toggleColorGradient()
    {
        if (renderer)
        {
            renderer->toggleColorGradient();
        }
    }

    void toggleLargeMode()
    {
        grid.largeMode = !grid.largeMode;
        
        if (grid.largeMode)
        {
            // Switch to large mode: 10x more tiles, but keep window same size
            // Use tileSize=1 to fit 1100x1100 tiles in 1100x1100 window (about 7x more tiles)
            grid.regenerateWithSize(gameWidth); // 1100 tiles (was 157), tileSize will be 1
        }
        else
        {
            // Switch back to normal mode
            grid.regenerateWithSize(totalGridTiles);
        }
        
        // Reset pause state
        grid.gamePaused = true;
    }

private:
    void updateView(sf::View& view, int viewWidth, int viewHeight)
    {
        // Get current window size
        sf::Vector2u windowSize = window.getSize();
        float windowAspect = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
        float gameAspect = static_cast<float>(viewWidth) / static_cast<float>(viewHeight);
        
        // Calculate viewport to maintain aspect ratio (letterboxing/pillarboxing)
        sf::FloatRect viewport(0.0f, 0.0f, 1.0f, 1.0f);
        
        if (windowAspect > gameAspect)
        {
            // Window is wider - add pillarboxing (black bars on sides)
            viewport.width = gameAspect / windowAspect;
            viewport.left = (1.0f - viewport.width) / 2.0f;
        }
        else
        {
            // Window is taller - add letterboxing (black bars on top/bottom)
            viewport.height = windowAspect / gameAspect;
            viewport.top = (1.0f - viewport.height) / 2.0f;
        }
        
        view.setViewport(viewport);
        view.setCenter(viewWidth / 2.0f, viewHeight / 2.0f);
        view.setSize(static_cast<float>(viewWidth), static_cast<float>(viewHeight));
    }
};

// Define InputManager::handleKeyPress here after Game is fully defined
inline void InputManager::handleKeyPress(sf::Keyboard::Key keyCode)
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
		// toggle color gradients on/off
		case sf::Keyboard::C:
			game.toggleColorGradient();
			break;
		// toggle large mode (10x size)
		case sf::Keyboard::L:
			game.toggleLargeMode();
			break;
		// density gradient with structures (large mode only)
		case sf::Keyboard::Num1:
			if (grid.largeMode)
			{
				grid.resetDensityGradient();
			}
			break;
	}
}

//todo: see if removing the grid-line renderer does anything for optimization
//add option to clear board / reset board
//add clicking and dragging 
//look into threading for massive simulations
//look into color gradients based on screen location
//maybe add a menu / loading screen, and a selection for the buggy version i had without copying, could be cool larger scale
