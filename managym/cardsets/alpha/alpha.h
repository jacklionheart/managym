#pragma once

#include "managym/state/card.h"

// Alpha cards

class CardRegistry;

namespace managym::cardsets {

Card llanowarElves();
Card greyOgre();

void registerAlpha(CardRegistry* registry);

} // namespace managym::cardsets