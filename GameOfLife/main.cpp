#include "Game.hpp"
#include "Constants.hpp"
#include <iostream>

void printKeybinds()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "    GAME OF LIFE - KEYBINDS" << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "SPACE  - Pause/Unpause simulation" << std::endl;
	std::cout << "R      - Reset with random pattern" << std::endl;
	std::cout << "G      - Reset with symmetrical pattern" << std::endl;
	std::cout << "C      - Toggle color gradients" << std::endl;
	std::cout << "L      - Toggle large mode (10x size)" << std::endl;
	std::cout << "1      - Density gradient pattern (large mode only)" << std::endl;
	std::cout << "2      - Glider gun arrays (large mode only)" << std::endl;
	std::cout << "3      - Concentric density rings (large mode only)" << std::endl;
	std::cout << "4      - Explosive r-pentomino seeds (large mode only)" << std::endl;
	std::cout << "Mouse  - Click to toggle tiles, drag to place" << std::endl;
	std::cout << "========================================\n" << std::endl;
}

int main()
{
	printKeybinds();
	
	// Create resizable window
	sf::RenderWindow window(sf::VideoMode(gameWidth, gameHeight), "GameOfLife");
	
	// Disable VSync to ensure consistent behavior across different monitors/refresh rates
	window.setVerticalSyncEnabled(false);
	
	Game game(window);
}