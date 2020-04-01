#ifndef __SURFACE_H__
#define __SURFACE_H__

#include <SDL/SDL.h>


typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} PIXEL;



typedef struct
{
  unsigned short x;
  unsigned short y;
  PIXEL color;
} PIXDIFF;



typedef struct
{
  unsigned short x;
  unsigned short y;
  unsigned short w;
  unsigned short h;
} AREA;



class Surface
{
 private:
  SDL_Surface *sdlSurface;
  AREA area;
  AREA source;

 public:
  Surface( AREA area, AREA source );
  ~Surface();

  void UpdatePixel( PIXDIFF &pixel );
  void DrawTo( SDL_Surface *dest );

  bool IsInArea( unsigned short x, unsigned short y );

  SDL_Surface *GetSurface();
};

#endif

