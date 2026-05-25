#pragma once
#include <cstdint>

#pragma pack(push, 1)

// Fixed-size header for the entire file archive
struct ArchiveHeader {
  char magic[4] = { 'T', 'E', 'R', 'R' };
  uint32_t totalTiles;
};

// Fixed-size descriptor entry for every single tile in the file
struct TileDescriptor {
  uint32_t lodLevel;
  uint32_t tileX;
  uint32_t tileY;
  uint32_t vertexCount;
  uint64_t byteOffset;   // Where the QuantizedVertex array starts in the file

  // Bounds for frustum culling and un-quantization calculations
  float minX, minY, minZ;
  float maxX, maxY, maxZ;
};

#pragma pack(pop)