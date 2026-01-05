#pragma once
#include <SFML/Graphics.hpp> // be careful, double inclusion leads to bugs without error messages :D
#include "Constants.hpp"
#include "Grid.hpp"
#include <iostream>
#include <cmath>

struct Renderer
{
    sf::RenderWindow& window;
    Grid& grid;

    // Render Window is non-copyable so pass it by reference
    // and use an initialization list before the constructor executes
    Renderer(sf::RenderWindow& win, Grid& grid) : window(win), grid(grid)
    {
    }

    void render()
    {
        renderGrid();
        renderTiles();
        window.display();
    }

    void renderGrid()
    {
        window.clear(sf::Color::Black);

        // Use fixed game dimensions instead of window size to prevent scaling issues
        const int numLinesX = gameWidth / tileSize + 1;
        const int numLinesY = gameHeight / tileSize + 1;
        const int totalVertices = 2 * (numLinesX + numLinesY);

        sf::VertexArray lines(sf::Lines, totalVertices);
        int index = 0;

        // horizontal lines - use fixed gameHeight
        for (int i = 0; i <= gameHeight; i += tileSize)
        {
            lines[index++] = sf::Vertex(sf::Vector2f(0.0f, static_cast<float>(i)), sf::Color(255, 255, 255, 255));
            lines[index++] = sf::Vertex(sf::Vector2f(static_cast<float>(gameWidth), static_cast<float>(i)), sf::Color(255, 255, 255, 255));
        }

        // vertical lines - use fixed gameWidth
        for (int i = 0; i <= gameWidth; i += tileSize)
        {
            lines[index++] = sf::Vertex(sf::Vector2f(static_cast<float>(i), 0.0f), sf::Color(255, 255, 255, 255));
            lines[index++] = sf::Vertex(sf::Vector2f(static_cast<float>(i), static_cast<float>(gameHeight)), sf::Color(255, 255, 255, 255));
        }
        window.draw(lines);
    }

    sf::Color getColorForPosition(int x, int y, bool isAlive)
    {
        if (!isAlive)
        {
            // Dead tiles stay dark
            return sf::Color(0, 0, 0, 255);
        }
        
        // Calculate distance from center for symmetrical radial gradient
        float centerX = static_cast<float>(totalGridTiles) / 2.0f;
        float centerY = static_cast<float>(totalGridTiles) / 2.0f;
        
        float dx = static_cast<float>(x) - centerX;
        float dy = static_cast<float>(y) - centerY;
        float distanceFromCenter = std::sqrt(dx * dx + dy * dy);
        
        // Maximum distance from center (corner)
        float maxDistance = std::sqrt(centerX * centerX + centerY * centerY);
        
        // Normalize distance (0.0 = center/purple, 1.0 = edge/red)
        float normalizedDistance = distanceFromCenter / maxDistance;
        normalizedDistance = std::min(1.0f, std::max(0.0f, normalizedDistance));
        
        // ROYGBIV gradient: Red (edge) -> Orange -> Yellow -> Green -> Blue -> Indigo -> Violet/Purple (center)
        // Map normalizedDistance (0-1) to hue (0-360 degrees), but reversed so red is at edge
        // Hue: 0=Red, 30=Orange, 60=Yellow, 120=Green, 180=Cyan, 240=Blue, 270=Indigo, 300=Violet/Purple
        float hue = (1.0f - normalizedDistance) * 300.0f; // 0 (edge/red) to 300 (center/purple)
        
        // Convert HSV to RGB
        float c = 1.0f; // Chroma (full saturation for bright colors)
        float x_hue = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
        float m = 0.0f; // No lightness adjustment for full brightness
        
        float r, g, b;
        if (hue < 60.0f) {
            // Red to Orange
            r = c; g = x_hue; b = 0.0f;
        } else if (hue < 120.0f) {
            // Orange to Yellow
            r = x_hue; g = c; b = 0.0f;
        } else if (hue < 180.0f) {
            // Yellow to Green
            r = 0.0f; g = c; b = x_hue;
        } else if (hue < 240.0f) {
            // Green to Cyan/Blue
            r = 0.0f; g = x_hue; b = c;
        } else if (hue < 300.0f) {
            // Blue to Indigo to Purple
            r = x_hue; g = 0.0f; b = c;
        } else {
            // Purple/Violet (center)
            r = c; g = 0.0f; b = x_hue;
        }
        
        // Convert to 0-255 range
        int r_int = static_cast<int>((r + m) * 255.0f);
        int g_int = static_cast<int>((g + m) * 255.0f);
        int b_int = static_cast<int>((b + m) * 255.0f);
        
        // Ensure values are in valid range
        r_int = std::min(255, std::max(0, r_int));
        g_int = std::min(255, std::max(0, g_int));
        b_int = std::min(255, std::max(0, b_int));
        
        return sf::Color(r_int, g_int, b_int, 255);
    }

    void renderTiles() 
    {
        // create vector of square objects for batch drawing optimization
        sf::VertexArray squares(sf::Triangles);

        for (size_t i = 0; i < grid.tiles.size(); i++)
        {
            for (size_t j = 0; j < grid.tiles[i].size(); j++)
            {
                // extract tile from vector-based grid
                Tile& tile = grid.tiles[i][j];

                float x = static_cast<float>(tile.x * tileSize);
                float y = static_cast<float>(tile.y * tileSize);

                // Get color based on position for gradient effect
                sf::Color tileColor = getColorForPosition(tile.x, tile.y, tile.isAlive);

                // define vertices of the square with gradient color
                squares.append(sf::Vertex(sf::Vector2f(x, y), tileColor));
                squares.append(sf::Vertex(sf::Vector2f(x + tileSize, y), tileColor));
                squares.append(sf::Vertex(sf::Vector2f(x, y + tileSize), tileColor));

                squares.append(sf::Vertex(sf::Vector2f(x + tileSize, y), tileColor));
                squares.append(sf::Vertex(sf::Vector2f(x + tileSize, y + tileSize), tileColor));
                squares.append(sf::Vertex(sf::Vector2f(x, y + tileSize), tileColor));
            }
        }
        // draw square on line-based grid based on position in vector-based grid
        window.draw(squares);
    }
};