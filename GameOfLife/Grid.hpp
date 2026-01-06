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
		
		// iterate over neighboring indices
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
};