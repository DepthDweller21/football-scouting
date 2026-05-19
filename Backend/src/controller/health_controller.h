#pragma once

#include <crow.h>

crow::response rootHandler();
crow::response healthHandler();
crow::response dbHealthHandler();
