#include "Surface.h"
#include <SDL/SDL_draw.h>


Surface::Surface( AREA area, AREA source )
{
  this->area = area;
  this->source = source;
  sdlSurface = SDL_CreateRGBSurface( SDL_HWSURFACE,
                                     area.w,
                                     area.h,
                                     32,
                                     0x0000ff00,
                                     0x00ff0000,
                                     0xff000000,
                                     0x000000ff );
  /* TODO:
   * Check if we succeeded at creating the surface. */
}



Surface::~Surface()
{
  SDL_FreeSurface( sdlSurface );
}



void Surface::UpdatePixel( PIXDIFF &pixel )
{
  Uint32 color = (pixel.color.r << 24) |
                 (pixel.color.g << 16) |
                 (pixel.color.b << 8 ) |
                 0xff;

  Draw_Pixel( sdlSurface,
              pixel.x - source.x,
              pixel.y - source.y,
              color );
}



void Surface::DrawTo( SDL_Surface *dest )
{
  SDL_Rect dstRect = { area.x, area.y, 0, 0 };
  SDL_BlitSurface( sdlSurface, NULL, dest, &dstRect );
}



bool Surface::IsInArea( unsigned short x, unsigned short y )
{
  if( x >= source.x && x < source.x+area.w &&
      y >= source.y && y < source.y+area.h )
    return true;
  return false;
}



SDL_Surface *Surface::GetSurface()
{
  return sdlSurface;
}

