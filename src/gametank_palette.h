// Minimal header: expose palette values as an external symbol to avoid multiple
// definitions when the header is included from multiple translation units.

#ifndef GAMETANK_PALETTE_H
#define GAMETANK_PALETTE_H

#include <cstdint>

using namespace std;

#define NUM_PALLETS 4

extern const uint8_t gt_palette_vals[768 * NUM_PALLETS];

#endif
