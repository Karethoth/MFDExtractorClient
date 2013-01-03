#include "main.h"

#define WIDTH 1024
#define HEIGHT 1200
#define DEPTH 32
#define BPP 6

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

int sockfd;
bool firstUpdate = true;
bool diffMode = false;

std::vector<PIXEL> image;
std::vector<PIXEL>::iterator imgIt;

inline void EndianSwap( unsigned int &x )
{
  x = (x>>24) |
      ((x<<8) & 0x00ff0000) |
      ((x>>8) & 0x0000ff00) |
      (x<<24);
}



void DrawScreen( SDL_Surface *screen, short imgW, short imgH )
{ 
  short x=0, y=0;
  for( imgIt = image.begin(); imgIt != image.end(); ++imgIt, ++x )
  {
    if( x >= imgW )
    {
      ++y;
      x = 0;
    }

    short translateX=0;
    short translateY=0;

    /*
    if( y < 745 )
    {
      translateX=-180;
      translateY=-290;
    }
    else
    {
      translateX=-750;
      translateY=-745;
    }
    */

    Uint32 color = (((*imgIt).r) << 24 ) |
                   (((*imgIt).g) << 16 ) |
                   (((*imgIt).b) << 8 ) |
                   0xff;
    EndianSwap( color );
    Draw_Pixel( screen, x+translateX, y+translateY, color );
  }

  SDL_Flip( screen ); 
}



void HandleReceivedDiffs( std::vector<char> &data, short imgW, short imgH )
{
  std::vector<char>::iterator it;
  unsigned short tmpX, tmpY;
  unsigned char r, g, b;
  for( it = data.begin(); it < data.end(); it+=7 )
  {
    tmpX = ((*it)<<8) | (unsigned char)(*(it+1));
    tmpY = (((*(it+2)<<8)) | (unsigned char)(*(it+3)) );
    r = (unsigned char)*(it+4);
    g = (unsigned char)*(it+5);
    b = (unsigned char)*(it+6);
    unsigned int pixelIndex = tmpY*imgW + tmpX;
    image[pixelIndex].r = r;
    image[pixelIndex].g = g;
    image[pixelIndex].b = b;
  }
}



void HandleReceivedImage( std::vector<char> &data )
{
  image.clear();

  std::vector<char>::iterator it;
  PIXEL tmp;
  for( it = data.begin(); it < data.end(); it+=3 )
  {
    tmp.r = *it;
    tmp.g = *(it+1);
    tmp.b = *(it+2);
    image.push_back( tmp );
  }
}



bool Update( short imgW, short imgH )
{
  std::vector<char> request;
  bool fullImage=false;

  if( firstUpdate )
  {
    firstUpdate = false;
    fullImage = true;
    request.push_back( 'I' );
  }
  else
    request.push_back( 'D' );

  request.push_back( imgW>>8 );
  request.push_back( imgW );
  request.push_back( imgH>>8 );
  request.push_back( imgH );
  request.push_back( '\n' );
  if( !diffMode )
    send( sockfd, &request[0], request.size(), 0 );


  char headerBuffer[6];
  int n;

  memset( headerBuffer, 0, 6 );

  n = recv( sockfd, headerBuffer, 6, 0 );

  if( headerBuffer[0] == 'I' )
    fullImage = true;
  else
    fullImage = false;

  int elementCount, dataCount;
  memcpy( &elementCount, (const void*)headerBuffer+1, 4 );
  EndianSwap( (unsigned int&)elementCount );

  if( fullImage )
    dataCount = elementCount*3;
  else
    dataCount = elementCount*7;

  std::vector<char> buffer;

  char tmpBuffer[10000];
  int tmp = dataCount;

  while( tmp > 0 )
  {
    memset( tmpBuffer, 0, 10000 );

    int requestedCount;
    if( tmp > 10000 )
      requestedCount = 10000;
    else
      requestedCount = tmp;

    n = recv( sockfd, tmpBuffer, requestedCount, 0 );
    buffer.insert( buffer.end(), tmpBuffer, tmpBuffer+n );
    tmp -= n;
  }

  if( buffer.size() == 0 )
  {
    return false;
  }

  if( fullImage )
  {
    HandleReceivedImage( buffer );
    diffMode = true;
  }
  else
    HandleReceivedDiffs( buffer, imgW, imgH );
  return true;
}



int main( int argc, char* argv[] )
{
  image.resize( 450*910 );
  SDL_Surface *screen;
  int portno;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  portno = 45001;
  sockfd = socket( AF_INET, SOCK_STREAM, 0 );
  if( sockfd < 0 )
    printf( "ERROR opening socket!\n" );

  server = gethostbyname( "192.168.2.2" );
  if( server == NULL )
    printf( "ERROR, no such host!\n" );

  bzero( (char*) &serv_addr, sizeof( serv_addr ) );
  serv_addr.sin_family = AF_INET;
  bcopy( (char*) server->h_addr,
         (char*)&serv_addr.sin_addr.s_addr,
         server->h_length );
  serv_addr.sin_port = htons( portno );

  if( connect( sockfd, (struct sockaddr*) &serv_addr, sizeof( serv_addr) ) < 0 )
  {
    printf( "ERROR connecting\n" );
  }

  printf( "Connected!\n" );

  if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) return 1;

  if( !(screen = SDL_SetVideoMode( WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE )) )
  {
    SDL_Quit();
    return 1;
  }

  bool quit = false;

  while( !quit )
  {
    if( Update( 450, 910 ) )
      DrawScreen( screen, 450, 910 );
  }

  SDL_Quit();

  return 0;
}

