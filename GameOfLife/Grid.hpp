#pragma once
#include "Tile.hpp"
#include "Constants.hpp"
#include <vector>
#include <iostream>
#include <random>
#include <future>
#include <thread>

// TODO
// should communicate with game and tile
// should create tile grid in constructor
// should employ conways logic in update, probably
// could have utility fncs like reset, placing your own tiles, saving loading etc

struct Grid
{
	int w;
	int h;
	bool gamePaused = true;
	bool largeMode = false; // Track if in large mode (10x size)

	std::vector<std::vector<Tile>> tiles; // 2d array of tiles, separate from rendered grid of lines
	
	// For dirty tracking: store positions of tiles that changed state
	std::vector<std::pair<int, int>> changedTiles;

	Grid() : w(gameWidth), h(gameHeight)
	{
		generateGridOfDeadTiles();
		setRandomLiveTiles();
	}

	// Get current grid size (tiles per side)
	int getGridSize() const
	{
		return static_cast<int>(tiles.size());
	}

	// Regenerate grid with new size
	void regenerateWithSize(int newSize)
	{
		// Clear existing tiles
		tiles.clear();
		
		// Resize to new dimensions
		tiles.resize(newSize);
		for (size_t i = 0; i < newSize; i++) {
			tiles[i].reserve(newSize);
			for (size_t j = 0; j < newSize; j++) {
				tiles[i].emplace_back(static_cast<int>(i), static_cast<int>(j));
			}
		}
		
		// Update dimensions - use actual tile size (1 for large mode, 7 for normal)
		int effectiveTileSize = largeMode ? 1 : tileSize;
		w = newSize * effectiveTileSize;
		h = newSize * effectiveTileSize;
		
		// Generate random tiles
		setRandomLiveTiles();
	}

	void updateChunk(size_t startRow, size_t endRow, std::vector<std::vector<Tile>>& tilesCopy, std::vector<std::pair<int, int>>& localChangedTiles)
	{
		// Process a chunk of rows - optimized version
		size_t maxRow = tiles.size() - 1;
		size_t maxCol = tiles[0].size() - 1;
		
		for (size_t i = startRow; i < endRow && i < maxRow; i++) {
			for (size_t j = 0; j < maxCol; j++) {
				const Tile& tile = tiles[i][j];
				Tile& newTile = tilesCopy[i][j];

				// Optimized: count neighbors directly without vector allocation
				int numLivingNeighbors = countLivingNeighbors(i, j);
				
				bool wasAlive = tile.isAlive;
				bool willBeAlive = wasAlive;
				
				// Apply the rules of the Game of Life
				if (tile.isAlive) 
				{
					if (numLivingNeighbors < 2 || numLivingNeighbors > 3) 
					{
						newTile.setDead();
						willBeAlive = false;
					}
				}
				else
				{
					if (numLivingNeighbors == 3)
					{
						newTile.setAlive();
						willBeAlive = true;
					}
				}
				
				// Track changed tiles for dirty rendering (only in large mode)
				if (largeMode && wasAlive != willBeAlive)
				{
					localChangedTiles.push_back(std::make_pair(static_cast<int>(i), static_cast<int>(j)));
				}
			}
		}
	}

	void update()
	{
		if (!gamePaused) {
			// copy grid to achieve simultaneous state changes 
			// Optimize: use move semantics and reserve space
			std::vector<std::vector<Tile>> tilesCopy;
			tilesCopy.reserve(tiles.size());
			for (const auto& row : tiles) {
				tilesCopy.emplace_back(row); // Copy row
			}

			// Clear changed tiles list for dirty tracking
			changedTiles.clear();
			if (largeMode) {
				changedTiles.reserve(10000); // Pre-allocate space for changed tiles
			}

			// Use async for better thread management (reuses thread pool)
			size_t totalRows = tiles.size() - 1; // Skip last row
			unsigned int numThreads = std::thread::hardware_concurrency();
			if (numThreads == 0) numThreads = 4;
			// For 10 cores, use all of them but ensure reasonable chunk size
			size_t minChunkSize = 10; // Minimum rows per thread
			if (totalRows / numThreads < minChunkSize) {
				numThreads = static_cast<unsigned int>(totalRows / minChunkSize);
				if (numThreads == 0) numThreads = 1;
			}
			
			size_t rowsPerThread = totalRows / numThreads;
			std::vector<std::future<void>> futures;
			std::vector<std::vector<std::pair<int, int>>> threadChangedTiles(numThreads);
			
			// Launch async tasks for each chunk
			for (unsigned int t = 0; t < numThreads; t++)
			{
				size_t startRow = t * rowsPerThread;
				size_t endRow = (t == numThreads - 1) ? totalRows : (t + 1) * rowsPerThread;
				
				futures.push_back(std::async(std::launch::async, 
					&Grid::updateChunk, this, startRow, endRow, std::ref(tilesCopy), std::ref(threadChangedTiles[t])));
			}
			
			// Wait for all tasks to complete
			for (auto& future : futures)
			{
				future.wait();
			}
			
			// Merge changed tiles from all threads
			if (largeMode) {
				for (const auto& threadChanges : threadChangedTiles) {
					changedTiles.insert(changedTiles.end(), threadChanges.begin(), threadChanges.end());
				}
			}
			
			tiles = tilesCopy; // set all state changes at the same time
		}
	}

	// Optimized: count neighbors directly without vector allocation
	int countLivingNeighbors(size_t i, size_t j)
	{
		int count = 0;
		// Use actual grid size (supports both normal and large mode)
		int gridSize = static_cast<int>(tiles.size());
		
		// iterate over neighboring indices (3x3 neighborhood)
		for (int xOffset = -1; xOffset <= 1; xOffset++)
		{
			for (int yOffset = -1; yOffset <= 1; yOffset++)
			{
				if (xOffset == 0 && yOffset == 0) continue; // skip current tile

				// calculate neighbor indices with wrap-around
				int neighborI = (static_cast<int>(i) + xOffset + gridSize) % gridSize;
				int neighborJ = (static_cast<int>(j) + yOffset + gridSize) % gridSize;

				if (tiles[neighborI][neighborJ].isAlive)
				{
					count++;
				}
			}
		}
		return count;
	}

	void generateGridOfDeadTiles()
	{
		// Use current grid size (supports both normal and large mode)
		int gridSize = largeMode ? (11000 / tileSize) : totalGridTiles; // 10x for large mode
		tiles.resize(gridSize); // set number of rows 

		for (size_t i = 0; i < gridSize; i++) {
			tiles[i].reserve(gridSize); // reserve space for number of columns in each row
			for (size_t j = 0; j < gridSize; j++) {
				tiles[i].emplace_back(static_cast<int>(i), static_cast<int>(j)); // only need Tile arguments because tiles is a vector of tile objects
			}
		}
	}

	void setRandomLiveTiles()
	{
		for (int i = 0; i < 10; i++) {
			// get random starting indices
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, 100);

			int threshold = 60; // % chance to spawn a live tile

			for (int i = 0; i < tiles.size(); i++)
			{
				for (int j = 0; j < tiles[i].size(); j++)
				{
					int val = dis(gen);
					if (val > threshold) 
					{
						tiles[i][j].setAlive();
					}
				}
			}
		}
	}

	void setSymmetricalEdgeTiles()
	{
		// Mirror the original setRandomLiveTiles() exactly, but with 4-way symmetry
		// Generate random pattern in one quadrant, then mirror to all 4 quadrants
		// Run 10 times like the original to create dense pattern
		int gridSize = static_cast<int>(tiles.size());
		for (int iteration = 0; iteration < 10; iteration++) {
			// get random starting indices (new generator each iteration like original)
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, 100);

			int threshold = 60; // Same threshold as original
			int centerX = gridSize / 2;
			int centerY = gridSize / 2;

			// Generate random pattern in the top-left quadrant, then mirror to all 4
			for (int i = 0; i < centerX; i++)
			{
				for (int j = 0; j < centerY; j++)
				{
					int val = dis(gen);
					if (val > threshold)
					{
					// Set tile in all 4 quadrants for symmetry (mirroring the original's full grid generation)
					int gridSize = static_cast<int>(tiles.size());
					tiles[i][j].setAlive();                                    // Top-left
					tiles[gridSize - 1 - i][j].setAlive();              // Top-right (mirror horizontally)
					tiles[i][gridSize - 1 - j].setAlive();               // Bottom-left (mirror vertically)
					tiles[gridSize - 1 - i][gridSize - 1 - j].setAlive(); // Bottom-right (mirror both)
					}
				}
			}
			
			// Handle center lines if grid size is odd (mirror original's full grid approach)
			if (gridSize % 2 == 1)
			{
				// Horizontal center line
				for (int i = 0; i < centerX; i++)
				{
				int val = dis(gen);
				if (val > threshold)
				{
					tiles[i][centerY].setAlive();
					tiles[gridSize - 1 - i][centerY].setAlive();
				}
			}
			
			// Vertical center line
			for (int j = 0; j < centerY; j++)
			{
				int val = dis(gen);
				if (val > threshold)
				{
					tiles[centerX][j].setAlive();
					tiles[centerX][gridSize - 1 - j].setAlive();
				}
				}
				
				// Center tile
				int val = dis(gen);
				if (val > threshold)
				{
					tiles[centerX][centerY].setAlive();
				}
			}
		}
	}

	void reset()
	{
		// Clear all tiles (set them to dead)
		for (size_t i = 0; i < tiles.size(); i++)
		{
			for (size_t j = 0; j < tiles[i].size(); j++)
			{
				tiles[i][j].setDead();
			}
		}
		
		// Reset pause state
		gamePaused = true;
		
		// Generate new random live tiles
		setRandomLiveTiles();
	}

	void resetSymmetrical()
	{
		// Clear all tiles (set them to dead)
		for (size_t i = 0; i < tiles.size(); i++)
		{
			for (size_t j = 0; j < tiles[i].size(); j++)
			{
				tiles[i][j].setDead();
			}
		}
		
		// Reset pause state
		gamePaused = true;
		
		// Generate new symmetrical random tiles on edges only
		setSymmetricalEdgeTiles();
	}

	void setDensityGradientWithStructures()
	{
		// Density gradient: high at edges (70-80%), low at center (20-30%)
		// With embedded oscillators, gliders, and still lifes
		int gridSize = static_cast<int>(tiles.size());
		int centerX = gridSize / 2;
		int centerY = gridSize / 2;
		float maxDistance = std::sqrt(static_cast<float>(centerX * centerX + centerY * centerY));
		
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, 100);
		
		// Generate density gradient
		for (int i = 0; i < gridSize; i++)
		{
			for (int j = 0; j < gridSize; j++)
			{
				// Calculate distance from center
				float dx = static_cast<float>(i) - centerX;
				float dy = static_cast<float>(j) - centerY;
				float distance = std::sqrt(dx * dx + dy * dy);
				float normalizedDist = distance / maxDistance; // 0 = center, 1 = edge
				
				// Density: 80% at edge, 20% at center (linear interpolation)
				float density = 0.8f - (normalizedDist * 0.6f); // 0.8 to 0.2
				density = std::max(0.2f, std::min(0.8f, density)); // Clamp to valid range
				int threshold = static_cast<int>(density * 100);
				
				int val = dis(gen);
				if (val < threshold)
				{
					tiles[i][j].setAlive();
				}
			}
		}
		
		// Embed structured patterns at various distances from center
		// Place oscillators (blinkers - period 2)
		for (int pattern = 0; pattern < 50; pattern++)
		{
			int offsetX = dis(gen) % (gridSize - 10);
			int offsetY = dis(gen) % (gridSize - 10);
			
			// Blinker (horizontal)
			if (offsetX + 2 < gridSize && offsetY < gridSize)
			{
				tiles[offsetX][offsetY].setAlive();
				tiles[offsetX + 1][offsetY].setAlive();
				tiles[offsetX + 2][offsetY].setAlive();
			}
		}
		
		// Place still lifes (blocks - 2x2 squares)
		for (int pattern = 0; pattern < 30; pattern++)
		{
			int offsetX = dis(gen) % (gridSize - 5);
			int offsetY = dis(gen) % (gridSize - 5);
			
			// Block (2x2)
			if (offsetX + 1 < gridSize && offsetY + 1 < gridSize)
			{
				tiles[offsetX][offsetY].setAlive();
				tiles[offsetX + 1][offsetY].setAlive();
				tiles[offsetX][offsetY + 1].setAlive();
				tiles[offsetX + 1][offsetY + 1].setAlive();
			}
		}
		
		// Place gliders (pointing inward from edges)
		for (int pattern = 0; pattern < 40; pattern++)
		{
			int edge = dis(gen) % 4; // Which edge: 0=top, 1=right, 2=bottom, 3=left
			int pos = dis(gen) % (gridSize - 10);
			
			if (edge == 0) // Top edge - glider pointing down-right
			{
				if (pos + 2 < gridSize && 2 < gridSize)
				{
					tiles[1][pos].setAlive();
					tiles[2][pos + 1].setAlive();
					tiles[0][pos + 2].setAlive();
					tiles[1][pos + 2].setAlive();
					tiles[2][pos + 2].setAlive();
				}
			}
			else if (edge == 1) // Right edge - glider pointing down-left
			{
				if (pos + 2 < gridSize && gridSize - 3 >= 0)
				{
					tiles[pos][gridSize - 2].setAlive();
					tiles[pos + 1][gridSize - 3].setAlive();
					tiles[pos + 2][gridSize - 1].setAlive();
					tiles[pos + 2][gridSize - 2].setAlive();
					tiles[pos + 2][gridSize - 3].setAlive();
				}
			}
			else if (edge == 2) // Bottom edge - glider pointing up-right
			{
				if (pos + 2 < gridSize && gridSize - 3 >= 0)
				{
					tiles[gridSize - 2][pos].setAlive();
					tiles[gridSize - 3][pos + 1].setAlive();
					tiles[gridSize - 1][pos + 2].setAlive();
					tiles[gridSize - 2][pos + 2].setAlive();
					tiles[gridSize - 3][pos + 2].setAlive();
				}
			}
			else // Left edge - glider pointing up-right
			{
				if (pos + 2 < gridSize && 2 < gridSize)
				{
					tiles[pos][1].setAlive();
					tiles[pos + 1][2].setAlive();
					tiles[pos + 2][0].setAlive();
					tiles[pos + 2][1].setAlive();
					tiles[pos + 2][2].setAlive();
				}
			}
		}
	}

	void resetDensityGradient()
	{
		// Clear all tiles
		for (size_t i = 0; i < tiles.size(); i++)
		{
			for (size_t j = 0; j < tiles[i].size(); j++)
			{
				tiles[i][j].setDead();
			}
		}
		
		// Reset pause state
		gamePaused = true;
		
		// Generate density gradient with embedded structures
		setDensityGradientWithStructures();
	}

	void placeGliderGun(int startX, int startY, int direction)
	{
		// Gosper Glider Gun pattern
		// Direction: 0=right, 1=down, 2=left, 3=up
		// Pattern is 36x9 cells, fires gliders every 30 generations
		
		int gridSize = static_cast<int>(tiles.size());
		
		// Base glider gun pattern (firing to the right)
		// This is a simplified/compact version of the Gosper glider gun
		int gunPattern[][2] = {
			// Left square
			{0, 4}, {0, 5}, {1, 4}, {1, 5},
			// Right square  
			{10, 4}, {10, 5}, {10, 6}, {11, 3}, {11, 7},
			{12, 2}, {12, 8}, {13, 2}, {13, 8},
			{14, 5}, {15, 3}, {15, 7},
			{16, 4}, {16, 5}, {16, 6}, {17, 5},
			// Left part
			{20, 2}, {20, 3}, {20, 4}, {21, 2}, {21, 3}, {21, 4},
			{22, 1}, {22, 5}, {24, 0}, {24, 1}, {24, 5}, {24, 6},
			// Right part
			{34, 2}, {34, 3}, {35, 2}, {35, 3}
		};
		
		int patternSize = sizeof(gunPattern) / sizeof(gunPattern[0]);
		
		for (int p = 0; p < patternSize; p++)
		{
			int x = gunPattern[p][0];
			int y = gunPattern[p][1];
			int finalX, finalY;
			
			// Rotate pattern based on direction
			if (direction == 0) // Right
			{
				finalX = startX + x;
				finalY = startY + y;
			}
			else if (direction == 1) // Down
			{
				finalX = startX + y;
				finalY = startY + x;
			}
			else if (direction == 2) // Left
			{
				finalX = startX - x;
				finalY = startY + y;
			}
			else // Up
			{
				finalX = startX + y;
				finalY = startY - x;
			}
			
			if (finalX >= 0 && finalX < gridSize && finalY >= 0 && finalY < gridSize)
			{
				tiles[finalY][finalX].setAlive();
			}
		}
	}

	void setGliderGunArrays()
	{
		// Place multiple glider guns at edges firing inward
		int gridSize = static_cast<int>(tiles.size());
		int spacing = 100; // Space between guns
		
		// Top edge - guns firing down
		for (int x = 50; x < gridSize - 50; x += spacing)
		{
			placeGliderGun(x, 10, 1); // direction 1 = down
		}
		
		// Bottom edge - guns firing up
		for (int x = 50; x < gridSize - 50; x += spacing)
		{
			placeGliderGun(x, gridSize - 50, 3); // direction 3 = up
		}
		
		// Left edge - guns firing right
		for (int y = 50; y < gridSize - 50; y += spacing)
		{
			placeGliderGun(10, y, 0); // direction 0 = right
		}
		
		// Right edge - guns firing left
		for (int y = 50; y < gridSize - 50; y += spacing)
		{
			placeGliderGun(gridSize - 50, y, 2); // direction 2 = left
		}
	}

	void resetGliderGunArrays()
	{
		// Clear all tiles
		for (size_t i = 0; i < tiles.size(); i++)
		{
			for (size_t j = 0; j < tiles[i].size(); j++)
			{
				tiles[i][j].setDead();
			}
		}
		
		// Reset pause state
		gamePaused = true;
		
		// Generate glider gun arrays
		setGliderGunArrays();
	}

	void setConcentricDensityRings()
	{
		// Create concentric rings of high density that create wavefronts
		int gridSize = static_cast<int>(tiles.size());
		int centerX = gridSize / 2;
		int centerY = gridSize / 2;
		float maxDistance = std::sqrt(static_cast<float>(centerX * centerX + centerY * centerY));
		
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, 100);
		
		// Create rings at regular intervals
		int numRings = 15; // Number of concentric rings
		float ringSpacing = maxDistance / (numRings + 1);
		
		for (int i = 0; i < gridSize; i++)
		{
			for (int j = 0; j < gridSize; j++)
			{
				// Calculate distance from center
				float dx = static_cast<float>(i) - centerX;
				float dy = static_cast<float>(j) - centerY;
				float distance = std::sqrt(dx * dx + dy * dy);
				
				// Determine which ring this position is in
				int ringIndex = static_cast<int>(distance / ringSpacing);
				float ringPosition = std::fmod(static_cast<float>(distance), static_cast<float>(ringSpacing));
				
				// High density on ring edges (within 2-3 cells of ring boundary)
				float ringThickness = ringSpacing * 0.15f; // 15% of ring spacing is the ring thickness
				bool isOnRing = ringPosition < ringThickness || ringPosition > (ringSpacing - ringThickness);
				
				// Density: 85% on rings, 15% between rings
				int threshold = isOnRing ? 15 : 85; // Lower threshold = higher density
				
				int val = dis(gen);
				if (val < threshold)
				{
					tiles[i][j].setAlive();
				}
			}
		}
	}

	void resetConcentricDensityRings()
	{
		// Clear all tiles
		for (size_t i = 0; i < tiles.size(); i++)
		{
			for (size_t j = 0; j < tiles[i].size(); j++)
			{
				tiles[i][j].setDead();
			}
		}
		
		// Reset pause state
		gamePaused = true;
		
		// Generate concentric density rings
		setConcentricDensityRings();
	}

	void placeRPentomino(int centerX, int centerY)
	{
		// R-pentomino pattern - known to explode and grow chaotically
		// Pattern (relative to center):
		//   X X
		// X X
		//   X
		int gridSize = static_cast<int>(tiles.size());
		
		int pattern[][2] = {
			{0, 0}, {1, 0},  // Top row
			{-1, 1}, {0, 1},  // Middle row
			{0, 2}            // Bottom
		};
		
		int patternSize = sizeof(pattern) / sizeof(pattern[0]);
		
		for (int p = 0; p < patternSize; p++)
		{
			int x = centerX + pattern[p][0];
			int y = centerY + pattern[p][1];
			
			if (x >= 0 && x < gridSize && y >= 0 && y < gridSize)
			{
				tiles[y][x].setAlive();
			}
		}
	}

	void setExplosiveSeeds()
	{
		// Place r-pentomino patterns at strategic locations
		// These explode and create large-scale chaotic growth
		int gridSize = static_cast<int>(tiles.size());
		int centerX = gridSize / 2;
		int centerY = gridSize / 2;
		
		// Place seeds in a grid pattern across the board
		int spacing = 150; // Space between seeds
		int offset = 100;  // Offset from edges
		
		// Create a grid of explosive seeds
		for (int x = offset; x < gridSize - offset; x += spacing)
		{
			for (int y = offset; y < gridSize - offset; y += spacing)
			{
				placeRPentomino(x, y);
			}
		}
		
		// Also place some seeds at edges to create cascading effects
		// Top edge
		for (int x = 50; x < gridSize - 50; x += spacing * 2)
		{
			placeRPentomino(x, 50);
		}
		
		// Bottom edge
		for (int x = 50; x < gridSize - 50; x += spacing * 2)
		{
			placeRPentomino(x, gridSize - 50);
		}
		
		// Left edge
		for (int y = 50; y < gridSize - 50; y += spacing * 2)
		{
			placeRPentomino(50, y);
		}
		
		// Right edge
		for (int y = 50; y < gridSize - 50; y += spacing * 2)
		{
			placeRPentomino(gridSize - 50, y);
		}
		
		// Place a few extra seeds near center for more chaos
		placeRPentomino(centerX, centerY);
		placeRPentomino(centerX + 200, centerY);
		placeRPentomino(centerX - 200, centerY);
		placeRPentomino(centerX, centerY + 200);
		placeRPentomino(centerX, centerY - 200);
	}

	void resetExplosiveSeeds()
	{
		// Clear all tiles
		for (size_t i = 0; i < tiles.size(); i++)
		{
			for (size_t j = 0; j < tiles[i].size(); j++)
			{
				tiles[i][j].setDead();
			}
		}
		
		// Reset pause state
		gamePaused = true;
		
		// Generate explosive seeds
		setExplosiveSeeds();
	}
};