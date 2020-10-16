/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <locale>
#include <sstream>

#ifdef VITA
#include <psp2/io/stat.h>
#endif

#if defined( ANDROID ) || defined( _MSC_VER )
#include <clocale>
#endif

#include "system.h"
#include <SDL.h>

#if defined( __MINGW32CE__ ) || defined( __MINGW32__ ) || defined( _MSC_VER )
#include <windows.h>
#include <shellapi.h>
#endif

#if !defined( __MINGW32CE__ ) && !defined( _MSC_VER )
#include <unistd.h>
#endif

#if defined( __MINGW32CE__ )
#undef Shell_NotifyIcon
extern "C" {
BOOL WINAPI Shell_NotifyIcon( DWORD, PNOTIFYICONDATAW );
}

// wincommon/SDL_sysevents.c
extern HICON screen_icn;
extern HINSTANCE SDL_Instance;
extern HWND SDL_Window;
#endif

#if defined( __WIN32__ )
#include <io.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#if defined( __SYMBIAN32__ )
#define SEPARATOR '\\'
#elif defined( __WIN32__ )
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

#if !( defined( _MSC_VER ) || defined( __MINGW32CE__ ) || defined( __MINGW32__ ) )
#include <dirent.h>
#endif

#include "serialize.h"
#include "tools.h"

int System::MakeDirectory( const std::string & path )
{
#if defined( __SYMBIAN32__ )
    return mkdir( path.c_str(), S_IRWXU );
#elif defined( __WIN32__ ) && defined( _MSC_VER )
    return CreateDirectoryA( path.c_str(), NULL );
#elif defined( __WIN32__ ) && !defined( _MSC_VER )
    return mkdir( path.c_str() );
#elif defined( VITA )
    return sceIoMkdir( path.c_str(), 0777 );
#else
    return mkdir( path.c_str(), S_IRWXU );
#endif
}

std::string System::ConcatePath( const std::string & str1, const std::string & str2 )
{
    return std::string( str1 + SEPARATOR + str2 );
}

std::string System::GetHomeDirectory( const std::string & prog )
{
    std::string res;
#ifdef VITA
    res = "ux0:data/fheroes2";
    return res;
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    char * path = SDL_GetPrefPath( "", prog.c_str() );
    res = path;
    SDL_free( path );
#endif

    if ( System::GetEnvironment( "HOME" ) )
        res = System::ConcatePath( System::GetEnvironment( "HOME" ), std::string( "." ).append( prog ) );
    else if ( System::GetEnvironment( "APPDATA" ) )
        res = System::ConcatePath( System::GetEnvironment( "APPDATA" ), prog );

    return res;
}

ListDirs System::GetDataDirectories( const std::string & prog )
{
    ListDirs dirs;

#if defined( ANDROID )
    const char * internal = SDL_AndroidGetInternalStoragePath();
    if ( internal )
        dirs.push_back( System::ConcatePath( internal, prog ) );

    if ( SDL_ANDROID_EXTERNAL_STORAGE_READ && SDL_AndroidGetExternalStorageState() ) {
        const char * external = SDL_AndroidGetExternalStoragePath();
        if ( external )
            dirs.push_back( System::ConcatePath( external, prog ) );
    }

    dirs.push_back( System::ConcatePath( "/storage/sdcard0", prog ) );
    dirs.push_back( System::ConcatePath( "/storage/sdcard1", prog ) );
#else
    (void)prog;
#endif

    return dirs;
}

ListFiles System::GetListFiles( const std::string & prog, const std::string & prefix, const std::string & filter )
{
    ListFiles res;

#if defined( ANDROID )
    VERBOSE( prefix << ", " << filter );

    // check assets
    StreamFile sf;
    if ( sf.open( "assets.list", "rb" ) ) {
        std::list<std::string> rows = StringSplit( GetString( sf.getRaw( sf.size() ) ), "\n" );
        for ( std::list<std::string>::const_iterator it = rows.begin(); it != rows.end(); ++it )
            if ( prefix.empty() || ( ( prefix.size() <= ( *it ).size() && 0 == prefix.compare( ( *it ).substr( 0, prefix.size() ) ) ) ) ) {
                if ( filter.empty() || ( 0 == filter.compare( ( *it ).substr( ( *it ).size() - filter.size(), filter.size() ) ) ) )
                    res.push_back( *it );
            }
    }

    ListDirs dirs = GetDataDirectories( prog );

    for ( ListDirs::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
        res.ReadDir( prefix.size() ? System::ConcatePath( *it, prefix ) : *it, filter, false );
    }
#else
    (void)prog;
    (void)prefix;
    (void)filter;
#endif
    return res;
}

std::string System::GetDirname( const std::string & str )
{
    if ( str.size() ) {
        size_t pos = str.rfind( SEPARATOR );

        if ( std::string::npos == pos )
            return std::string( "." );
        else if ( pos == 0 )
            return std::string( "./" );
        else if ( pos == str.size() - 1 )
            return GetDirname( str.substr( 0, str.size() - 1 ) );
        else
            return str.substr( 0, pos );
    }

    return str;
}

std::string System::GetBasename( const std::string & str )
{
    if ( str.size() ) {
        size_t pos = str.rfind( SEPARATOR );

        if ( std::string::npos == pos || pos == 0 )
            return str;
        else if ( pos == str.size() - 1 )
            return GetBasename( str.substr( 0, str.size() - 1 ) );
        else
            return str.substr( pos + 1 );
    }

    return str;
}

const char * System::GetEnvironment( const char * name )
{
#if defined( __MINGW32CE__ ) || defined( __MINGW32__ )
    return SDL_getenv( name );
#else
    return getenv( name );
#endif
}

int System::SetEnvironment( const char * name, const char * value )
{
#if defined( __MINGW32CE__ ) || defined( __MINGW32__ ) || defined( _MSC_VER )
    std::string str( std::string( name ) + "=" + std::string( value ) );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    return _putenv( str.c_str() );
#else
    // SDL 1.2.12 (char *)
    return SDL_putenv( &str[0] );
#endif
#elif defined( VITA )
    // setenv not found with -std=c++11 on Vita
    return 1;
#else
    return setenv( name, value, 1 );
#endif
}

void System::SetLocale( int category, const char * locale )
{
#if defined( ANDROID ) || defined( __APPLE__ ) || defined( __clang__ )
    setlocale( category, locale );
#else
    std::setlocale( category, locale );
#endif
}

std::string System::GetMessageLocale( int length /* 1, 2, 3 */ )
{
    std::string locname;
#if defined( __MINGW32CE__ ) || defined( __MINGW32__ ) || defined( _MSC_VER )
    char * clocale = std::setlocale( LC_MONETARY, NULL );
#elif defined( ANDROID ) || defined( __APPLE__ ) || defined( __clang__ )
    char * clocale = setlocale( LC_MESSAGES, NULL );
#else
    char * clocale = std::setlocale( LC_MESSAGES, NULL );
#endif

    if ( clocale ) {
        locname = StringLower( clocale );
        // 3: en_us.utf-8
        // 2: en_us
        // 1: en
        if ( length < 3 ) {
            std::list<std::string> list = StringSplit( locname, length < 2 ? "_" : "." );
            return list.empty() ? locname : list.front();
        }
    }

    return locname;
}

int System::GetCommandOptions( int argc, char * const argv[], const char * optstring )
{
#if defined( __MINGW32CE__ ) || defined( _MSC_VER )
    (void)argc;
    (void)argv;
    (void)optstring;
    return -1;
#else
    return getopt( argc, argv, optstring );
#endif
}

char * System::GetOptionsArgument( void )
{
#if defined( __MINGW32CE__ ) || defined( _MSC_VER )
    return NULL;
#else
    return optarg;
#endif
}

size_t System::GetMemoryUsage( void )
{
#if defined( __SYMBIAN32__ )
    return 0;
#elif defined( __WIN32__ )
    static MEMORYSTATUS ms;

    ZeroMemory( &ms, sizeof( ms ) );
    ms.dwLength = sizeof( MEMORYSTATUS );
    GlobalMemoryStatus( &ms );

    return ( ms.dwTotalVirtual - ms.dwAvailVirtual );
#elif defined( __LINUX__ )
    unsigned int size = 0;
    std::ostringstream os;
    os << "/proc/" << getpid() << "/statm";

    std::ifstream fs( os.str().c_str() );
    if ( fs.is_open() ) {
        fs >> size;
        fs.close();
    }

    return size * getpagesize();
#else
    return 0;
#endif
}

std::string System::GetTime( void )
{
    time_t raw;
    struct tm * tmi;
    char buf[13] = {0};

    std::time( &raw );
    tmi = std::localtime( &raw );

    std::strftime( buf, sizeof( buf ) - 1, "%X", tmi );

    return std::string( buf );
}

bool System::IsFile( const std::string & name, bool writable )
{
#if defined( _MSC_VER )
    return writable ? ( 0 == _access( name.c_str(), 06 ) ) : ( 0 == _access( name.c_str(), 04 ) );
#elif defined( ANDROID ) || defined( VITA )
    return writable ? 0 == access( name.c_str(), W_OK ) : true;
#else
    std::string correctedPath;
    if ( !GetCaseInsensitivePath( name, correctedPath ) )
        return false;

    struct stat fs;

    if ( stat( correctedPath.c_str(), &fs ) || !S_ISREG( fs.st_mode ) )
        return false;

    return writable ? 0 == access( correctedPath.c_str(), W_OK ) : S_IRUSR & fs.st_mode;
#endif
}

bool System::IsDirectory( const std::string & name, bool writable )
{
#if defined( _MSC_VER )
    return writable ? ( 0 == _access( name.c_str(), 06 ) ) : ( 0 == _access( name.c_str(), 00 ) );
#elif defined( ANDROID ) || defined( VITA )
    return writable ? 0 == access( name.c_str(), W_OK ) : true;
#else
    std::string correctedPath;
    if ( !GetCaseInsensitivePath( name, correctedPath ) )
        return false;

    struct stat fs;

    if ( stat( correctedPath.c_str(), &fs ) || !S_ISDIR( fs.st_mode ) )
        return false;

    return writable ? 0 == access( correctedPath.c_str(), W_OK ) : S_IRUSR & fs.st_mode;
#endif
}

int System::Unlink( const std::string & file )
{
#if defined( _MSC_VER )
    return _unlink( file.c_str() );
#else
    return unlink( file.c_str() );
#endif
}

int System::CreateTrayIcon( bool fl )
{
#if defined( __MINGW32CE__ ) && defined( ID_ICON )
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof( nid );
    nid.uID = ID_ICON;
    nid.hWnd = SDL_Window;

    if ( fl ) {
        nid.uFlags = NIF_ICON | NIF_MESSAGE;
        nid.uCallbackMessage = WM_USER;
        nid.hIcon = ::LoadIcon( SDL_Instance, MAKEINTRESOURCE( ID_ICON ) );
        return Shell_NotifyIcon( NIM_ADD, &nid );
    }

    return Shell_NotifyIcon( NIM_DELETE, &nid );
#else
    (void)fl;
#endif
    return 0;
}

void System::PowerManagerOff( bool fl )
{
#if defined( __MINGW32CE__ )
    // power manager control
    const wchar_t lpGlobalSubKeyPM[] = TEXT( "System\\CurrentControlSet\\Control\\Power\\Timeouts" );
    const wchar_t lpNamePM[] = TEXT( "BattSuspendTimeout" );
    static DWORD origValuePM = 0;

    HKEY hKey;

    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, lpGlobalSubKeyPM, 0, KEY_ALL_ACCESS, &hKey ) ) {
        DWORD dwType = REG_DWORD;
        DWORD value = 0;

        // save orig value
        if ( fl ) {
            DWORD valueLen = sizeof( origValuePM );

            if ( ERROR_SUCCESS == RegQueryValueEx( hKey, lpNamePM, 0, &dwType, (LPBYTE)&origValuePM, &valueLen ) )
                RegSetValueEx( hKey, lpNamePM, 0, dwType, (const BYTE *)&value, sizeof( value ) );
        }
        else if ( origValuePM )
            RegSetValueEx( hKey, lpNamePM, 0, dwType, (const BYTE *)&origValuePM, sizeof( origValuePM ) );

        RegCloseKey( hKey );
    }

    HANDLE hEvent = CreateEvent( NULL, FALSE, FALSE, TEXT( "PowerManager/ReloadActivityTimeouts" ) );

    if ( hEvent != NULL ) {
        SetEvent( hEvent );
        CloseHandle( hEvent );
    }

    // backlight control
    const wchar_t lpGlobalSubKeyBL[] = TEXT( "\\ControlPanel\\Backlight" );
    const wchar_t lpNameBL[] = TEXT( "BatteryTimeout" );
    static DWORD origValueBL = 0;

    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER, lpGlobalSubKeyBL, 0, KEY_ALL_ACCESS, &hKey ) ) {
        DWORD dwType = REG_DWORD;
        DWORD value = 0;

        // save orig value
        if ( fl ) {
            DWORD valueLen = sizeof( origValueBL );

            if ( ERROR_SUCCESS == RegQueryValueEx( hKey, lpNameBL, 0, &dwType, (LPBYTE)&origValueBL, &valueLen ) )
                RegSetValueEx( hKey, lpNameBL, 0, dwType, (const BYTE *)&value, sizeof( value ) );
        }
        else if ( origValueBL )
            RegSetValueEx( hKey, lpNameBL, 0, dwType, (const BYTE *)&origValueBL, sizeof( origValueBL ) );

        RegCloseKey( hKey );
    }

    hEvent = CreateEvent( NULL, FALSE, FALSE, TEXT( "BackLightChangeEvent" ) );

    if ( hEvent != NULL ) {
        SetEvent( hEvent );
        CloseHandle( hEvent );
    }
#else
    (void)fl;
#endif
}

bool System::isRunning( void )
{
#if defined( __MINGW32CE__ )
    SetEnvironment( "DEBUG_VIDEO", "1" );
    SetEnvironment( "DEBUG_VIDEO_GAPI", "1" );

    HWND hwnd = FindWindow( NULL, L"SDL_app" );

    if ( hwnd ) {
        ShowWindow( hwnd, SW_SHOW );
        SetForegroundWindow( hwnd );
    }

    return hwnd;
#endif

    return false;
}

int System::ShellCommand( const char * cmd )
{
#if defined( __MINGW32CE__ )
    return cmd ? 0 : -1;
#else
    return system( cmd );
#endif
}

bool System::isEmbededDevice( void )
{
#if defined( __MINGW32CE__ ) || defined( ANDROID ) || defined( __SYMBIAN32__ )
    return true;
#endif
    return false;
}

int System::GetRenderFlags( void )
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#if defined( __MINGW32CE__ ) || defined( __SYMBIAN32__ )
    return SDL_RENDERER_SOFTWARE;
#elif defined( __WIN32__ ) || defined( ANDROID )
    return SDL_RENDERER_ACCELERATED;
#else
    return SDL_RENDERER_ACCELERATED;
    // return SDL_RENDERER_SOFTWARE;
#endif
#else
#if defined( __MINGW32CE__ ) || defined( __SYMBIAN32__ )
    return SDL_SWSURFACE;
#elif defined( __WIN32__ ) || defined( ANDROID )
    return SDL_HWSURFACE;
#else
    return SDL_SWSURFACE;
#endif
#endif
}

#if !( defined( _MSC_VER ) || defined( __MINGW32CE__ ) || defined( __MINGW32__ ) )
// splitUnixPath - function for splitting strings by delimiter
std::vector<std::string> splitUnixPath( const std::string & path, const std::string & delimiter )
{
    std::vector<std::string> result;

    if ( path.empty() ) {
        return result;
    }

    size_t pos = path.find( delimiter, 0 );
    while ( pos != std::string::npos ) { // while found delimiter
        const size_t nextPos = path.find( delimiter, pos + 1 );
        if ( nextPos != std::string::npos ) { // if found next delimiter
            if ( pos + 1 < nextPos ) { // have what to append
                result.push_back( path.substr( pos + 1, nextPos - pos - 1 ) );
            }
        }
        else { // if no more delimiter present
            if ( pos + 1 < path.length() ) { // if not a postfix delimiter
                result.push_back( path.substr( pos + 1 ) );
            }
            break;
        }

        pos = path.find( delimiter, pos + 1 );
    }

    if ( result.empty() ) { // if delimiter not present
        result.push_back( path );
    }

    return result;
}

// based on: https://github.com/OneSadCookie/fcaseopen
bool System::GetCaseInsensitivePath( const std::string & path, std::string & correctedPath )
{
    correctedPath.clear();

    if ( path.empty() )
        return false;

    DIR * d;
    bool last = false;
    const char * curDir = ".";
    const char * delimiter = "/";

    if ( path[0] == delimiter[0] ) {
        d = opendir( delimiter );
    }
    else {
        correctedPath = curDir[0];
        d = opendir( curDir );
    }

    const std::vector<std::string> splittedPath = splitUnixPath( path, delimiter );
    for ( std::vector<std::string>::const_iterator subPathIter = splittedPath.begin(); subPathIter != splittedPath.end(); ++subPathIter ) {
        if ( !d ) {
            return false;
        }

        if ( last ) {
            closedir( d );
            return false;
        }

        correctedPath.append( delimiter );

        struct dirent * e = readdir( d );
        while ( e ) {
            if ( strcasecmp( ( *subPathIter ).c_str(), e->d_name ) == 0 ) {
                correctedPath += e->d_name;

                closedir( d );
                d = opendir( correctedPath.c_str() );

                break;
            }

            e = readdir( d );
        }

        if ( !e ) {
            correctedPath += *subPathIter;
            last = true;
        }
    }

    if ( d )
        closedir( d );

    return !last;
}
#else
bool System::GetCaseInsensitivePath( const std::string & path, std::string & correctedPath )
{
    correctedPath = path;
    return true;
}
#endif
