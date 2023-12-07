#include "HSRIFF.hpp"

bool HSMakeRiffChunkFourCC ( char * lpChunkBaseName , std::string * lpChunkFourCC ){
	if ( lpChunkBaseName == nullptr ) return false;
	if ( lpChunkFourCC == nullptr ) return false;
	char name [ 5 ];
	name [ 4 ] = 0;
	size_t paramLen = strlen ( lpChunkBaseName );
	memcpy ( name , lpChunkBaseName , ( paramLen > 4 ) ? 4 : paramLen );
	if ( paramLen < 4 ) memset ( name + paramLen , ' ' , 4 - paramLen );
	*lpChunkFourCC = name;
	return true;
}

int HSScanRiffChunkTable ( char * lpChunkName , CHSRiffChunkTable * pTable ){
	std::string name;
	if ( HSMakeRiffChunkFourCC ( lpChunkName , &name ) == false ) return -1;
	if ( pTable == nullptr ) return -1;
	uint32_t base = *reinterpret_cast< const uint32_t* >( name.c_str ( ) );
	uint32_t target;
	int len = pTable->size ( );

	for ( int i = 0; i < len; i++ ) {
		target = *reinterpret_cast< uint32_t* >( pTable->at ( i ).Header.Name );
		if ( base == target ) {
			return i;
		}
	}
	return -1;
}


