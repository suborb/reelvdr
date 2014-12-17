#include "setup.h"
#include "widgets.h"

#include <vdr/config.h>
#include <vdr/tools.h>

#include "compat.h"


cScroller::cScroller()
{
  Reset();
}

void cScroller::Reset()
{
  x = 0;
  y = 0;
  xmax = 0;
  font = NULL;
  text = "";
  active = false;
  update = false;
  position = 0;
  increment = 0;
  lastUpdate = 0;
}

bool cScroller::NeedsUpdate()
{
  if (active && 
      TimeMs() - lastUpdate > (unsigned long long) GraphLCDSetup.ScrollTime)
  {
    update = true;
    return true;
  }
  return false;
}

void cScroller::Init(int X, int Y, int Xmax, const GLCD::cFont * Font, const std::string & Text)
{
  x = X;
  y = Y;
  xmax = Xmax;
  font = Font;
  text = Text;
  increment = GraphLCDSetup.ScrollSpeed;
  position = 0;
  if (GraphLCDSetup.ScrollMode != 0 &&
      font->Width(text) > xmax - x + 1)
    active = true;
  else
    active = false;
  update = false;
  lastUpdate = TimeMs() + 2000;
}

void cScroller::Draw(GLCD::cBitmap * bitmap)
{
	if (!active)
	{
		bitmap->DrawText(x, y, xmax, text, font);
	}
	else
	{
		if (update)
		{
			if (increment > 0)
			{
				if (font->Width(text) - position + font->TotalWidth() * 5 < increment)
				{
					increment = 0;
					position = 0;
				}
			}
			else
			{
				if (GraphLCDSetup.ScrollMode == 2)
				{
					increment = GraphLCDSetup.ScrollSpeed;
				}
				else
				{
					active = false;
				}
			}
			position += increment;
			lastUpdate = TimeMs();
			update = false;
		}
		bitmap->DrawText(x, y, xmax, text, font, GLCD::clrBlack, true, position);
		if (font->Width(text) - position <= xmax - x + 10 + font->TotalWidth() * 5)
			bitmap->DrawText(x + font->Width(text) - position + font->TotalWidth() * 5, y, xmax, text, font);
	}
}
