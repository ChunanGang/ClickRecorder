#include "pch.h"


std::string ConvertWStringToString( const std::wstring& wstr )
{
    if( wstr.empty() ) {
        return std::string();
    }
    int size_needed = WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], (int)wstr.size(), NULL, 0, NULL, NULL );
    std::string str( size_needed, 0 );
    WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], (int)wstr.size(), &str[ 0 ], size_needed, NULL, NULL );
    return str;
}

std::wstring ConvertStringToWString( const std::string& str )
{
    if( str.empty() ) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar( CP_UTF8, 0, &str[ 0 ], (int)str.size(), NULL, 0 );
    std::wstring wstr( size_needed, 0 );
    MultiByteToWideChar( CP_UTF8, 0, &str[ 0 ], (int)str.size(), &wstr[ 0 ], size_needed );
    return wstr;
}

std::string GetExecutablePath()
{
    wchar_t buffer[ MAX_PATH ];
    GetModuleFileName( NULL, buffer, MAX_PATH );
    std::wstring fullPath( buffer );
    size_t pos = fullPath.find_last_of( L"\\/" );
    std::wstring directory = fullPath.substr( 0, pos );
    return ConvertWStringToString( directory );
}

// Function to check if a file exists
bool FileExists( const std::string& filename )
{
    std::ifstream file( filename );
    return file.good();
}

// Function to generate a unique filename using strftime
std::string GenerateUniqueFilename( const std::string& baseFilename )
{
    std::string newFilename = baseFilename;
    if( FileExists( baseFilename ) ) {
        // Generate a timestamp-based unique filename
        std::time_t t = std::time( nullptr );
        char buffer[ 100 ];
        std::strftime( buffer, sizeof( buffer ), "%Y%m%d%H%M%S", std::localtime( &t ) );
        std::ostringstream oss;
        oss << baseFilename.substr( 0, baseFilename.find_last_of( '.' ) ) << "_" << buffer << ".txt";
        newFilename = oss.str();
    }
    return newFilename;
}

std::vector<std::string> ParseConfigFile( const std::string& configFilename )
{
    std::vector<std::string> clickInfoFiles;
    std::ifstream configFile( configFilename );

    if( !configFile.is_open() ) {
        std::cerr << "Failed to open config file: " << configFilename << "\n";
        return clickInfoFiles;
    }

    UINT16 fileNumber = 0;
    std::string line;
    while( std::getline( configFile, line ) ) {

        // Trim leading whitespace
        line.erase( 0, line.find_first_not_of( " \t" ) );

        // Skip comment lines
        if( line.empty() || line[ 0 ] == '#' ) {
            continue;
        }

        // add as file name if not comment
        clickInfoFiles.push_back( line );
    }

    configFile.close();
    return clickInfoFiles;
}