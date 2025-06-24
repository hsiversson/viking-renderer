#pragma once

#include <cassert>

#define checkNoEntry() assert(false && "Enclosing block should never be called")