#include "TiledWorldGenerator.h"
#include "Tile.h"
#include "imgui_internal.h"
#include <iostream>
#include <algorithm>
#include <vector>

const float WindowBuffer = 5.0f;
const float CellBorder = 1.0f;

void TiledWorldGenerator::Generate()
{
	// perform the world generation
	NormaliseProbabilities();
	ClearWorld();
	GenerateWorld();
}

/*
Node (variables)
 - Bounding Box (AABB)
 - Layer/level/depth
 - Parent pointer
 - Children
 - Contents

Node (functions)
 - Add a relevant object
 - Find potentially relevant tiles

Process (for creating)
 - Create the root node
 - Add all of the relevant objects to the root node
 - Inside "Add a relevant object" on the node:
   - If we already have children:
     - Find the children that the object overlaps
	   - Call "Add a relevant object" on those children
   - Otherwise:
     - Add the relevant object to our contents
	 - If number of objects being tracked over threshold AND our size is > some threshold
	   - Split
	     - Create the children (and track them)
		 - Distribute all relevant objects to the children

Process (for using)
 - Inputs
   - Location
 - Output
   - List of potentially relevant tiles
 - What it actually does?
   - Runs "Find potentially relevant tiles" on the root node
   - If we have no children
     - Return the contents
   - If we have children
     - Find which one the point is in
	 - Run "Find potential relevant tiles" on that child and return the result
*/

void TiledWorldGenerator::CalculateField()
{
	largestFieldStrength = 0;

	AABBf worldBounds = AABBf(Vector2f::Zero, Vector2f(Length, Width));

	// TODO: build tree here
	rootNode = new Node(worldBounds.boxMin, worldBounds.boxMax, nullptr, 0);

	for (auto tile : world)
	{
		rootNode->AddObject(tile);
	}
	
	// iterate over the tiles and calculate their field
	for (Tile* currentTilePtr : world)
	{
		// reset the field
		currentTilePtr->LocalFieldValue = Vector2f::Zero;

		Vector2f boxSize = Vector2f(currentTilePtr->FieldRange, currentTilePtr->FieldRange);
		AABBf boundingBox(currentTilePtr->Location - boxSize, 
			              currentTilePtr->Location + boxSize);

		// is this an obstacle? if so do nothing
		if (currentTilePtr->Type == ettObstructed)
			continue;

		// TODO: "world" below changes to result we get back from asking for relevant tiles

		// iterate over every other tile and add their contribution to the field

		//for(Tile* otherTilePtr : world)
		for (Tile* otherTilePtr : rootNode->FindTiles(currentTilePtr->Location))
		{
			// skip this tile
			if (otherTilePtr == currentTilePtr)
				continue;

			currentTilePtr->LocalFieldValue += otherTilePtr->CalculateFieldTo(currentTilePtr);
		}

		// track the largest field strength
		float fieldStrength = currentTilePtr->LocalFieldValue.Magnitude();
		if (fieldStrength > largestFieldStrength)
			largestFieldStrength = fieldStrength;
	}
}

void TiledWorldGenerator::DrawWorld()
{
	// early out if there is no world
	if (world.size() == 0)
		return;

	// grab the window
    ImGuiWindow* window = ImGui::GetCurrentWindowRead();
	
	// determine the cell size
	ImVec2 windowSize = ImGui::GetWindowSize();
	int cellSize = (int) std::min((windowSize.x - (WindowBuffer * 2)) / Length, (windowSize.y - window->TitleBarHeight() - (WindowBuffer * 2)) / Width);

	// get the draw list to update
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// get the window pos
	ImVec2 startPoint = ImGui::GetWindowPos();
	startPoint.x += WindowBuffer;
	startPoint.y += window->TitleBarHeight() + WindowBuffer;

	// draw the tiles
	for(Tile* tilePtr : world)
	{
		// calculate the tile location
		ImVec2 location = ImVec2((tilePtr->Location.X * cellSize) + startPoint.x, (tilePtr->Location.Y * cellSize) + startPoint.y);
		ImColor workingColour = tilePtr->Colour;

		// add the cell bounds
		//drawList->AddRect(location, ImVec2(location.x + cellSize, location.y + cellSize), 0xFFFFFFFF);

		// normalise the field
		if (ShowField && largestFieldStrength > 0)
		{
			Vector2f localField = tilePtr->LocalFieldValue.Normalised();// / largestFieldStrength;
			workingColour = ImColor(0.5f + (localField.X / 2.0f), 
									0.5f + (localField.Y / 2.0f), 
									0.0f);
		}

		// draw the cell
		drawList->AddRectFilled(ImVec2(location.x + CellBorder, location.y + CellBorder), 
						        ImVec2(location.x + cellSize - CellBorder*2, location.y + cellSize - CellBorder*2),
								workingColour);
	}

	////////////////////////////////////////////////////////////////////////////////
	// TODO: Add any debug drawing here. You can use drawList to draw lines etc
	////////////////////////////////////////////////////////////////////////////////
}

void TiledWorldGenerator::NormaliseProbabilities()
{
	// sum all of the tile frequencies
	int frequencySum = 0;
	for(AvailableTile* tilePtr : TilePalette)
	{
		frequencySum += tilePtr->Frequency;
	}

	// set the overall probability thresholds
	float currentThreshold = 0;
	for (AvailableTile* tilePtr : TilePalette)
	{
		currentThreshold += (float)tilePtr->Frequency / (float)frequencySum;
		tilePtr->Threshold = currentThreshold;
	}
}

void TiledWorldGenerator::ClearWorld()
{
	// cleanup the world
	for (Tile* tilePtr : world)
	{
		delete tilePtr;
	}
	world.clear();
}

void TiledWorldGenerator::GenerateWorld()
{
	// tile palette is empty so early out
	if (TilePalette.size() == 0)
		return;

	// reserve space for the world
	world.reserve(Length * Width);

	// generate the world
	for (int lengthIndex = 0; lengthIndex < Length; ++lengthIndex)
	{
		for (int widthIndex = 0; widthIndex < Width; ++widthIndex)
		{
			// roll a random number from 0 to 1
			float roll = (float)(rand() % 101) / 100.0f;

			// select matching reference tile (default is pure random)
			AvailableTile* referenceTilePtr = nullptr;
			for(AvailableTile* tilePtr : TilePalette)
			{
				if (roll <= tilePtr->Threshold)
				{
					referenceTilePtr = tilePtr;
					break;
				}
			}
			if (!referenceTilePtr)
				referenceTilePtr = TilePalette[rand() % TilePalette.size()];
			 
			// instantiate the new tile
			world.push_back(new Tile(referenceTilePtr->Type, referenceTilePtr->Colour, 
									 Vector2f((float)lengthIndex, (float)widthIndex), 
									 referenceTilePtr->FieldStrength, referenceTilePtr->FieldRange));
		}
	}

	
}

std::vector<Tile*> TiledWorldGenerator::ReturnSelectedNode(Vector2f _target)
{
	return rootNode->FindTiles(_target);
}

