#pragma once
#include "Renderer.hpp"
#include "Tile.hpp"
#pragma once
#include "Grid.hpp"
#include "Constants.hpp"
#include "InputManager.hpp"

struct Game 
{
    sf::RenderWindow& window;
    Grid grid;
    InputManager ip;

    Game(sf::RenderWindow& win) : window(win), grid(), ip(grid, window)
	{
        Run();
	}

    void Run()
    {
        Renderer renderer(window, grid);
        
        // Create view that maintains game's logical coordinate system
        // This view will scale to fit the window while maintaining aspect ratio
        sf::View gameView(sf::FloatRect(0, 0, gameWidth, gameHeight));
        updateView(gameView);
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
                    updateView(gameView);
                    window.setView(gameView);
                }
                else
                    ip.handleEvent(event);
            }
            renderer.render(); // general render function which calls all render functions in renderer.hpp
            grid.update(); // updates all tile states with logic applied
        }
    }

private:
    void updateView(sf::View& view)
    {
        // Get current window size
        sf::Vector2u windowSize = window.getSize();
        float windowAspect = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
        float gameAspect = static_cast<float>(gameWidth) / static_cast<float>(gameHeight);
        
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
        view.setCenter(gameWidth / 2.0f, gameHeight / 2.0f);
        view.setSize(static_cast<float>(gameWidth), static_cast<float>(gameHeight));
    }
};

//todo: see if removing the grid-line renderer does anything for optimization
//add option to clear board / reset board
//add clicking and dragging 
//look into threading for massive simulations
//look into color gradients based on screen location
//maybe add a menu / loading screen, and a selection for the buggy version i had without copying, could be cool larger scale
