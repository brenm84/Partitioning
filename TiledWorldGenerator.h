#pragma once

#include <vector>
#include <string>
#include "imgui.h"
#include "Tile.h"
#include "Node.h"

class AvailableTile
{
    public:
        int Frequency;
        float Threshold;
        std::string Name;
        ImColor Colour;
        TileType Type;
        float FieldStrength;
        float FieldRange;
		

        AvailableTile(int _frequency, const std::string& _name, const ImColor& _colour, TileType _type, float _fieldStrength, float _fieldRange) :
            Frequency(_frequency), Name(_name), Colour(_colour), Type(_type), FieldStrength(_fieldStrength), FieldRange(_fieldRange)
        {

        }
};

class TiledWorldGenerator
{
    public:
        int Length;
        int Width;
        std::vector<AvailableTile*> TilePalette;
		Node *rootNode;

        TiledWorldGenerator() :
            Length(120), Width(120)
        {
            TilePalette.push_back(new AvailableTile(85, "Free", ImColor(121, 255, 116), ettFree, 0, 0));
            TilePalette.push_back(new AvailableTile(10, "Obstructed", ImColor(81, 0, 0), ettObstructed, 4, 5));
            TilePalette.push_back(new AvailableTile(4, "Undesirable", ImColor(255, 127, 39), ettUndesirable, 3, 10));
            TilePalette.push_back(new AvailableTile(1, "Desirable", ImColor(0, 81, 0), ettDesirable, -10, 60));
        }

        ~TiledWorldGenerator()
        {
            // cleanup the tile palette
            for (AvailableTile* tilePtr : TilePalette)
            {
                delete tilePtr;
            }
            TilePalette.clear();

            ClearWorld();
        }

        void Generate();

        void CalculateField();

        void DrawWorld();

		std::vector<Tile*> ReturnSelectedNode(Vector2f);

    protected:
	    void NormaliseProbabilities();
	    void ClearWorld();
	    void GenerateWorld();

    protected:
        std::vector<Tile*> world;
        float largestFieldStrength;

    public:
        bool ShowField = false;
};
