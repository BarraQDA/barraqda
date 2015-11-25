/***************************************************************************
 *   Copyright (C) 2015 by Jonathan Schultz <jonathan@imatix.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TAGGING_H_
#define _OKULAR_TAGGING_H_

#include "okular_export.h"

namespace Okular {
  
class OKULAR_EXPORT Tagging
{
public:
    int x, y, w, h;
    
    Tagging() { x = 90; y = 170; w = 100; y = 120; }
};

class OKULAR_EXPORT TaggingUtils
{
    public:
	static void storeTagging ( int x, int y, int w, int h );
	
	static Tagging retrieveTagging ();

};

}

#endif
