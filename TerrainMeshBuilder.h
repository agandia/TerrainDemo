#pragma once
#include <vector>
#include <cstdint>
#include <iostream>

struct Vertex3D {
  float x, y, z;
};

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
  static const int GRID_SIZE = 33; // 33x33 vertices

  // Generates a single, reusable index buffer for a regular grid + skirts
  static std::vector<uint32_t> GenerateGlobalIndices() {
    std::vector<uint32_t> indices;
    indices.reserve((GRID_SIZE - 1) * (GRID_SIZE - 1) * 6 + (GRID_SIZE - 1) * 4 * 6);

    // 1. Generate core surface indices (Triangle List layout)
    for (int r = 0; r < GRID_SIZE - 1; ++r) {
      for (int c = 0; c < GRID_SIZE - 1; ++c) {
        uint32_t topLeft = r * GRID_SIZE + c;
        uint32_t topRight = topLeft + 1;
        uint32_t bottomLeft = (r + 1) * GRID_SIZE + c;
        uint32_t bottomRight = bottomLeft + 1;

        // Triangle 1
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);

        // Triangle 2
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
      }
    }

    // 2. Generate Skirt Indices
    // Collect the perimeter vertex indices in a clean, sequential loop
    std::vector<uint32_t> perimeterIndices;

    // Top edge (Left to Right)
    for (int c = 0; c < GRID_SIZE - 1; ++c) perimeterIndices.push_back(0 * GRID_SIZE + c);
    // Right edge (Top to Bottom)
    for (int r = 0; r < GRID_SIZE - 1; ++r) perimeterIndices.push_back(r * GRID_SIZE + (GRID_SIZE - 1));
    // Bottom edge (Right to Left)
    for (int c = GRID_SIZE - 1; c > 0; --c) perimeterIndices.push_back((GRID_SIZE - 1) * GRID_SIZE + c);
    // Left edge (Bottom to Top)
    for (int r = GRID_SIZE - 1; r > 0; --r) perimeterIndices.push_back(r * GRID_SIZE + 0);

    uint32_t perimeterCount = static_cast<uint32_t>(perimeterIndices.size()); // Exactly 128
    uint32_t skirtVertexOffsetStart = GRID_SIZE * GRID_SIZE; // 1089

    // Stitch quads connecting the top edge loop to the dropped edge loop
    for (uint32_t i = 0; i < perimeterCount; ++i) {
      uint32_t nextI = (i + 1) % perimeterCount;

      uint32_t topCurrent = perimeterIndices[i];
      uint32_t topNext = perimeterIndices[nextI];

      uint32_t bottomCurrent = skirtVertexOffsetStart + i;
      uint32_t bottomNext = skirtVertexOffsetStart + nextI;

      // Quad Triangle 1
      indices.push_back(topCurrent);
      indices.push_back(bottomCurrent);
      indices.push_back(topNext);

      // Quad Triangle 2
      indices.push_back(topNext);
      indices.push_back(bottomCurrent);
      indices.push_back(bottomNext);
    }

    return indices;
  }

  // Generates vertices for the tile, including extruded vertical boundary skirts
  static std::vector<Vertex3D> GenerateMeshWithSkirts(const std::vector<float>& heightData, float skirtDepth) {
    std::vector<Vertex3D> vertices;
    vertices.reserve(GRID_SIZE * GRID_SIZE + 128);

    // 1. Populate standard 33x33 surface vertices
    for (int r = 0; r < GRID_SIZE; ++r) {
      for (int c = 0; c < GRID_SIZE; ++c) {
        Vertex3D v;
        v.x = static_cast<float>(c) / (GRID_SIZE - 1);
        v.z = static_cast<float>(r) / (GRID_SIZE - 1);
        v.y = heightData[r * GRID_SIZE + c];
        vertices.push_back(v);
      }
    }

    // 2. Append duplicated skirt vertices pushed downward
    // Use the exact same tracking logic order as index generation to keep memory aligned

    // Top edge
    for (int c = 0; c < GRID_SIZE - 1; ++c) {
      Vertex3D v = vertices[0 * GRID_SIZE + c]; v.y -= skirtDepth; vertices.push_back(v);
    }
    // Right edge
    for (int r = 0; r < GRID_SIZE - 1; ++r) {
      Vertex3D v = vertices[r * GRID_SIZE + (GRID_SIZE - 1)]; v.y -= skirtDepth; vertices.push_back(v);
    }
    // Bottom edge
    for (int c = GRID_SIZE - 1; c > 0; --c) {
      Vertex3D v = vertices[(GRID_SIZE - 1) * GRID_SIZE + c]; v.y -= skirtDepth; vertices.push_back(v);
    }
    // Left edge
    for (int r = GRID_SIZE - 1; r > 0; --r) {
      Vertex3D v = vertices[r * GRID_SIZE + 0]; v.y -= skirtDepth; vertices.push_back(v);
    }

    return vertices;
  }

  static std::vector<QuantizedVertex> QuantizeMesh(const std::vector<Vertex3D>& rawVertices, AABB& outBox) {
    std::vector<QuantizedVertex> quantized;
    quantized.reserve(rawVertices.size());

    if (rawVertices.empty()) return quantized;

    // 1. Compute the Axis-Aligned Bounding Box (AABB)
    outBox.minX = outBox.maxX = rawVertices[0].x;
    outBox.minY = outBox.maxY = rawVertices[0].y;
    outBox.minZ = outBox.maxZ = rawVertices[0].z;

    for (const auto& v : rawVertices) {
      if (v.x < outBox.minX) outBox.minX = v.x; if (v.x > outBox.maxX) outBox.maxX = v.x;
      if (v.y < outBox.minY) outBox.minY = v.y; if (v.y > outBox.maxY) outBox.maxY = v.y;
      if (v.z < outBox.minZ) outBox.minZ = v.z; if (v.z > outBox.maxZ) outBox.maxZ = v.z;
    }

    // 2. Quantize each vertex relative to the bounding box dimensions
    float sizeX = outBox.maxX - outBox.minX;
    float sizeY = outBox.maxY - outBox.minY;
    float sizeZ = outBox.maxZ - outBox.minZ;

    // Prevent division by zero for flat test planes
    if (sizeX == 0.0f) sizeX = 1.0f;
    if (sizeY == 0.0f) sizeY = 1.0f;
    if (sizeZ == 0.0f) sizeZ = 1.0f;

    for (const auto& v : rawVertices) {
      QuantizedVertex q;

      // Normalize to [0.0 ... 1.0], scale to 65535, and cast down safely
      q.x = static_cast<uint16_t>(((v.x - outBox.minX) / sizeX) * 65535.0f);
      q.y = static_cast<uint16_t>(((v.y - outBox.minY) / sizeY) * 65535.0f);
      q.z = static_cast<uint16_t>(((v.z - outBox.minZ) / sizeZ) * 65535.0f);

      quantized.push_back(q);
    }

    return quantized;
  }
};