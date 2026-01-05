#include "Game.hpp"
#include "Constants.hpp"

int main()
{
    // Create resizable window
    sf::RenderWindow window(sf::VideoMode(gameWidth, gameHeight), "GameOfLife");
    
    // Disable VSync to ensure consistent behavior across different monitors/refresh rates
    window.setVerticalSyncEnabled(false);
    
    Game game(window);
}