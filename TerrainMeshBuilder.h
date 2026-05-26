#pragma once
#include <vector>
#include <cstdint>

#pragma pack(push, 1)
struct QuantizedVertex {
  uint16_t x;
  uint16_t y;
  uint16_t z;
};
#pragma pack(pop)

// Quick compile-time static assert to verify our structural packing rules
static_assert(sizeof(QuantizedVertex) == 6, "Structure alignment error: QuantizedVertex must be exactly 6 bytes.");

struct AABB {
  float minX, minY, minZ;
  float maxX, maxY, maxZ;
};

class TerrainMeshBuilder {
public:

  static std::vector<QuantizedVertex> BuildQuantizedMesh(const std::vector<float>& heightData, int gridSize, AABB& outBox) {
    // std::algorithm linear pass over raw data to find elevation limits in O(N)
    auto [min_it, max_it] = std::minmax_element(heightData.begin(), heightData.end());
    float minElev = *min_it;
    float maxElev = *max_it;

    // Calculate dynamic skirt depth directly from raw values
    float heightDelta = maxElev - minElev;
    float skirtDepth = (heightDelta > 0.0f) ? (heightDelta * 0.05f) : 10.0f;

    // 1. Compute the Axis-Aligned Bounding Box (AABB)
    outBox.minX = 0.0f;
    outBox.maxX = 1.0f;
    outBox.minY = minElev - skirtDepth;
    outBox.maxY = maxElev;
    outBox.minZ = 0.0f;
    outBox.maxZ = 1.0f;

    float sizeY = outBox.maxY - outBox.minY;
    if (sizeY == 0.0f) sizeY = 1.0f;

    int totalSurfaceVertices = gridSize * gridSize;
    int perimeterCount = (gridSize - 1) * 4;

    std::vector<QuantizedVertex> quantizedVertices;
    quantizedVertices.reserve(totalSurfaceVertices + perimeterCount);

    // pack values inline with a helper lambda.
    auto packVertex = [&](float x, float y, float z) {
      QuantizedVertex q;
      q.x = static_cast<uint16_t>(x * 65535.0f);
      q.z = static_cast<uint16_t>(z * 65535.0f);
      q.y = static_cast<uint16_t>(((y - outBox.minY) / sizeY) * 65535.0f);
      return q;
    };

    // Pass 1: Surface
    for (int r = 0; r < gridSize; ++r) {
      for (int c = 0; c < gridSize; ++c) {
        float x = static_cast<float>(c) / (gridSize - 1);
        float z = static_cast<float>(r) / (gridSize - 1);
        float y = heightData[r * gridSize + c];
        quantizedVertices.push_back(packVertex(x, y, z));
      }
    }

    // Pass 2: Skirts
    for (int c = 0; c < gridSize - 1; ++c) {
      quantizedVertices.push_back(packVertex(static_cast<float>(c) / (gridSize - 1), heightData[0 * gridSize + c] - skirtDepth, 0.0f));
    }
    for (int r = 0; r < gridSize - 1; ++r) {
      quantizedVertices.push_back(packVertex(1.0f, heightData[r * gridSize + (gridSize - 1)] - skirtDepth, static_cast<float>(r) / (gridSize - 1)));
    }
    for (int c = gridSize - 1; c > 0; --c) {
      quantizedVertices.push_back(packVertex(static_cast<float>(c) / (gridSize - 1), heightData[(gridSize - 1) * gridSize + c] - skirtDepth, 1.0f));
    }
    for (int r = gridSize - 1; r > 0; --r) {
      quantizedVertices.push_back(packVertex(0.0f, heightData[r * gridSize + 0] - skirtDepth, static_cast<float>(r) / (gridSize - 1)));
    }

    return quantizedVertices;
  }
};