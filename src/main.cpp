#include "main.h"
#include <fstream>

#define WIDTH 1024
#define HEIGHT 600
#define DEPTH 32
#define BPP 6

int sockfd;
bool firstUpdate = true;
bool diffMode = false;

INI ini;
AREA mainArea;
std::vector<Surface*> surfaces;


inline void EndianSwap( unsigned int &x )
{
  x = (x>>24) |
      ((x<<8) & 0x00ff0000) |
      ((x>>8) & 0x0000ff00) |
      (x<<24);
}

SDL_Surface *jpg = NULL;


void DrawScreen( SDL_Surface *screen )
{ 
  SDL_FillRect( screen, 0, 0 );
  if( jpg )
  {
    SDL_Rect src = { 0,0, 450, 455 };
    SDL_Rect dest = { 575, 0, 0, 0 };
    SDL_BlitSurface( jpg, &src, screen, &dest );
    src = { 0, 455, 450, 455 };
    dest = { 0, 0, 0, 0 };
    SDL_BlitSurface( jpg, &src, screen, &dest );
  }
  else
  {
    std::vector<Surface*>::iterator it;
    for( it = surfaces.begin(); it != surfaces.end(); ++it )
    {
      (*it)->DrawTo( screen );
    }
  }

  SDL_Flip( screen );
}



void HandleReceivedDiffs( std::vector<char> &data )
{
  if( jpg )
  {
    SDL_FreeSurface( jpg );
    jpg = NULL;
  }


  PIXDIFF pixDiff;
  std::vector<char>::iterator it;
  std::vector<Surface*>::iterator sit;

  for( it = data.begin(); it < data.end(); it+=7 )
  {
    pixDiff.x = ((*it)<<8) | (unsigned char)(*(it+1));
    pixDiff.y = (((*(it+2)<<8)) | (unsigned char)(*(it+3)) );
    pixDiff.color.r = (unsigned char)*(it+4);
    pixDiff.color.g = (unsigned char)*(it+5);
    pixDiff.color.b = (unsigned char)*(it+6);

    for( sit = surfaces.begin(); sit != surfaces.end(); ++sit )
    {
      if( (*sit)->IsInArea( pixDiff.x, pixDiff.y ) )
      {
        (*sit)->UpdatePixel( pixDiff );
      }
    }
  }
}



void HandleReceivedImage( std::vector<char> &data )
{
  if( jpg )
  {
    SDL_FreeSurface( jpg );
    jpg = NULL;
  }

  std::vector<char>::iterator it;
  std::vector<Surface*>::iterator sit;
  PIXDIFF tmp;

  unsigned short x=0;
  unsigned short y=0;

  for( it = data.begin(); it < data.end(); it+=3, ++x )
  {
    if( x >= mainArea.w )
    {
      x = 0;
      ++y;
    }
    if( y >= mainArea.h )
    {
      break;
    }

    tmp.x = x;
    tmp.y = y;
    tmp.color.r = *it;
    tmp.color.g = *(it+1);
    tmp.color.b = *(it+2);

    for( sit = surfaces.begin(); sit != surfaces.end(); ++sit )
    {
      if( (*sit)->IsInArea( tmp.x, tmp.y ) )
      {
        (*sit)->UpdatePixel( tmp );
      }
    }
  }
}



void HandleReceivedJPEG( std::vector<char> &data )
{
  SDL_RWops *rwop;
  rwop = SDL_RWFromMem( data.data(), data.size() );
  if( jpg )
    SDL_FreeSurface( jpg );
  jpg = IMG_LoadJPG_RW( rwop );
}



bool Update()
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

  request.push_back( mainArea.w>>8 );
  request.push_back( mainArea.w );
  request.push_back( mainArea.h>>8 );
  request.push_back( mainArea.h );
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

  if( headerBuffer[0] == 'J' )
    dataCount = elementCount;
  else if( fullImage )
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

  if( headerBuffer[0] == 'J' )
  {
    HandleReceivedJPEG( buffer );
    diffMode = true;
  }
  else if( fullImage )
  {
    HandleReceivedImage( buffer );
    diffMode = true;
  }
  else
    HandleReceivedDiffs( buffer );
  return true;
}



void LoadSurfaces()
{
  mainArea.x = atoi( ini["MAIN_AREA_X"].c_str() );
  mainArea.y = atoi( ini["MAIN_AREA_Y"].c_str() );
  mainArea.w = atoi( ini["MAIN_AREA_W"].c_str() );
  mainArea.h = atoi( ini["MAIN_AREA_H"].c_str() );

  for( int i=1 ;; ++i )
  {
    std::ostringstream stream;
    stream << "SURFACE_" << i;
    std::string  pre = stream.str();

    std::string key = pre + "_X";
    if( ini[key].length() <= 0 )
      break;

    AREA area, src;

    area.x = atoi( ini[key].c_str() );

    key = pre + "_Y";
    area.y = atoi( ini[key].c_str() );

    key = pre + "_W";
    area.w = atoi( ini[key].c_str() );

    key = pre + "_H";
    area.h = atoi( ini[key].c_str() );

    key = pre + "_SRC_X";
    src.x = atoi( ini[key].c_str() );

    key = pre + "_SRC_Y";
    src.y = atoi( ini[key].c_str() );

    Surface *newSurface = new Surface( area, src );
    surfaces.push_back( newSurface );
  }
}



int main( int argc, char* argv[] )
{
  SDL_Surface *screen;
  ini.Load( "config.ini" );

  int portno;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  printf( "trying to connect..\n" );

  portno = 45001;
  sockfd = socket( AF_INET, SOCK_STREAM, 0 );
  if( sockfd < 0 )
    printf( "ERROR opening socket!\n" );

  server = gethostbyname( ini["HOST"].c_str() );
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

  if( !(screen = SDL_SetVideoMode( WIDTH, HEIGHT, DEPTH, SDL_DOUBLEBUF | SDL_HWSURFACE )) )
  {
    SDL_Quit();
    return 1;
  }

  LoadSurfaces();

  bool quit = false;

  while( !quit )
  {
    if( Update() )
    {
      DrawScreen( screen );
    }
  }

  SDL_Quit();

  return 0;
}

