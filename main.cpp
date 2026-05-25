#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include "TerrainMeshBuilder.h"

namespace fs = std::filesystem;

int main() {
  std::string filepath = "../data_bins/lod_6_33x33.bin";
  fs::path absolute_target = fs::absolute(filepath);

  if (!fs::exists(absolute_target)) return 1;

  std::ifstream file(absolute_target, std::ios::binary | std::ios::ate);
  std::streamsize fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<float> heights(TerrainMeshBuilder::GRID_SIZE * TerrainMeshBuilder::GRID_SIZE);
  file.read(reinterpret_cast<char*>(heights.data()), fileSize);

  // Run the pipeline
  float mockSkirtDepth = 50.0f;
  std::vector<Vertex3D> compositeVertices = TerrainMeshBuilder::GenerateMeshWithSkirts(heights, mockSkirtDepth);

  AABB tileBox;
  std::vector<QuantizedVertex> packedVertices = TerrainMeshBuilder::QuantizeMesh(compositeVertices, tileBox);

  std::cout << "Data Compression Pipeline Active:\n";
  std::cout << " -> Calculated Bounding Box Minimums: [" << tileBox.minX << ", " << tileBox.minY << ", " << tileBox.minZ << "]\n";
  std::cout << " -> Calculated Bounding Box Maximums: [" << tileBox.maxX << ", " << tileBox.maxY << ", " << tileBox.maxZ << "]\n\n";

  std::cout << "Memory Allocation Savings Analysis:\n";
  std::cout << " -> Raw Float Mesh Array Allocation:  " << (compositeVertices.size() * sizeof(Vertex3D)) << " Bytes.\n";
  std::cout << " -> Packed 16-bit Array Allocation: " << (packedVertices.size() * sizeof(QuantizedVertex)) << " Bytes.\n";

  float reductionPercent = (1.0f - (static_cast<float>(packedVertices.size() * sizeof(QuantizedVertex)) / (compositeVertices.size() * sizeof(Vertex3D)))) * 100.0f;
  std::cout << " -> Memory footprint optimized by:  " << reductionPercent << "%\n";

  return 0;
}