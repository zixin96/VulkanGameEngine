#pragma once

// std
#include <iostream>
#include <fstream>
#include <array>
#include <chrono>
#include <string>
#include <functional>
#include <utility>

#include <cassert>
#include <cstring>

#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include <limits>
#include <iostream>
#include <optional>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>