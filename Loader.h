#ifndef LOADER_H
#define LOADER_H

#include <filesystem>
#include <chrono>
#include <memory>
#include "Model.h"
#include "Animation.h"


using LoadedData = std::tuple<std::shared_ptr<const ModelData>, std::vector<std::shared_ptr<const AnimationData>>>;
LoadedData decodeFbx(const std::filesystem::path& file);

#endif
