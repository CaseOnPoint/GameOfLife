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

	std::vector<std::vector<Tile>> tiles; // 2d array of tiles, separate from rendered grid of lines

	Grid() : w(gameWidth), h(gameHeight)
	{
		generateGridOfDeadTiles();
		setRandomLiveTiles();
	}

	void updateChunk(size_t startRow, size_t endRow, std::vector<std::vector<Tile>>& tilesCopy)
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
				
				// Apply the rules of the Game of Life
				if (tile.isAlive) 
				{
					if (numLivingNeighbors < 2 || numLivingNeighbors > 3) 
					{
						newTile.setDead();
					}
				}
				else
				{
					if (numLivingNeighbors == 3)
					{
						newTile.setAlive();
					}
				}
			}
		}
	}

	void update()
	{
		if (!gamePaused) {
			// copy grid to achieve simultaneous state changes 
			std::vector<std::vector<Tile>> tilesCopy = tiles;

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
			
			// Launch async tasks for each chunk
			for (unsigned int t = 0; t < numThreads; t++)
			{
				size_t startRow = t * rowsPerThread;
				size_t endRow = (t == numThreads - 1) ? totalRows : (t + 1) * rowsPerThread;
				
				futures.push_back(std::async(std::launch::async, 
					&Grid::updateChunk, this, startRow, endRow, std::ref(tilesCopy)));
			}
			
			// Wait for all tasks to complete
			for (auto& future : futures)
			{
				future.wait();
			}
			
			tiles = tilesCopy; // set all state changes at the same time
		}
	}

	// Optimized: count neighbors directly without vector allocation
	int countLivingNeighbors(size_t i, size_t j)
	{
		int count = 0;
		// iterate over neighboring indices
		for (int xOffset = -1; xOffset <= 1; xOffset++)
		{
			for (int yOffset = -1; yOffset <= 1; yOffset++)
			{
				if (xOffset == 0 && yOffset == 0) continue; // skip current tile

				// calculate neighbor indices with wrap-around
				int neighborI = (static_cast<int>(i) + xOffset + totalGridTiles) % (totalGridTiles);
				int neighborJ = (static_cast<int>(j) + yOffset + totalGridTiles) % (totalGridTiles);

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
		tiles.resize(totalGridTiles); // set number of rows 

		for (size_t i = 0; i < totalGridTiles; i++) {
			tiles[i].reserve(totalGridTiles); // reserve space for number of columns in each row
			for (size_t j = 0; j < totalGridTiles; j++) {
				tiles[i].emplace_back(i, j); // only need Tile arguments because tiles is a vector of tile objects
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
		for (int iteration = 0; iteration < 10; iteration++) {
			// get random starting indices (new generator each iteration like original)
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, 100);

			int threshold = 60; // Same threshold as original
			int centerX = totalGridTiles / 2;
			int centerY = totalGridTiles / 2;

			// Generate random pattern in the top-left quadrant, then mirror to all 4
			for (int i = 0; i < centerX; i++)
			{
				for (int j = 0; j < centerY; j++)
				{
					int val = dis(gen);
					if (val > threshold)
					{
						// Set tile in all 4 quadrants for symmetry (mirroring the original's full grid generation)
						tiles[i][j].setAlive();                                    // Top-left
						tiles[totalGridTiles - 1 - i][j].setAlive();              // Top-right (mirror horizontally)
						tiles[i][totalGridTiles - 1 - j].setAlive();               // Bottom-left (mirror vertically)
						tiles[totalGridTiles - 1 - i][totalGridTiles - 1 - j].setAlive(); // Bottom-right (mirror both)
					}
				}
			}
			
			// Handle center lines if grid size is odd (mirror original's full grid approach)
			if (totalGridTiles % 2 == 1)
			{
				// Horizontal center line
				for (int i = 0; i < centerX; i++)
				{
					int val = dis(gen);
					if (val > threshold)
					{
						tiles[i][centerY].setAlive();
						tiles[totalGridTiles - 1 - i][centerY].setAlive();
					}
				}
				
				// Vertical center line
				for (int j = 0; j < centerY; j++)
				{
					int val = dis(gen);
					if (val > threshold)
					{
						tiles[centerX][j].setAlive();
						tiles[centerX][totalGridTiles - 1 - j].setAlive();
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
};