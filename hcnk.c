/*
 *
 * This work has been migrated from my project, which was originally developed
 * in  the  Harbour  language. The code is inspired  by previous functions and
 * structures  that  were  implemented  in  that  project. This note serves to
 * highlight  the  development  history  and the source of inspiration for the
 * current project.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#if defined( _WIN32 ) || defined( _WIN64 )
   #include <direct.h>
   #include <windows.h>

   #define SDL_MAIN_HANDLED
   #define GET_CURRENT_DIR   _getcwd
   #define PATH_MAX          260  /* Windows standard path limit */
   #define PS                "\\"
#else
   #ifndef _POSIX_C_SOURCE
      #define _POSIX_C_SOURCE 200809L
   #endif

   #include <dirent.h>
   #include <sys/stat.h>
   #include <unistd.h>

   #define GET_CURRENT_DIR  getcwd
   #define PATH_MAX         4096  /* # chars in a path name including nul */
   #define PS               "/"
#endif

#include <SDL2/SDL.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION

#include "./include/nuklear.h"
#include "./include/nuklear_sdl_renderer.h"

#define IIF( condition, trueValue, falseValue ) ( ( condition ) ? ( trueValue ) : ( falseValue ) )

#define BOX_SINGLE         "┌─┐│┘└"
#define BOX_DOUBLE         "╔═╗║╝╚"
#define BOX_SINGLE_DOUBLE  "╓─╖║╜╙"
#define BOX_DOUBLE_SINGLE  "╒═╕│╛╘"

#define BLACK               nk_rgb(  12,  12,  12 ) /* "#0C0C0C" */
#define BLUE                nk_rgb(   0,  55, 218 ) /* "#0037DA" */
#define GREEN               nk_rgb(  19, 161,  14 ) /* "#13A10E" */
#define CYAN                nk_rgb(  58, 150, 221 ) /* "#3A96DD" */
#define RED                 nk_rgb( 197,  15,  31 ) /* "#C50F1F" */
#define MAGENTA             nk_rgb( 136,  23, 152 ) /* "#881798" */
#define BROWN               nk_rgb( 193, 156,   0 ) /* "#C19C00" */
#define LIGHT_GRAY          nk_rgb( 204, 204, 204 ) /* "#CCCCCC" */
#define GRAY                nk_rgb( 118, 118, 118 ) /* "#767676" */
#define LIGHT_BLUE          nk_rgb(  59, 120, 255 ) /* "#3B78FF" */
#define LIGHT_GREEN         nk_rgb(  22, 198,  12 ) /* "#16C60C" */
#define LIGHT_CYAN          nk_rgb(  97, 214, 214 ) /* "#61D6D6" */
#define LIGHT_RED           nk_rgb( 231,  72,  86 ) /* "#E74856" */
#define LIGHT_MAGENTA       nk_rgb( 180,   0, 158 ) /* "#B4009E" */
#define YELLOW              nk_rgb( 249, 241, 165 ) /* "#F9F1A5" */
#define WHITE               nk_rgb( 242, 242, 242 ) /* "#F2F2F2" */

#define WAIT_TIME_SECONDS   10

enum nk_bool
{
   F = 0,
   T = ( !0 )
};

typedef struct _HC      HC;
typedef struct _DirList DirList;

struct _DirList
{
   char name[ 512 ];
   char size[ 20 ];
   char date[ 11 ];
   char time[ 9 ];
   char attr[ 6 ];
   nk_bool state;
};

struct _HC
{
   int       col;
   int       row;
   int       maxCol;
   int       maxRow;

   char      currentDir[ PATH_MAX ];
   DirList  *dirList;
   int       itemCount;

   int       rowBar;
   int       rowNo;

   char      cmdLine[ PATH_MAX ];
   int       cmdCol;
   int       cmdColNo;

   nk_bool   isFirstDirectory;
   nk_bool   isHiddenDirectory;
   nk_bool   isFirstFile;
   nk_bool   isHiddenFile;

   nk_bool   sizeVisible;
   nk_bool   attrVisible;
   nk_bool   dateVisible;
   nk_bool   timeVisible;
};

static HC         *hc_init( void );
static void        hc_free( HC *selectedPanel );
static void        hc_printInfo( const HC *selectedPanel );
static void        hc_fetchList( HC *selectedPanel, const char *currentDir );
static int         hc_compareDirList( const void *A, const void *B );
static const char *hc_cwd( void );
static const char *hc_defaultValueChar( const char *A, const char *B );
static void        hc_strncpy( char oldValue[ PATH_MAX ], const char *newValue );
static DirList    *hc_directory( const char *currentDir, int *size );
static nk_bool     hc_loadFonts( struct nk_context *ctx, const char *filePath, float height );
static void        hc_resize( HC *selectedPanel, int col, int row, int maxCol, int maxRow );
static void        hc_drawPanel( struct nk_context *ctx, HC *selectedPanel );
static int         hc_findLongestName( HC *selectedPanel );
static int         hc_findLongestSize( HC *selectedPanel );
static int         hc_findLongestAttr( HC *selectedPanel );
static const char *hc_paddedString( HC *selectedPanel, int longestName, int longestSize, int longestAttr, const char *name, const char *size, const char *date, const char *time, const char *attr );
static int         hc_maxCol( struct nk_context *ctx );
static int         hc_maxRow( struct nk_context *ctx );
static void        hc_drawText( struct nk_context *ctx, int col, int row, const char *text, struct nk_color bgColor, struct nk_color textColor );
static void        hc_drawBox( struct nk_context* ctx, int x, int y, int width, int height, const char *boxString, struct nk_color bgColor, struct nk_color textColor );
static char       *hc_addStr( const char *firstString, ... );
static void        hc_changeDir( HC *selectedPanel );
static const char *hc_dirLastName( const char *path );
static const char *hc_dirDeleteLastPath( const char *path );
static void        hc_updateFetchList( HC *selectedPanel, const char *newDir );
static int         hc_dirIndexName( HC *selectedPanel, const char *tmpDir );
/* --- */
static void        hc_utf8CharExtract( const char *source, char *dest, size_t *index );
static size_t      hc_utf8Len( const char *utf8String );
static size_t      hc_utf8LenUpTo( const char *utf8String, const char *endPosition );
static const char *hc_utf8CharPtrAt( const char *utf8String, int characterOffset );
static int         hc_at( const char *search, const char *string );
static char       *hc_padR( const char *string, int length );
static char       *hc_padL( const char *string, int length );
static char       *hc_left( const char *string, int count );
static char       *hc_subStr( const char *string, int start, int count );
static char       *hc_strdup( const char *string );

HC *activePanel = NULL;

int main( int argc, char *argv[] )
{
   SDL_Window *window;
   SDL_Renderer *renderer;

   nk_bool quit = F;
   int windowWidth = 800, windowHeight = 450;

   nk_bool waitMode = T;
   time_t lastEventTime = time( NULL );

   HC *leftPanel   = NULL;
   HC *rightPanel  = NULL;
   int index;

   struct nk_context *ctx;
   static nk_flags windowFlags = NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | NK_WINDOW_SCROLL_AUTO_HIDE;
   nk_flags actualWindowFlags = 0;

   NK_UNUSED( argc );
   NK_UNUSED( argv );

   SDL_SetHint( SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0" );
   SDL_Init( SDL_INIT_VIDEO );

   window = SDL_CreateWindow( "Harbour Commander", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight,  SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE );
   if( window == NULL )
   {
      SDL_Log( "Error SDL_CreateWindow %s", SDL_GetError() );
      exit( -1 );
   }

   renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
   if( renderer == NULL )
   {
      SDL_Log( "Error SDL_CreateRenderer %s", SDL_GetError() );
      exit( -1 );
   }

   SDL_SetWindowMinimumSize( window, windowWidth, windowHeight );

   ctx = nk_sdl_init( window, renderer );

   leftPanel  = hc_init();
   rightPanel = hc_init();

   hc_fetchList( leftPanel, hc_cwd() );
   hc_fetchList( rightPanel, hc_cwd() );

   activePanel = leftPanel;

   hc_loadFonts( ctx, "9x18.ttf", 18 );

   while( !quit )
   {
      SDL_Event event;
      time_t currentTime = time( NULL );

      if( waitMode || difftime( currentTime, lastEventTime ) >= WAIT_TIME_SECONDS )
      {
         if( SDL_WaitEvent( &event ) )
         {
            lastEventTime = time( NULL );
            waitMode = F;
         }
      }
      else
      {
         nk_input_begin( ctx );
         while( SDL_PollEvent( &event ) )
         {
            lastEventTime = time( NULL );
            switch( event.type )
            {
               case SDL_QUIT:
                  quit = T;
                  break;

               case SDL_WINDOWEVENT:
                  if( event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED )
                  {
                     windowWidth = event.window.data1;
                     windowHeight = event.window.data2;
                  }
                  break;

               default:
                  break;
            }
            nk_sdl_handle_event( &event );
         }
         /* After `WAIT_TIME_SECONDS` seconds without events, return to `WaitEvent` mode */
         if( difftime( currentTime, lastEventTime ) >= WAIT_TIME_SECONDS )
         {
            waitMode = T;
         }

         nk_sdl_handle_grab();
         nk_input_end( ctx );

         SDL_SetRenderDrawColor( renderer, 12, 12, 12, 255 );
         SDL_RenderClear( renderer );
         /* --- */
         ctx->style.window.padding.x = 0;
         ctx->style.window.padding.y = 0;

         ctx->style.window.fixed_background = nk_style_item_color( WHITE );

         actualWindowFlags = windowFlags;
         if( !( windowFlags & NK_WINDOW_TITLE ) )
            windowFlags &= ~( NK_WINDOW_MINIMIZABLE | NK_WINDOW_CLOSABLE );
         if( nk_begin( ctx, "hcnk", nk_rect( 0, 0, windowWidth, windowHeight ), actualWindowFlags ) )
         {
            if( nk_input_is_key_pressed( &ctx->input, NK_KEY_ENTER ) )
            {
               index = activePanel->rowBar + activePanel->rowNo;
               if( hc_at( "D", activePanel->dirList[ index ].attr ) == 0 )
               {
                  hc_changeDir( activePanel );
               }
               else
               {
                  /* TODO */
               }
            }
            else if( nk_input_is_key_pressed( &ctx->input, NK_KEY_TAB ) )
            {
               if( activePanel == leftPanel )
               {
                  activePanel = rightPanel;
                  hc_strncpy( activePanel->cmdLine, leftPanel->cmdLine );
                  activePanel->cmdCol  = leftPanel->cmdCol;

                  hc_strncpy( leftPanel->cmdLine, "" );
                  leftPanel->cmdCol  = 0;
               }
               else
               {
                  activePanel = leftPanel;
                  hc_strncpy( activePanel->cmdLine, rightPanel->cmdLine );
                  activePanel->cmdCol  = rightPanel->cmdCol;

                  hc_strncpy( rightPanel->cmdLine, "" );
                  rightPanel->cmdCol  = 0;
               }
            }
            else if( nk_input_is_key_pressed( &ctx->input, NK_KEY_UP ) )
            {
               if( activePanel->rowBar > 0 )
               {
                  --activePanel->rowBar;
               }
               else if( activePanel->rowNo > 0 )
               {
                  --activePanel->rowNo;
               }
            }
            else if( nk_input_is_key_pressed( &ctx->input, NK_KEY_DOWN ) )
            {
               if( activePanel->rowBar < activePanel->maxRow - 3 && activePanel->rowBar <= activePanel->itemCount - 2 )
               {
                  ++activePanel->rowBar;
               }
               else if( activePanel->rowNo + activePanel->rowBar <= activePanel->itemCount - 2 )
               {
                  ++activePanel->rowNo;
               }
            }
            else if( nk_input_is_key_pressed( &ctx->input, NK_KEY_SCROLL_UP ) )
            {
               if( activePanel->rowBar <= 1 )
               {
                  if( activePanel->rowNo - hc_maxRow( ctx ) >= 0 )
                  {
                     activePanel->rowNo -= hc_maxRow( ctx );
                  }
                  else
                  {
                     activePanel->rowNo = 0;
                  }
               }
               activePanel->rowBar = 0;
            }
            else if( nk_input_is_key_pressed( &ctx->input, NK_KEY_SCROLL_DOWN ) )
            {
               if( activePanel->rowBar >= hc_maxRow( ctx ) - 4 ) /* ? */
               {
                  if( activePanel->rowNo + hc_maxRow( ctx ) <= activePanel->itemCount )
                  {
                     activePanel->rowNo += hc_maxRow( ctx ) - activePanel->rowBar;
                  }
               }
               activePanel->rowBar = NK_MIN( hc_maxRow( ctx ) - 4, activePanel->itemCount - activePanel->rowNo - 1 );
            }

            hc_resize( leftPanel, 0, 0, hc_maxCol( ctx ) / 2, hc_maxRow( ctx ) -3 );
            hc_resize( rightPanel, hc_maxCol( ctx ) / 2, 0, hc_maxCol( ctx ) / 2 -1, hc_maxRow( ctx ) -3 );

            hc_drawPanel( ctx, leftPanel );
            hc_drawPanel( ctx, rightPanel );
         }
         nk_end( ctx );
         hc_printInfo( activePanel );
         /* --- */
         nk_sdl_render( NK_ANTI_ALIASING_ON );
         SDL_RenderPresent( renderer );
      }
   }

   hc_free( leftPanel );
   hc_free( rightPanel );
   activePanel = NULL;

   nk_sdl_shutdown();
   SDL_DestroyRenderer( renderer );
   SDL_DestroyWindow( window );
   SDL_Quit();
   return 0;
}

static HC *hc_init( void )
{
   HC *panel = malloc( sizeof( HC ) );
   if( !panel )
   {
      fprintf( stderr, "Failed to allocate memory for HC. \n" );
      return NULL;
   }

   memset( panel, 0, sizeof( HC ) );

   panel->isFirstDirectory  = T;
   panel->isHiddenDirectory = F;
   panel->isFirstFile       = F;
   panel->isHiddenFile      = F;

   panel->sizeVisible = T;
   panel->attrVisible = T;
   panel->dateVisible = T;
   panel->timeVisible = T;

   return panel;
}

static void hc_free( HC *selectedPanel )
{
   if( selectedPanel )
   {
      if( selectedPanel->dirList )
      {
         free( selectedPanel->dirList );
      }
      free( selectedPanel );
   }
}

static void hc_printInfo( const HC *selectedPanel )
{
   printf( "\033[2J" );
   printf( "\033[H" );

   printf(" HC Info\n" );
   printf(" [\n" );
   printf("   col               : %d\n", selectedPanel->col );
   printf("   row               : %d\n", selectedPanel->row );
   printf("   maxCol            : %d\n", selectedPanel->maxCol );
   printf("   maxRow            : %d\n", selectedPanel->maxRow );
   printf("   currentDir        : %s\n", selectedPanel->currentDir );
   printf("   itemCount         : %d\n", selectedPanel->itemCount );
   printf("   rowBar            : %d\n", selectedPanel->rowBar );
   printf("   rowNo             : %d\n", selectedPanel->rowNo );
   printf("   isFirstDirectory  : %s\n", IIF( selectedPanel->isFirstDirectory, "T", "F" ) );
   printf("   isHiddenDirectory : %s\n", IIF( selectedPanel->isHiddenDirectory, "T", "F" ) );
   printf("   isFirstFile       : %s\n", IIF( selectedPanel->isFirstFile, "T", "F" ) );
   printf("   isHiddenFile      : %s\n", IIF( selectedPanel->isHiddenFile, "T", "F" ) );
   printf("   sizeVisible       : %s\n", IIF( selectedPanel->sizeVisible, "T", "F" ) );
   printf("   attrVisible       : %s\n", IIF( selectedPanel->attrVisible, "T", "F" ) );
   printf("   dateVisible       : %s\n", IIF( selectedPanel->dateVisible, "T", "F" ) );
   printf("   timeVisible       : %s\n", IIF( selectedPanel->timeVisible, "T", "F" ) );
   printf(" ]\n");

   fflush( stdout );
}

static void hc_fetchList( HC *selectedPanel, const char *currentDir )
{
   hc_strncpy( selectedPanel->currentDir, hc_defaultValueChar( currentDir, hc_cwd() ) );

   free( selectedPanel->dirList );
   selectedPanel->itemCount = 0;
   selectedPanel->dirList = hc_directory( selectedPanel->currentDir, &selectedPanel->itemCount );

   if( selectedPanel->isFirstDirectory )
   {
      qsort( selectedPanel->dirList, selectedPanel->itemCount, sizeof( DirList ), hc_compareDirList );
   }
}

static int hc_compareDirList( const void *A, const void *B )
{
   DirList *dirListA = ( DirList * ) A;
   DirList *dirListB = ( DirList * ) B;

   // The ".." Directory always comes first
   if( strcmp( dirListA->name, ".." ) == 0 ) return - 1;
   if( strcmp( dirListB->name, ".." ) == 0 ) return 1;

   // Directories before dirList
   nk_bool isDirA = strchr( dirListA->attr, 'D' ) != NULL;
   nk_bool isDirB = strchr( dirListB->attr, 'D' ) != NULL;
   if( isDirA != isDirB )
   {
      return IIF( isDirA, - 1, 1 );
   }

   // Hidden dirList/directories after regular ones
   nk_bool isHiddenA = strchr( dirListA->attr, 'H' ) != NULL;
   nk_bool isHiddenB = strchr( dirListB->attr, 'H' ) != NULL;
   if( isHiddenA != isHiddenB )
   {
      return IIF( isHiddenA, 1, - 1 );
   }

   return strcmp( dirListA->name, dirListB->name );
}

const char *hc_cwd( void )
{
   static char result[ PATH_MAX ];
   size_t len;
   char separator;

   if( GET_CURRENT_DIR( result, sizeof( result ) ) )
   {
      len = strlen( result );
      separator = PS[ 0 ];

      /* Checking if the path length is within the buffer limits */
      if( len >= ( sizeof( result ) - 2 ) )
      {
         fprintf( stderr, "Error: Path exceeds the allowed buffer size. \n" );
         return NULL;
      }

      /* Checking if there is already a separator at the end of the path */
      if( result[ len - 1 ] != separator )
      {
         result[ len ] = separator;
         result[ len + 1 ] = '\0';
      }

      return result;
   }
   else
   {
      fprintf( stderr, "Error: hc_cwd. \n" );
      return NULL;
   }
}

static const char *hc_defaultValueChar( const char *A, const char *B )
{
   return IIF( A && *A, A, B );
}

static void hc_strncpy( char oldValue[ PATH_MAX ], const char *newValue )
{
   if( newValue )
   {
      strncpy( oldValue, newValue, PATH_MAX - 1 );
      oldValue[ PATH_MAX - 1 ] = '\0';
   }
   else
   {
      oldValue[ 0 ] = '\0';
   }
}

static DirList *hc_directory( const char *currentDir, int *size )
{
#if defined( _WIN32 ) || defined( _WIN64 )
   DirList *files = NULL;
   int count = 0;
   int parentIndex = -1;

   WIN32_FIND_DATA findFileData;
   char fullPath[ PATH_MAX ];
   snprintf( fullPath, sizeof( fullPath ), "%s\\*", currentDir );
   HANDLE hFind = FindFirstFile( fullPath, &findFileData );

   if( hFind == INVALID_HANDLE_VALUE )
   {
      fprintf( stderr, "Directory cannot be opened: %s\n", currentDir );
      return NULL;
   }

   files = malloc( sizeof( DirList ) * INITIAL_FPCSR );
   if( !files )
   {
      fprintf( stderr, "Memory allocation error.\n" );
      FindClose( hFind );
      return NULL;
   }

   do
   {
      if( strcmp( findFileData.cFileName, "." ) == 0 )
      {
         continue;
      }

      if( strcmp( findFileData.cFileName, ".." ) == 0 )
      {
         parentIndex = count;
      }

      if( count >= INITIAL_FPCSR )
      {
         DirList *temp = realloc( files, sizeof( DirList ) * ( count + 1 ) );
         if( !temp )
         {
            fprintf( stderr, "Memory allocation error.\n" );
            FindClose( hFind );
            free( files );
            return NULL;
         }
         files = temp;
      }

      files[ count ].state = F;
      strncpy( files[ count ].name, findFileData.cFileName, sizeof( files[ count ].name ) - 1 );
      files[ count ].name[ sizeof( files[ count ].name ) - 1 ] = '\0';

      LARGE_INTEGER fileSize;
      fileSize.LowPart = findFileData.nFileSizeLow;
      fileSize.HighPart = findFileData.nFileSizeHigh;
      snprintf( files[ count ].size, sizeof( files[ count ].size ), "%lld", fileSize.QuadPart );

      FILETIME ft = findFileData.ftLastWriteTime;
      SYSTEMTIME st;
      FileTimeToLocalFileTime( &ft, &ft );
      FileTimeToSystemTime( &ft, &st );

      struct tm tm = ConvertSystemTimeToTm( &st );
      strftime( files[ count ].date, sizeof( files[ count ].date ), "%d-%m-%Y", &tm );
      strftime( files[ count ].time, sizeof( files[ count ].time ), "%H:%M:%S", &tm );

      strcpy( files[ count ].attr, "" );
      if( findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
      {
         strcat( files[ count ].attr, "D" );
      }
      if( findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )
      {
         strcat( files[ count ].attr, "H" );
      }

      count++;
   }
   while( FindNextFile( hFind, &findFileData ) != 0 );

   FindClose( hFind );

   /* Move the parent directory ("..") to the first position */
   if( parentIndex > 0 )
   {
      DirList temp = files[ parentIndex ];
      memmove( &files[ 1 ], &files[ 0 ], sizeof( DirList ) * parentIndex );
      files[ 0 ] = temp;
   }

   *size = count;
   return files;
#else
   DirList *files = NULL;
   int count = 0;
   int parentIndex = - 1;

   DIR *pDir;
   struct dirent *entry;
   struct stat fileInfo;

   pDir = opendir( currentDir );
   if( pDir == NULL )
   {
      fprintf( stderr, "Directory cannot be opened: %s\n", currentDir );
      return NULL;
   }

   while( ( entry = readdir( pDir ) ) != NULL )
   {
      if( strcmp( entry->d_name, "." ) == 0 )
      {
         continue;
      }

      if( strcmp( entry->d_name, ".." ) == 0 )
      {
         parentIndex = count;
      }

      {
         char fullPath[ PATH_MAX ];
         snprintf( fullPath, sizeof( fullPath ), "%s/%s", currentDir, entry->d_name );

         if( stat( fullPath, &fileInfo ) == - 1 )
         {
            perror( "Error getting file info" );
            continue;
         }
      }

      {
         DirList *temp = realloc( files, ( count + 1 ) * sizeof( DirList ) );
         if( !temp )
         {
            closedir( pDir );
            free( files );
            return NULL;
         }
         files = temp;
      }

      files[ count ].state = F;
      strncpy( files[ count ].name, entry->d_name, sizeof( files[ count ].name ) - 1 );
      files[ count ].name[ sizeof( files[ count ].name ) - 1 ] = '\0';

      snprintf( files[ count ].size, sizeof( files[ count ].size ), "%ld", fileInfo.st_size );

      {
         struct tm *tm = localtime( &fileInfo.st_mtime );
         strftime( files[ count ].date, sizeof( files[ count ].date ), "%d-%m-%Y", tm );
         strftime( files[ count ].time, sizeof( files[ count ].time ), "%H:%M:%S", tm );
      }

      strcpy( files[ count ].attr, "" );
      if( S_ISREG( fileInfo.st_mode ) )
      {
         strcat( files[ count ].attr, "A" );
         if( fileInfo.st_mode & S_IXUSR )
         {
            strcat( files[ count ].attr, "E" );
         }
      }
      if( S_ISDIR( fileInfo.st_mode ) )
      {
         strcat( files[ count ].attr, "D" );
      }
      if( entry->d_name[ 0 ] == '.' )
      {
         strcat( files[ count ].attr, "H" );
      }

      count++;
   }

   closedir( pDir );

   /* Move the parent directory ("..") to the first position if found */
   if( parentIndex > 0 )
   {
      DirList temp = files[ parentIndex ];
      memmove( &files[ 1 ], &files[ 0 ], sizeof( DirList ) * parentIndex );
      files[ 0 ] = temp;
   }

   *size = count;
   return files;
#endif
}

static nk_bool hc_loadFonts( struct nk_context *ctx, const char *filePath, float height )
{
   if( !ctx || !filePath )
      return F;

   struct nk_font_atlas *atlas = NULL;
   struct nk_font_config config = nk_font_config( 0 );
   struct nk_font *font = NULL;

   /* TODO */
   static const nk_rune hc_ranges[] =
   {
      0x0020, 0x007F, /* ASCII */
      0x0080, 0x00FF, /* Latin-1 Supplement */
      0x0100, 0x017F, /* Latin-Extended-A */
      0x0180, 0x024F, /* Latin-Extended-B */
      0x2500, 0x257F, /* Box Drawing */
      0x2580, 0x259F, /* Block Elements */
      0
   };

   config.range = hc_ranges;

   nk_sdl_font_stash_begin( &atlas );

   font = nk_font_atlas_add_from_file( atlas, filePath, height, &config );

   nk_sdl_font_stash_end();

   if( font )
   {
      nk_style_set_font( ctx, &font->handle );
      return T;
   }
   return F;
}

static void hc_resize( HC *selectedPanel, int col, int row, int maxCol, int maxRow )
{
   selectedPanel->col    = col;
   selectedPanel->row    = row;
   selectedPanel->maxCol = maxCol;
   selectedPanel->maxRow = maxRow;
}

static void hc_drawPanel( struct nk_context *ctx, HC *selectedPanel )
{
   int row, i = 0;
   int longestName = 4;
   int longestSize = 0;
   int longestAttr = 0;
   struct nk_color bgColor;
   struct nk_color textColor;

   if( activePanel == selectedPanel )
   {
      hc_drawBox( ctx, selectedPanel->col, selectedPanel->row, selectedPanel->maxCol, selectedPanel->maxRow, BOX_DOUBLE, WHITE, BLACK );
   }
   else
   {
      hc_drawBox( ctx, selectedPanel->col, selectedPanel->row, selectedPanel->maxCol, selectedPanel->maxRow, BOX_SINGLE, WHITE, BLACK );
   }

   longestName = NK_MAX( longestName, hc_findLongestName( selectedPanel ) );
   longestSize = hc_findLongestSize( selectedPanel );
   longestAttr = hc_findLongestAttr( selectedPanel );

   i += selectedPanel->rowNo;
   for( row = selectedPanel->row + 1; row < selectedPanel->maxRow - 1; row++ )
   {
      if( i < selectedPanel->itemCount )
      {
         const char *paddedString = hc_paddedString( selectedPanel, longestName, longestSize, longestAttr,
                                                     selectedPanel->dirList[ i ].name,
                                                     selectedPanel->dirList[ i ].size,
                                                     selectedPanel->dirList[ i ].date,
                                                     selectedPanel->dirList[ i ].time,
                                                     selectedPanel->dirList[ i ].attr );

         char *paddedResult = hc_padR( paddedString, selectedPanel->maxCol - 2 );

         if( activePanel == selectedPanel && i == selectedPanel->rowBar + selectedPanel->rowNo )
         {
            if( selectedPanel->dirList[ i ].state == T )
            {
               bgColor   = BLACK;
               textColor = RED;
            }
            else
            {
               bgColor   = BLACK;
               textColor = LIGHT_GREEN;
            }
         }
         else
         {
            if( selectedPanel->dirList[ i ].state == T )
            {
               bgColor   = WHITE;
               textColor = RED;
            }
            else if( strcmp( selectedPanel->dirList[ i ].attr, "DH" ) == 0 || strcmp( selectedPanel->dirList[ i ].attr, "AH" ) == 0 )
            {
               bgColor   = WHITE;
               textColor = LIGHT_BLUE;
            }
            else
            {
               bgColor   = WHITE;
               textColor = BLACK;
            }
         }

         hc_drawText( ctx, selectedPanel->col + 1, row, paddedResult, bgColor, textColor );

         free( paddedResult );

         ++i;
      }
      else
      {
         break;
      }
   }
}

static int hc_findLongestName( HC *selectedPanel )
{
   int longestName = 0;
   int i;

   for( i = 0; i < selectedPanel->itemCount; i++ )
   {
      int currentNameLength = hc_utf8Len( selectedPanel->dirList[ i ].name );
      if( currentNameLength > longestName )
      {
         longestName = currentNameLength;
      }
   }
   return longestName;
}

static int hc_findLongestSize( HC *selectedPanel )
{
   int longestSize = 0;
   int i;

   for( i = 0; i < selectedPanel->itemCount; i++ )
   {
      int currentSizeLength = strlen( selectedPanel->dirList[ i ].size );
      if( currentSizeLength > longestSize )
      {
         longestSize = currentSizeLength;
      }
   }
   return longestSize;
}

static int hc_findLongestAttr( HC *selectedPanel )
{
   int longestAttr = 0;
   int i;

   for( i = 0; i < selectedPanel->itemCount; i++ )
   {
      int currentAttrLength = strlen( selectedPanel->dirList[ i ].attr );
      if( currentAttrLength > longestAttr )
      {
         longestAttr = currentAttrLength;
      }
   }
   return longestAttr;
}

static const char *hc_paddedString( HC *selectedPanel, int longestName, int longestSize, int longestAttr, const char *name, const char *size, const char *date, const char *time, const char *attr )
{
   int lengthName = longestName;
   int lengthSize;
   int lengthAttr;
   int lengthDate;
   int lengthTime;
   int parentDir = 4;
   int space     = 1;
   int border    = 2;
   int nullTerminator = 1;

   static char formattedLine[ PATH_MAX ];

   if( strchr( attr, 'D' ) )
   {
      size = "DIR";
   }

   IIF( selectedPanel->sizeVisible, lengthSize = longestSize, lengthSize = 0 );
   IIF( selectedPanel->attrVisible, lengthAttr = longestAttr, lengthAttr = 0 );
   IIF( selectedPanel->dateVisible, lengthDate = 11, lengthDate = space );
   IIF( selectedPanel->timeVisible, lengthTime = 10, lengthTime = space );

   char *padLAttr = hc_padL( attr, lengthAttr );
   char *padLSize = hc_padL( size, lengthSize );

   char sizeAttrDateTime[ lengthSize + lengthAttr + lengthDate + lengthTime + nullTerminator ];

   if( selectedPanel->sizeVisible && selectedPanel->attrVisible && selectedPanel->dateVisible && selectedPanel->timeVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s %s %s", padLSize, padLAttr, date, time );
   }
   else if( selectedPanel->sizeVisible && selectedPanel->attrVisible && selectedPanel->dateVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s %s", padLSize, padLAttr, date );
   }
   else if( selectedPanel->sizeVisible && selectedPanel->attrVisible && selectedPanel->timeVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s %s", padLSize, padLAttr, time );
   }
   else if( selectedPanel->sizeVisible && selectedPanel->attrVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s", padLSize, padLAttr );
   }
   else if( selectedPanel->sizeVisible && selectedPanel->dateVisible && selectedPanel->timeVisible)
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s %s", padLSize, date, time );
   }
   else if( selectedPanel->sizeVisible && selectedPanel->dateVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s", padLSize, date );
   }
   else if( selectedPanel->sizeVisible && selectedPanel->timeVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s", padLSize, time );
   }
   else if ( selectedPanel->sizeVisible)
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s", padLSize );
   }
   else if( selectedPanel->attrVisible && selectedPanel->dateVisible && selectedPanel->timeVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s %s", padLAttr, date, time );
   }
   else if( selectedPanel->attrVisible && selectedPanel->dateVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s", padLAttr, date );
   }
   else if( selectedPanel->attrVisible && selectedPanel->timeVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s", padLAttr, time );
   }
   else if( selectedPanel->attrVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s", padLAttr );
   }
   else if( selectedPanel->dateVisible && selectedPanel->timeVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s %s", date, time );
   }
   else if( selectedPanel->dateVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s", date );
   }
   else if( selectedPanel->timeVisible )
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), "%s", time );
   }
   else
   {
      snprintf( sizeAttrDateTime, sizeof( sizeAttrDateTime ), " " );
   }

   if( strcmp( name, ".." ) == 0 )
   {
      const char *lBracket = "[";
      const char *rBracket = "]";

      char *padLSizeAttrDateTime = hc_padL( sizeAttrDateTime, selectedPanel->maxCol - border - parentDir );

      size_t max_len = sizeof( formattedLine ) - strlen( lBracket ) - strlen( name ) - strlen( rBracket ) - 1;
      if( strlen( padLSizeAttrDateTime ) > max_len )
      {
         padLSizeAttrDateTime[ max_len ] = '\0';
      }
      snprintf( formattedLine, sizeof( formattedLine ), "%s%s%s%s", lBracket, name, rBracket, padLSizeAttrDateTime );

      free( padLSizeAttrDateTime );
   }
   else
   {
      char *padLSizeAttrDateTime = hc_padL( sizeAttrDateTime, selectedPanel->maxCol - border - lengthName );

      int availableWidthForName = selectedPanel->maxCol - border - strlen( padLSizeAttrDateTime );

      char *cutName = hc_left( name, availableWidthForName );

      char *padRName = hc_padR( cutName, availableWidthForName );

      snprintf( formattedLine, sizeof( formattedLine ), "%s%s", padRName, padLSizeAttrDateTime );

      free( cutName );
      free( padRName );
      free( padLSizeAttrDateTime );
   }

   free( padLAttr );
   free( padLSize );

   return formattedLine;
}

static int hc_maxCol( struct nk_context *ctx )
{
   const struct nk_user_font *font = ctx->style.font;

   if( font && font->width )
   {
      const char *sampleText = "W";
      int sampleLength = 1;
      float fontCellWidth = font->width( font->userdata, font->height, sampleText, sampleLength );

      struct nk_rect bounds = nk_window_get_bounds( ctx );
      return bounds.w / fontCellWidth;
   }
   else
   {
      return 0;
   }
}

static int hc_maxRow( struct nk_context *ctx )
{
   const struct nk_user_font *font = ctx->style.font;

   if( font && font->height > 0 )
   {
      float fontCellHeight = font->height;

      struct nk_rect bounds = nk_window_get_bounds( ctx );
      return bounds.h / fontCellHeight;
   }
   else
   {
      return 0;
   }
}

static void hc_drawText( struct nk_context *ctx, int col, int row, const char *text, struct nk_color bgColor, struct nk_color textColor )
{
   const struct nk_user_font *font = ctx->style.font;
   const char *sampleText = "W";
   int sampleLength = 1;

   if( !font )
   {
      printf( "Error: No font set in context.\n" );
      return;
   }

   float fontCellWidth = font->width( font->userdata, font->height, sampleText, sampleLength );
   float fontCellHeight = font->height;

   struct nk_rect windowBounds = nk_window_get_bounds( ctx );

   float paddingX = ctx->style.window.padding.x + 2;

   float x = windowBounds.x + paddingX + col * fontCellWidth;
   /* TODO */
   float y = windowBounds.y + ( row + 2 ) * fontCellHeight;

   struct nk_command_buffer *canvas = nk_window_get_canvas( ctx );
   if( !canvas )
   {
      printf( "Error: No canvas available.\n" );
      return;
   }

   struct nk_rect textBackground = nk_rect( x, y, strlen( text ) * fontCellWidth, fontCellHeight );
   nk_fill_rect( canvas, textBackground, 0.0f, bgColor );

   struct nk_rect textRect = textBackground;
   textRect.y -= 1;

   nk_draw_text( canvas, textRect, text, strlen( text ), font, bgColor, textColor );
}

static void hc_changeDir( HC *selectedPanel )
{
   int i = selectedPanel->rowBar + selectedPanel->rowNo;

   if( strcmp( selectedPanel->dirList[ i ].name, ".." ) == 0 )
   {
      const char *tmpDir = hc_dirLastName( selectedPanel->currentDir );
      const char *newDir;
      newDir = hc_dirDeleteLastPath( selectedPanel->currentDir );

      hc_updateFetchList( selectedPanel, newDir );

      int lastPosition = NK_MAX( hc_dirIndexName( selectedPanel, tmpDir ), 1 ) ;
      if( lastPosition > activePanel->maxRow -3 )
      {
         activePanel->rowNo  = lastPosition % ( activePanel->maxRow -3 );
         activePanel->rowBar = activePanel->maxRow -3;
      }
      else
      {
         activePanel->rowNo  = 0;
         activePanel->rowBar = lastPosition;
      }
   }
   else
   {
      char *newDir = hc_addStr( selectedPanel->currentDir, selectedPanel->dirList[ i ].name, PS, NULL );
      selectedPanel->rowBar = 0;
      selectedPanel->rowNo  = 0;

      hc_updateFetchList( selectedPanel, newDir );
      free( newDir );
   }
}

static const char *hc_dirLastName( const char *path )
{
   static char result[ PATH_MAX ];
   int pathLength;

   if( path == NULL || ( pathLength = strlen( path ) ) == 0 )
   {
      result[ 0 ] = '\0';
      return result;
   }

   char separator = PS[ 0 ];

   if( path[ pathLength - 1 ] == separator )
   {
      pathLength--;
   }

   for( int i = pathLength - 1; i >= 0; i-- )
   {
      if( path[ i ] == separator )
      {
         int length = pathLength - i - 1;
         strncpy( result, path + i + 1, length );
         result[ length ] = '\0';
         return result;
      }
   }

   strncpy( result, path, pathLength );
   result[ pathLength ] = '\0';

   return result;
}

static const char *hc_dirDeleteLastPath( const char *path )
{
   static char result[ PATH_MAX ];
   char *lastPath;

   strncpy( result, path, sizeof( result ) - 1 );
   result[ sizeof( result ) - 1 ] = '\0';

   char separator = PS[ 0 ];

   lastPath = strrchr( result, separator );
   if( lastPath )
   {
      char *secondLastPath = NULL;
      *lastPath = '\0';
      secondLastPath = strrchr( result, separator );

      if( secondLastPath )
      {
         *lastPath = separator;
         *( secondLastPath + 1 ) = '\0';
      }
      else
      {
         *lastPath = separator;
      }
   }

   return result;
}

static void hc_updateFetchList( HC *selectedPanel, const char *newDir )
{
   strncpy( selectedPanel->currentDir, newDir, sizeof( selectedPanel->currentDir ) - 1 );
   selectedPanel->currentDir[ sizeof( selectedPanel->currentDir ) - 1 ] = '\0';
   hc_fetchList( selectedPanel, newDir );
}

static int hc_dirIndexName( HC *selectedPanel, const char *tmpDir )
{
   for( int i = 0; i < selectedPanel->itemCount; i++ )
   {
      if( strcmp( selectedPanel->dirList[ i ].name, tmpDir ) == 0 )
      {
         return i;
      }
   }
   return - 1;
}

/* -------------------------------------------------------------------------
UTF-8
------------------------------------------------------------------------- */
static void hc_utf8CharExtract( const char *source, char *dest, size_t *index )
{
   unsigned char firstByte = source[ *index ];
   size_t charLen = 1;

   if( ( firstByte & 0x80 ) == 0x00 )       /* ASCII (0xxxxxxx) */
      charLen = 1;
   else if( ( firstByte & 0xE0 ) == 0xC0 )  /* (110xxxxx) */
      charLen = 2;
   else if( ( firstByte & 0xF0 ) == 0xE0 )  /* (1110xxxx) */
      charLen = 3;
   else if( ( firstByte & 0xF8 ) == 0xF0 )  /* (11110xxx) */
      charLen = 4;

   memcpy( dest, source + *index, charLen );
   dest[ charLen ] = '\0';

   *index += charLen;
}

static size_t hc_utf8Len( const char *utf8String )
{
   size_t len = 0;
   while( *utf8String )
   {
      unsigned char byte = *utf8String;

      if( ( byte & 0x80 ) == 0 )
         utf8String += 1;
      else if( ( byte & 0xE0 ) == 0xC0 )
         utf8String += 2;
      else if( ( byte & 0xF0 ) == 0xE0 )
         utf8String += 3;
      else if( ( byte & 0xF8 ) == 0xF0 )
         utf8String += 4;
      else
         utf8String += 1;
      len++;
   }
   return len;
}

static size_t hc_utf8LenUpTo( const char *utf8String, const char *endPosition )
{
   size_t len = 0;
   while( utf8String < endPosition && *utf8String )
   {
      unsigned char byte = *utf8String;

      if( ( byte & 0x80 ) == 0 )         // ASCII (0xxxxxxx)
         utf8String += 1;
      else if( ( byte & 0xE0 ) == 0xC0 ) // (110xxxxx)
         utf8String += 2;
      else if( ( byte & 0xF0 ) == 0xE0 ) // (1110xxxx)
         utf8String += 3;
      else if( ( byte & 0xF8 ) == 0xF0 ) // (11110xxx)
         utf8String += 4;
      else
         utf8String += 1;

      len++;
   }

   return len;
}

static const char *hc_utf8CharPtrAt( const char *utf8String, int characterOffset )
{
   while( characterOffset > 0 && *utf8String )
   {
      unsigned char byte = *utf8String;

      if( ( byte & 0x80 ) == 0x00 )
         utf8String += 1;
      else if( ( byte & 0xE0 ) == 0xC0 )
         utf8String += 2;
      else if( ( byte & 0xF0 ) == 0xE0 )
         utf8String += 3;
      else if( ( byte & 0xF8 ) == 0xF0 )
         utf8String += 4;
      else
         utf8String += 1;

      characterOffset--;
   }
   return utf8String;
}

static void hc_drawBox( struct nk_context* ctx, int x, int y, int width, int height, const char *boxString, struct nk_color bgColor, struct nk_color textColor )
{
   /* Buffers for individual UTF-8 box-drawing characters (maximum 4 bytes + 1 for '\0') */
   char topLeft[ 5 ]     = { 0 };
   char horizontal[ 5 ]  = { 0 };
   char topRight[ 5 ]    = { 0 };
   char vertical[ 5 ]    = { 0 };
   char bottomRight[ 5 ] = { 0 };
   char bottomLeft[ 5 ]  = { 0 };

   /* Extract each UTF-8 character from boxString*/
   size_t index = 0;
   hc_utf8CharExtract( boxString, topLeft, &index );
   hc_utf8CharExtract( boxString, horizontal, &index );
   hc_utf8CharExtract( boxString, topRight, &index );
   hc_utf8CharExtract( boxString, vertical, &index );
   hc_utf8CharExtract( boxString, bottomRight, &index );
   hc_utf8CharExtract( boxString, bottomLeft, &index );

   hc_drawText( ctx, x, y, topLeft, bgColor, textColor );                  /* top-left corner */
   hc_drawText( ctx, x, y + height - 1, bottomLeft, bgColor, textColor );  /* bottom-left corner */

   for( int i = 1; i < width - 1; i++ )
   {
      hc_drawText( ctx, x + i, y, horizontal, bgColor, textColor );               /* top edge */
      hc_drawText( ctx, x + i, y + height - 1, horizontal, bgColor, textColor );  /* bottom edge */
   }

   for( int i = 1; i < height - 1; i++ )
   {
      hc_drawText( ctx, x, y + i, vertical, bgColor, textColor );              /* left edge */
      hc_drawText( ctx, x + width - 1, y + i, vertical, bgColor, textColor );  /* right edge */
   }

   hc_drawText( ctx, x + width - 1, y, topRight, bgColor, textColor );                  /* top-right corner */
   hc_drawText( ctx, x + width - 1, y + height - 1, bottomRight, bgColor, textColor );  /* bottom-right corner */
}

/* -------------------------------------------------------------------------
char *hc_addStr( const char *firstString, ... )
Adds the following strings in sequence.
------------------------------------------------------------------------- */
static char *hc_addStr( const char *firstString, ... )
{
   if( !firstString || *firstString == '\0' )
   {
      return hc_strdup( "" );
   }

   size_t totalLength = 0;
   va_list args;

   va_start( args, firstString );
   const char *string = firstString;
   while( string )
   {
      totalLength += strlen( string );
      string = va_arg( args, const char * );
   }
   va_end( args );

   char *result = ( char * ) malloc( totalLength + 1 );
   if( !result )
   {
      return NULL;
   }

   result[ 0 ] = '\0';

   va_start( args, firstString );
   string = firstString;
   while( string )
   {
      strcat( result, string );
      string = va_arg( args, const char * );
   }
   va_end( args );

   return result;
}

/* -------------------------------------------------------------------------
int hc_at( const char *search, const char *string )
Return the position of a substring within a character string.
------------------------------------------------------------------------- */
static int hc_at( const char *search, const char *string )
{
   if( !search || !*search || !string || !*string )
   {
      return 0;
   }

   char *occurrence = strstr( string, search );
   if( !occurrence )
   {
      return 0;
   }

   int utf8Position = hc_utf8LenUpTo( string, occurrence );

   return utf8Position;
}

/* -------------------------------------------------------------------------
const char *hc_padR( const char *string, int length )
Character function that pads a string of characters by a specified length.
------------------------------------------------------------------------- */
static char *hc_padR( const char *string, int length )
{
   if( !string || length <= 0 )
   {
      return hc_strdup( "" );
   }

   int len = hc_utf8Len( string );
   int byteLen = strlen( string );

   if( len >= length )
   {
      return hc_subStr( string, 0, length );
   }
   else
   {
      int padding = length - len;

      char *result = ( char * ) malloc( byteLen + padding + 1 );
      if( !result )
      {
         return NULL;
      }

      memcpy( result, string, byteLen );
      memset( result + byteLen, ' ', padding );
      result[ byteLen + padding ] = '\0';
      return result;
   }
}

/* -------------------------------------------------------------------------
const char *hc_padL( const char *string, int length )
Character function that pads a string of characters by a specified length.
------------------------------------------------------------------------- */
static char *hc_padL( const char *string, int length )
{
   if( !string || length <= 0 )
   {
      return hc_strdup( "" );
   }

   int len = hc_utf8Len( string );
   int byteLen = strlen( string );

   if( len >= length )
   {
      const char *byteEnd = hc_utf8CharPtrAt( string, length );
      int truncatedBytes = byteEnd - string;

      char *result = ( char * ) malloc( truncatedBytes + 1 );
      if( !result )
      {
         return NULL;
      }

      memcpy( result, string, truncatedBytes );
      result[ truncatedBytes ] = '\0';
      return result;
   }
   else
   {
      int padding = length - len;

      char *result = ( char * ) malloc( padding + byteLen + 1 );
      if( !result )
      {
         return NULL;
      }

      memset( result, ' ', padding );
      memcpy( result + padding, string, byteLen );
      result[ padding + byteLen ] = '\0';
      return result;
   }
}

/* -------------------------------------------------------------------------
const char *hc_left( const char *string, int count )
Extract a substring beginning with the first character in a string.
------------------------------------------------------------------------- */
static char *hc_left( const char *string, int count )
{
   if( !string || count <= 0 )
   {
      return hc_strdup( "" );
   }

   int len = hc_utf8Len( string );

   if( count <= 0 )
   {
      char *result = ( char * ) malloc( 1 );
      if( result )
      {
         result[ 0 ] = '\0';
      }
      return result;
   }

   if( count >= len )
   {
      return strdup( string );
   }

   const char *byteEnd = hc_utf8CharPtrAt( string, count );
   int byteCount = byteEnd - string;

   char *result = ( char * ) malloc( byteCount + 1 );
   if( result == NULL )
   {
      return NULL;
   }

   memcpy( result, string, byteCount );
   result[ byteCount ] = '\0';

   return result;
}

/* -------------------------------------------------------------------------
char *hc_subStr( const char *string, int start, int count )
Extract a substring from a character string
------------------------------------------------------------------------- */
static char *hc_subStr( const char *string, int start, int count )
{
   if( !string || *string == '\0' )
   {
      return hc_strdup( "" );
   }

   int nSize = hc_utf8Len( string );

   if( start > nSize )
   {
      count = 0;
   }

   if( count > 0 )
   {
      if( start < 0 )
      {
         start = nSize + start;
      }
      if( start < 0 )
      {
         start = 0;
      }
      if( start + count > nSize )
      {
         count = nSize - start;
      }
   }
   else
   {
      if( start < 0 )
      {
         start = nSize + start;
      }
      if( start < 0 )
      {
         start = 0;
      }
      count = nSize - start;
   }

   const char *byteStart = hc_utf8CharPtrAt( string, start );
   const char *byteEnd = hc_utf8CharPtrAt( byteStart, count );

   int byteCount = byteEnd - byteStart;

   char *result = ( char * ) malloc( byteCount + 1 );
   if( !result )
   {
      return hc_strdup( "" );
   }

   memcpy( result, byteStart, byteCount );
   result[ byteCount ] = '\0';

   return result;
}

static char *hc_strdup( const char *string )
{
   if( !string )
   {
      char *dup = ( char * ) malloc( 1 );
      if( dup )
      {
         dup[ 0 ] = '\0';
      }
      return dup;
   }

   size_t len = strlen( string ) + 1;
   char *dup = ( char * ) malloc( len );
   if( dup )
   {
      memcpy( dup, string, len );
   }

   return dup;
}