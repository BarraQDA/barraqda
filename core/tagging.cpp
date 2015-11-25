/***************************************************************************
 *   Copyright (C) 2015 by Jonathan Schultz <jonathan@imatix.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tagging.h"

#include <kdebug.h>

using namespace Okular;

static Tagging tagging;

void TaggingUtils::storeTagging ( int x, int y, int w, int h )
{
    tagging.x = x;
    tagging.y = y;
    tagging.w = w;
    tagging.h = h;
    
    kWarning() << "Storing tag: x " << x << ", y " << y << ", w " << w << ", h " << h;

}

Tagging TaggingUtils::retrieveTagging ()
{
    return tagging;
}
    