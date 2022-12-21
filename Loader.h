#ifndef LOADER_H
#define LOADER_H

#include <filesystem>
#include <chrono>
#include "Model.h"
#include "Animation.h"


using LoadedData = std::tuple<ModelData, AnimationData, Skeleton>;
LoadedData decodeFbx(const std::filesystem::path& file);

#endif
