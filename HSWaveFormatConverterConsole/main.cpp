#include <stdio.h>
#include <Windows.h>
#include <locale.h>
#include <string>
#include <Shobjidl.h>
#include <Shlwapi.h>
#include <atlstr.h>
#include "inc/HSWaveOperation.hpp"
#include "inc/HSWaveConverter.hpp"
#include "inc/HSWAVE.hpp"
#pragma comment(lib,"Comctl32.lib")

#pragma comment(lib , "winmm.lib")
#pragma comment(lib,"Shlwapi.lib")



#pragma comment(linker,"\"/manifestdependency:type='win32' "	\
	"name='Microsoft.Windows.Common-Controls' "					\
	"version='6.0.0.0' "										\
	"processorArchitecture='*' "								\
	"publicKeyToken='6595b64144ccf1df' "						\
	"language='*'\"")


int AppMain ( void );
void AppPause ( void );

bool HSCheckSupportFormat ( WAVEFORMATEX wfex );

void HSShowFormat ( WAVEFORMATEX wfex );


bool HSMakeOutFormat ( WAVEFORMATEX inFormat , WAVEFORMATEX *poutFormat );

bool HSSelectFormatAndBitrate ( WAVEFORMATEX inFormat , WAVEFORMATEX *poutFormat );
bool HSSelectSamplingFrequency ( WAVEFORMATEX inFormat , WAVEFORMATEX *poutFormat );

bool HSGetOpenWaveFile ( std::wstring *pFilePath );

bool HSGetSaveWaveFile ( std::wstring *pFilePath );

std::wstring MilisecondsToString( uint32_t ms );

void  SetExeDirToCurrentDir ( void );

int main ( void ) {

	SetExeDirToCurrentDir ( );
	setlocale ( LC_ALL , "Japanese" );
	InitCommonControls ( );

	SetConsoleTitleA ( "hirosof WAVEフォーマットコンバーターコンソール" );

	CoInitialize ( NULL );

	AppMain ( );

	CoUninitialize ( );

	AppPause ( );
	return 0;
}

void AppPause ( void ) {
	system ( "pause" );
}


int OpenAfterProcess ( CHSWaveReaderW &cReader , std::wstring  baseFilePath );

int AppMain ( void ) {

	//変換元のファイル指定
	printf ( "変換元(入力元)のWAVEファイル選択してください。\n" );
	std::wstring BaseWaveFilePath;
	if ( HSGetOpenWaveFile ( &BaseWaveFilePath ) == false ) {
		printf ( "ファイル選択がキャンセルされたかエラーが発生しました。\n" );
		return -1;
	}
	wprintf ( L"変換元：%s\n" , BaseWaveFilePath.c_str ( ) );
	
	//変換元のファイルを開く
	printf ( "\n変換元のWAVEファイルを開いています・・・" );
	CHSWaveReaderW cReader;
	if ( cReader.Open ( BaseWaveFilePath.c_str ( ) ) == false ) {
		printf ( "失敗\n" );
		return -2;	
	}
	printf ( "成功\n" );

	int ret = OpenAfterProcess ( cReader  , BaseWaveFilePath);
	
	cReader.Close ( );

	return 0;
}



bool ConvertProcess ( CHSWaveReaderW &cReader , CHSWaveWriterW &cWriter  , WAVEFORMATEX inFormat,WAVEFORMATEX outFormat , double pitch);

int OpenAfterProcess ( CHSWaveReaderW &cReader  , std::wstring  baseFilePath) {
	//フォーマット取得
	printf ( "フォーマットを取得しています・・・" );
	WAVEFORMATEX inFormat;
	if ( cReader.GetFormat ( &inFormat ) == false ) {
		printf ( "失敗\n" );
		return -3;
	}
	printf ( "成功\nフォーマットをチェックしています・・・" );

	if ( HSCheckSupportFormat ( inFormat ) == false ) {
		printf ( "非対応\n" );
		return -3;
	}
	printf ( "対応\n" );


	printf ( "\n<<変換元フォーマット情報>>\n" );
	HSShowFormat ( inFormat );


	if ( cReader.HasListChunk( "INFO" ) ) {

		printf( "\n<<変換元WAVEァイルのメタデータ(INFO型LISTチャンクデータ>>\n" );

		CHSRiffChunkTable table;

		if ( cReader.GetListChunkMemberTable( "INFO", &table ) ) {
			std::string desc;
			char name[5];
			name[4] = 0;

			for ( auto it = table.cbegin( ); it != table.cend( ); it++ ) {
				memcpy( name, it->Header.Name, 4 );

				desc = HSGetListInfoChunkMemberDescription( name );
				if ( desc.length( ) == 0 ) desc = name;

				char* pdata = new char[it->Header.DataSize + 1];
				*( pdata + it->Header.DataSize ) = 0;

				if ( cReader.ReadListChunkMemberData( "INFO", name, pdata, 0, it->Header.DataSize ) > 0 ) {
					printf( "%s (%s) : %s\n", name, desc.c_str( ), pdata );
				}
				
				delete[]pdata;
			}
			printf( "\n" );
		}

	}


	printf ( "変換先(保存先)のWAVEファイルを指定してください。\n" );
	std::wstring TargetWaveFilePath;

	wchar_t *pPath = new wchar_t [ baseFilePath.size ( ) + 1 ];

	lstrcpyW ( pPath , baseFilePath.c_str ( ) );

	PathRemoveExtensionW ( pPath );

	TargetWaveFilePath = pPath;
	TargetWaveFilePath += L"_Output";

	delete [ ]pPath;

	if ( HSGetSaveWaveFile ( &TargetWaveFilePath ) == false ) {
		printf ( "ファイル指定がキャンセルされたかエラーが発生しました。\n" );
		return -4;
	}
	wprintf ( L"変換先：%s\n\n" , TargetWaveFilePath.c_str ( ) );



	printf ( "<<出力フォーマット指定(チャンネル数は固定で変換元と同じです。)>>\n" );

	WAVEFORMATEX outFormat;
	char checkText [ 32 ];
	outFormat.nChannels = inFormat.nChannels;

	do {
		if ( HSMakeOutFormat ( inFormat , &outFormat ) == false ) {
			return -5;
		}

		printf ( "\n<<変換先フォーマット情報>>\n" );

		HSShowFormat ( outFormat );


		printf ( "上記のフォーマットへの変換でよろしいですか？(Y/N)：" );

		checkText[0] = 0;
		scanf_s ( "%[^\n]" , checkText , 32 );
		(void)getchar ( );

		if ( ( checkText [ 0 ] == 'N' ) || ( checkText [ 0 ] == 'n' ) ) {
			printf ( "再指定してください。\n" );
		}
	} while ( ( checkText [ 0 ] == 'N' ) || ( checkText [ 0 ] == 'n' ) );


	printf ( "\n<<おまけ機能>>\n" );
	double pitch;

	printf ( "ピッチ数(再生速度)を実数で指定してください：" );
	scanf_s ( "%lf" , &pitch );

	if ( pitch <= 0 ) {
		pitch = 1.00;
		printf ( "不正な値が入力されたのでピッチ数は 1.00 に設定されました。\n" );
	}

	printf ( "出力先ファイルを作成しています・・・" );
	CHSWaveWriterW cWriter;
	
	if ( cWriter.Create ( TargetWaveFilePath.c_str ( ) ) ) {
		printf ( "成功\n" );

		bool bRet = ConvertProcess ( cReader , cWriter ,inFormat ,outFormat , pitch );

		cWriter.Close ( );
		return ( bRet ) ? 1 : 0;

	} else {
		printf ( "失敗しました。\n" );
		return -6;
	}
}


bool ConvertProcess ( CHSWaveReaderW &cReader , CHSWaveWriterW &cWriter ,
	WAVEFORMATEX inFormat , WAVEFORMATEX outFormat , double pitch )
{
	printf ( "処理の準備をしています・・・\n" );
	if ( outFormat.wFormatTag == WAVE_FORMAT_PCM ) {
		PCMWAVEFORMAT pcm;
		memcpy ( &pcm , &outFormat , sizeof ( PCMWAVEFORMAT ) );
		cWriter.WriteFormatChunk ( pcm );
	} else {
		cWriter.WriteFormatChunk ( outFormat );
	}

	double rate = inFormat.nSamplesPerSec;
	rate /= outFormat.nSamplesPerSec;
	rate *= pitch;

	//入力サンプル数
	uint32_t inputSamples = cReader.GetDataChunkSize ( ) / inFormat.nBlockAlign;
	
	//処理ブロックにかかわる計算
	uint32_t ProcessBlockSeconds = 1;
	uint32_t ProcessBlockSamples = inFormat.nSamplesPerSec * ProcessBlockSeconds;
	uint32_t ProcessBlockBytes = ProcessBlockSamples * inFormat.nBlockAlign;
	uint32_t NumberOfProcessBlocks = inputSamples / ProcessBlockSamples;
	uint32_t RestSamplesOfProcessBlock = inputSamples % ProcessBlockSamples;
	uint32_t RestBytesOfProcessBlock = RestSamplesOfProcessBlock * inFormat.nBlockAlign;

	if ( RestSamplesOfProcessBlock > 0 ) NumberOfProcessBlocks++;

	//出力サンプル数
	uint32_t outSamples;

	if ( rate == 1 ) outSamples = inputSamples;
	else outSamples = HSWaveConvCalc_ReSamplingOutSamples ( inputSamples , ProcessBlockSamples , rate );

	bool bWriteFact;

	bWriteFact = ( outFormat.wFormatTag == WAVE_FORMAT_MULAW );
	bWriteFact |= ( outFormat.wFormatTag == WAVE_FORMAT_ALAW );
	if ( bWriteFact ) cWriter.WriteChunkType<uint32_t> ( "fact" , outSamples );



	CHSWaveBufferNormalized input_norm[3] , output_norm;
	CHSWaveBufferUnknown initData , loadData , saveData;

	for ( int i = 0; i < 3; i++ ) {
		input_norm [ i ].Alloc ( ProcessBlockSamples , ( uint8_t ) inFormat.nChannels );
	}

	HSWave_UnNormalize ( &input_norm[0] , &initData , inFormat );
	HSWaveBufferPreparation ( &initData , &loadData );

	for ( int i = 1; i <= 2; i++ ) {
		memcpy ( loadData.GetBufferPointer ( 0 , 0 ) ,
			initData.GetBufferPointer ( 0 , 0 ) ,
			initData.GetBufferSizeBytes ( )
		);		
		cReader.ReadDataChunk ( loadData.GetBufferPointer ( 0 , 0 ) ,
			( i - 1 ) * ProcessBlockBytes ,
			loadData.GetBufferSizeBytes ( )
		);

		HSWave_Normalize ( &loadData , inFormat , &input_norm [ i ] );
	}

	cWriter.BeginDataChunk ( );

	bool bLastProcessBlock;
	printf ( "処理を開始します。\n" );
	DWORD st = timeGetTime ( );
	for ( uint32_t block = 0; block < NumberOfProcessBlocks; block++ ) {

		printf ( "\r進行状況：%u / %u 処理ブロック(%d%%)" ,
			block + 1 , NumberOfProcessBlocks ,
			( ( block + 1 ) * 100 ) / NumberOfProcessBlocks
		);
		bLastProcessBlock = ( block == ( NumberOfProcessBlocks - 1 ) );

		if ( bLastProcessBlock && ( RestSamplesOfProcessBlock > 0 ) ) {
			input_norm [ 1 ].ReAlloc ( RestSamplesOfProcessBlock , ( uint8_t ) inFormat.nChannels );
		}

		if ( rate != 1 ) {
			HSWaveConv_ReSampling_MT ( &input_norm [ 1 ] , &output_norm , rate ,
				&input_norm [ 0 ] , &input_norm [ 2 ]
			);
		} else {
			HSWaveBufferPreparation ( &input_norm [ 1 ] , &output_norm );
			memcpy ( output_norm.GetBufferPointer ( 0 , 0 ) ,
				input_norm [ 1 ].GetBufferPointer ( 0 , 0 ) ,
				input_norm [ 1 ].GetBufferSizeBytes()
			);
		}

		HSWave_UnNormalize ( &output_norm , &saveData , outFormat );
		
		cWriter.AdditionalDataChunkContent ( saveData.GetBufferPointer ( 0 , 0 ) ,
			saveData.GetBufferSizeBytes ( )
		);

		if ( bLastProcessBlock == false ) {

			for ( int i = 1; i <= 2; i++ ) {
				memcpy ( input_norm [ i - 1 ].GetBufferPointer ( 0 , 0 ) ,
					input_norm [ i ].GetBufferPointer ( 0 , 0 ) ,
					input_norm [ i - 1 ].GetBufferSizeBytes ( )
				);
			}
			memcpy ( loadData.GetBufferPointer ( 0 , 0 ) ,
				initData.GetBufferPointer ( 0 , 0 ) ,
				initData.GetBufferSizeBytes ( )
			);
			cReader.ReadDataChunk ( loadData.GetBufferPointer ( 0 , 0 ) ,
				( block + 2 ) * ProcessBlockBytes ,
				loadData.GetBufferSizeBytes ( )
			);
			HSWave_Normalize ( &loadData , inFormat , &input_norm [ 2 ] );
		}

	}
	DWORD et = timeGetTime ( );

	cWriter.EndDataChunk ( );
	for ( int i = 0; i < 3; i++ ) {
		input_norm [ i ].Free ( );
	}
	initData.Free ( );
	loadData.Free ( );
	output_norm.Free ( );
	saveData.Free ( );
	
	printf( "\n処理が終わりました。\n" );
	
	printf( "\n<<処理結果>>\n" );

	printf( "処理時間\n\t%S\n\n", MilisecondsToString( et - st ).c_str() );
	printf( "サンプル数\n\t処理元：%u\n\t処理先：%u\n\n", inputSamples, outSamples );
	printf( "再生時間\n\t処理元：%S\n\t処理先：%S\n\n",
		MilisecondsToString( static_cast<uint32_t>( inputSamples * 1000.0 / inFormat.nSamplesPerSec ) ).c_str( ),
		MilisecondsToString( static_cast<uint32_t>( outSamples * 1000.0 / outFormat.nSamplesPerSec ) ).c_str( ) );

	return true;
}


std::wstring MilisecondsToString( uint32_t ms ) {

	CAtlStringW sw;


	uint32_t sec = ms / 1000;


	sw.Format( L"%02u:%02u:%02u.%03u (%u ms)", sec / 3600, sec % 3600 / 60, sec % 60, ms % 1000 ,ms);


	return std::wstring( sw.GetString( ) );
}


const uint32_t HSWAVE_CUSTOMFREQUENCY_MIN = 8000;
const uint32_t HSWAVE_CUSTOMFREQUENCY_MAX = 192000;

bool HSMakeOutFormat ( WAVEFORMATEX inFormat , WAVEFORMATEX *poutFormat ) {
	if ( poutFormat == nullptr ) return false;
	poutFormat->cbSize = 0; //追加データなし
	printf ( "基本フォーマットを表示されるダイアログで選択してください。\n" );
	if ( HSSelectFormatAndBitrate ( inFormat , poutFormat ) == false ) {
		printf ( "キャンセルもしくは内部エラーが発生しました。\n" );
		return false;
	}


	printf ( "次に標本化周波数を選択してください。\n" );
	if ( HSSelectSamplingFrequency ( inFormat , poutFormat ) == false ) {
		printf ( "キャンセルもしくは内部エラーが発生しました。\n" );
		return false;
	}

	poutFormat->nBlockAlign = poutFormat->wBitsPerSample / 8 * poutFormat->nChannels;
	poutFormat->nAvgBytesPerSec = poutFormat->nSamplesPerSec * poutFormat->nBlockAlign;

	if ( HSCheckSupportFormat ( *poutFormat ) == false ) {
		printf ( "内部エラーが発生しました。\n" );
		return false;
	}

	return true;
}

enum struct EHSWAVESELECTFORMAT {
	FromInputFormat = 100,
	PCM8 ,
	PCM16 ,
	PCM24 ,
	PCM32 ,
	IEEEFLOAT32 ,
	IEEEFLOAT64 ,
	PCMU,
	PCMA
};
bool HSSelectFormatAndBitrate ( WAVEFORMATEX inFormat , WAVEFORMATEX *poutFormat ) {
	if ( poutFormat == nullptr ) return false;

	TASKDIALOGCONFIG tc;

	memset ( &tc , 0 , sizeof ( TASKDIALOGCONFIG ) );
	tc.cbSize = sizeof ( TASKDIALOGCONFIG );

	tc.hwndParent = GetConsoleWindow ( );

	tc.dwFlags = TDF_SIZE_TO_CONTENT | TDF_USE_COMMAND_LINKS;
	tc.pszWindowTitle = L"出力フォーマット選択ダイアログ";

	tc.pszMainInstruction = L"フォーマットを以下から選択してください";
	

	TASKDIALOG_BUTTON buttons [ 9 ];
	tc.cButtons = 9;
	tc.pButtons = buttons;
	tc.dwCommonButtons = TDCBF_CANCEL_BUTTON;

	tc.nDefaultButton = ( int ) EHSWAVESELECTFORMAT::FromInputFormat;
	wchar_t InputFormatText[64] ,  buf[24];
	
	wchar_t *pFormatText [ ] = { L"PCM" , L"PCM[float]" ,L"PCMU(μ-law)",L"PCMA(A-law)" };

	swprintf_s ( InputFormatText , L"変換元と同じ(" );


	switch ( inFormat.wFormatTag ) {
		case WAVE_FORMAT_PCM:
			wcscat_s ( InputFormatText , pFormatText [ 0 ] );
			break;
		case WAVE_FORMAT_IEEE_FLOAT:
			wcscat_s ( InputFormatText , pFormatText [ 1 ] );
			break;
		case WAVE_FORMAT_MULAW:
			wcscat_s ( InputFormatText , pFormatText [ 2 ] );
			break;
		case WAVE_FORMAT_ALAW:
			wcscat_s ( InputFormatText , pFormatText [ 3 ] );
			break;
	}
	swprintf_s ( buf , L")\n量子化ビット数：%uビット"  , inFormat.wBitsPerSample);
	wcscat_s ( InputFormatText , buf );

	//入力に同じ
	buttons [ 0 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::FromInputFormat;
	buttons [ 0 ].pszButtonText = InputFormatText;

	//PCM 8bit
	buttons [ 1 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::PCM8;
	buttons [ 1 ].pszButtonText = L"PCM\n量子化ビット数：8ビット";

	//PCM 16bit
	buttons [ 2 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::PCM16;
	buttons [ 2 ].pszButtonText = L"PCM\n量子化ビット数：16ビット";

	//PCM 24bit
	buttons [ 3 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::PCM24;
	buttons [ 3 ].pszButtonText = L"PCM\n量子化ビット数：24ビット";
	
	//PCM 32bit
	buttons [ 4 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::PCM32;
	buttons [ 4 ].pszButtonText = L"PCM\n量子化ビット数：32ビット";
	
	//PCM(float) 32bit
	buttons [ 5 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::IEEEFLOAT32;
	buttons [ 5 ].pszButtonText = L"PCM(float)\n量子化ビット数：32ビット";

	//PCM(float) 64bit
	buttons [ 6 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::IEEEFLOAT64;
	buttons [ 6 ].pszButtonText = L"PCM(float)\n量子化ビット数：64ビット";

	
	//PCMU 8bit
	buttons [ 7 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::PCMU;
	buttons [ 7 ].pszButtonText = L"PCMU(μ-law)\n量子化ビット数：8ビット";

	//PCMA 8bit
	buttons [ 8 ].nButtonID = ( int ) EHSWAVESELECTFORMAT::PCMA;
	buttons [ 8 ].pszButtonText = L"PCMA(A-law)\n量子化ビット数：8ビット";


	int select=0;
	if ( TaskDialogIndirect ( &tc , &select , nullptr , nullptr ) != S_OK ) {
		printf ( "TaskDialogIndirect エラー" );
		return false;
	}
	if ( select == IDCANCEL ) return false;

	EHSWAVESELECTFORMAT ID = ( EHSWAVESELECTFORMAT ) select;

	switch ( ID ) {
		case EHSWAVESELECTFORMAT::FromInputFormat:
			poutFormat->wFormatTag = inFormat.wFormatTag;
			poutFormat->wBitsPerSample = inFormat.wBitsPerSample;
			break;
		case EHSWAVESELECTFORMAT::PCM8:
			poutFormat->wFormatTag = WAVE_FORMAT_PCM;
			poutFormat->wBitsPerSample = 8;
			break;
		case EHSWAVESELECTFORMAT::PCM16:
			poutFormat->wFormatTag = WAVE_FORMAT_PCM;
			poutFormat->wBitsPerSample = 16;
			break;
		case EHSWAVESELECTFORMAT::PCM24:
			poutFormat->wFormatTag = WAVE_FORMAT_PCM;
			poutFormat->wBitsPerSample = 24;
			break;
		case EHSWAVESELECTFORMAT::PCM32:
			poutFormat->wFormatTag = WAVE_FORMAT_PCM;
			poutFormat->wBitsPerSample = 32;
			break;
		case EHSWAVESELECTFORMAT::IEEEFLOAT32:
			poutFormat->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
			poutFormat->wBitsPerSample = 32;
			break;
		case EHSWAVESELECTFORMAT::IEEEFLOAT64:
			poutFormat->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
			poutFormat->wBitsPerSample = 64;
			break;
		case EHSWAVESELECTFORMAT::PCMU:
			poutFormat->wFormatTag = WAVE_FORMAT_MULAW;
			poutFormat->wBitsPerSample = 8;
			break;
		case EHSWAVESELECTFORMAT::PCMA:
			poutFormat->wFormatTag = WAVE_FORMAT_ALAW;
			poutFormat->wBitsPerSample = 8;
			break;
	}

	return true;
}


enum struct EHSWAVESELECTSAMPLINGFREQUENCY {
	FromInput = 100 ,
	FConsoleInput ,
	F22050 ,
	F44100 ,
	F48000 ,
	F96000 ,
	F192000
};


bool HSSelectSamplingFrequency ( WAVEFORMATEX inFormat , WAVEFORMATEX *poutFormat ) {

	if ( poutFormat == nullptr ) return false;

	TASKDIALOGCONFIG tc;

	memset ( &tc , 0 , sizeof ( TASKDIALOGCONFIG ) );
	tc.cbSize = sizeof ( TASKDIALOGCONFIG );

	tc.hwndParent = GetConsoleWindow ( );

	tc.dwFlags = TDF_SIZE_TO_CONTENT | TDF_USE_COMMAND_LINKS;
	tc.pszWindowTitle = L"標本化周波数選択ダイアログ";

	tc.pszMainInstruction = L"標本化周波数を以下から選択してください";
	tc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	TASKDIALOG_BUTTON buttons [ 7 ];
	tc.cButtons = 7;
	tc.pButtons = buttons;
	tc.nDefaultButton = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::FromInput;
	wchar_t InputFormatText [ 128 ] , ConsoleInputText[128];


	swprintf_s ( InputFormatText , L"変換元と同じ(%u Hz)" , inFormat.nSamplesPerSec );
	buttons [ 0 ].nButtonID = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::FromInput;
	buttons [ 0 ].pszButtonText = InputFormatText;

	swprintf_s ( ConsoleInputText , L"コンソールに直接入力する\n%uHz～%uHz間の標本化周波数を直接入力できます。" ,
		HSWAVE_CUSTOMFREQUENCY_MIN , HSWAVE_CUSTOMFREQUENCY_MAX );

	buttons [ 1 ].nButtonID = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::FConsoleInput;
	buttons [ 1 ].pszButtonText = ConsoleInputText;

	buttons [ 2 ].nButtonID = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::F22050;
	buttons [ 2 ].pszButtonText = L"22.05Khz(22050Hz)";
	buttons [ 3 ].nButtonID = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::F44100;
	buttons [ 3 ].pszButtonText = L"44.1Khz(44100Hz)";
	buttons [ 4 ].nButtonID = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::F48000;
	buttons [ 4 ].pszButtonText = L"48Khz(48000Hz)";
	buttons [ 5 ].nButtonID = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::F96000;
	buttons [ 5 ].pszButtonText = L"96Khz(96000Hz)";
	buttons [ 6 ].nButtonID = ( int ) EHSWAVESELECTSAMPLINGFREQUENCY::F192000;
	buttons [ 6 ].pszButtonText = L"192Khz(192000Hz)";


	int select = 0;
	if ( TaskDialogIndirect ( &tc , &select , nullptr , nullptr ) != S_OK ) {
		printf ( "TaskDialogIndirect エラー" );
		return false;
	}
	if ( select == IDCANCEL ) return false;

	EHSWAVESELECTSAMPLINGFREQUENCY freqselect = ( EHSWAVESELECTSAMPLINGFREQUENCY ) select;

	switch ( freqselect ) {
		case EHSWAVESELECTSAMPLINGFREQUENCY::FromInput:
			poutFormat->nSamplesPerSec = inFormat.nSamplesPerSec;
			break;
		case EHSWAVESELECTSAMPLINGFREQUENCY::FConsoleInput:
			{
				uint32_t f;
				printf ( "標本化周波数を%uHz～%uHzの間で入力してください：" ,
					HSWAVE_CUSTOMFREQUENCY_MIN , HSWAVE_CUSTOMFREQUENCY_MAX );
				scanf_s ( "%u" , &f );

				if ( f < HSWAVE_CUSTOMFREQUENCY_MIN ) {
					printf ( "%uHzを下回る値が入力されましたので%uHzが設定されました。" ,
						HSWAVE_CUSTOMFREQUENCY_MIN, HSWAVE_CUSTOMFREQUENCY_MIN
					);
					f = HSWAVE_CUSTOMFREQUENCY_MIN;
				} else if ( f > HSWAVE_CUSTOMFREQUENCY_MAX ) {
					printf ( "%uHzを上回る値が入力されましたので%uHzが設定されました。" ,
						HSWAVE_CUSTOMFREQUENCY_MAX, HSWAVE_CUSTOMFREQUENCY_MAX
					);

					f = HSWAVE_CUSTOMFREQUENCY_MAX;
				}

				poutFormat->nSamplesPerSec = f;
			}
			break;
		case EHSWAVESELECTSAMPLINGFREQUENCY::F22050:
			poutFormat->nSamplesPerSec = 22050;
			break;
		case EHSWAVESELECTSAMPLINGFREQUENCY::F44100:
			poutFormat->nSamplesPerSec = 44100;
			break;
		case EHSWAVESELECTSAMPLINGFREQUENCY::F48000:
			poutFormat->nSamplesPerSec = 48000;
			break;
		case EHSWAVESELECTSAMPLINGFREQUENCY::F96000:
			poutFormat->nSamplesPerSec = 96000;
			break;
		case EHSWAVESELECTSAMPLINGFREQUENCY::F192000:
			poutFormat->nSamplesPerSec = 192000;
			break;
	}
	
	return true;
}


bool HSCheckSupportFormat ( WAVEFORMATEX wfex ) {
	if ( wfex.nChannels <= 2 ) {
		switch ( wfex.wFormatTag ) {
			case WAVE_FORMAT_PCM:
				if ( ( wfex.wBitsPerSample <= 32 ) && ( ( wfex.wBitsPerSample % 8 ) == 0 ) ) {
					return true;
				}
				break;
			case WAVE_FORMAT_IEEE_FLOAT:
				if ( wfex.wBitsPerSample == 32 ) return true;
				if ( wfex.wBitsPerSample == 64 ) return true;
				break;
			case WAVE_FORMAT_MULAW:
			case WAVE_FORMAT_ALAW:
				if ( wfex.wBitsPerSample == 8 ) return true;
				break;
		}
	}
	return false;
}

void HSShowFormat ( WAVEFORMATEX wfex ) {
	printf ( "フォーマット：" );

	switch ( wfex.wFormatTag ) {
		case WAVE_FORMAT_PCM:
			printf ( "PCM" );
			break;
		case WAVE_FORMAT_IEEE_FLOAT:
			printf ( "PCM(floating-point)" );
			break;
		case WAVE_FORMAT_MULAW:
			printf ( "PCMU(μ-law)" );
			break;
		case WAVE_FORMAT_ALAW:
			printf ( "PCMA(A-law)" );
			break;
	}

	printf ( "\n標本化周波数：%u\n" , wfex.nSamplesPerSec );
	printf ( "量子化ビット数：%u\n" , wfex.wBitsPerSample );
	printf ( "チャンネル数：%u\n\n" , wfex.nChannels );
}

bool HSGetOpenWaveFile ( std::wstring * pFilePath )
{
	if ( pFilePath == nullptr ) return false;

	bool bRet = false;
	IFileOpenDialog *pDialog;

	HRESULT hr = CoCreateInstance ( CLSID_FileOpenDialog , NULL , CLSCTX_INPROC_SERVER , IID_PPV_ARGS ( &pDialog ) );

	if ( FAILED ( hr ) ) return false;

	wchar_t cPath [ MAX_PATH ];

	GetCurrentDirectoryW ( MAX_PATH , cPath );

	PathAddBackslashW ( cPath );

	IShellItem *pCurrent;

	SHCreateItemFromParsingName ( cPath , NULL , IID_PPV_ARGS ( &pCurrent ) );

	pDialog->SetDefaultFolder ( pCurrent );

	pDialog->SetDefaultExtension ( L"wav" );

	COMDLG_FILTERSPEC fs = { L"WAVEファイル(*.wav)" , L"*.wav" };

	pDialog->SetFileTypes ( 1 , &fs );

	pDialog->SetOptions ( FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST );

	pDialog->SetTitle ( L"変換元のWAVEファイルを指定してください。" );

	HWND hwnd = GetConsoleWindow ( );
	if ( pDialog->Show ( hwnd ) == S_OK ) {

		IShellItem *pResult;

		if ( pDialog->GetResult ( &pResult ) == S_OK ) {

			wchar_t *pres = nullptr;
			pResult->GetDisplayName ( SIGDN_FILESYSPATH , &pres );

			if ( pres ) {
				bRet = true;
				*pFilePath = pres;
				CoTaskMemFree ( pres );
			}
			pResult->Release ( );
		}

	}

	pDialog->Release ( );
	pCurrent->Release ( );

	return bRet;
}

bool HSGetSaveWaveFile ( std::wstring * pFilePath )
{
	if ( pFilePath == nullptr ) return false;

	bool bRet = false;
	IFileSaveDialog *pDialog;

	HRESULT hr = CoCreateInstance ( CLSID_FileSaveDialog , NULL , CLSCTX_INPROC_SERVER , IID_PPV_ARGS ( &pDialog ) );

	if ( FAILED ( hr ) ) return false;

	wchar_t cPath [ MAX_PATH ];

	GetCurrentDirectoryW ( MAX_PATH , cPath );

	PathAddBackslashW ( cPath );

	IShellItem *pCurrent;

	SHCreateItemFromParsingName ( cPath , NULL , IID_PPV_ARGS ( &pCurrent ) );

	pDialog->SetDefaultFolder ( pCurrent );
	
	wchar_t *pfn = PathFindFileNameW ( pFilePath->c_str ( ) );
	pDialog->SetFileName (pfn );

	COMDLG_FILTERSPEC fs = { L"WAVEファイル(*.wav)" , L"*.wav" };

	pDialog->SetFileTypes ( 1 , &fs );
	pDialog->SetDefaultExtension ( L"wav" );
	pDialog->SetOptions ( FOS_OVERWRITEPROMPT );

	pDialog->SetTitle ( L"変換先のWAVEファイルを選択してください。" );

	HWND hwnd = GetConsoleWindow ( );
	if ( pDialog->Show ( hwnd ) == S_OK ) {

		IShellItem *pResult;
		
		if ( pDialog->GetResult ( &pResult ) == S_OK ) {

			wchar_t *pres = nullptr;
			pResult->GetDisplayName ( SIGDN_FILESYSPATH , &pres );

			if ( pres ) {
				bRet = true;
				*pFilePath = pres;
				CoTaskMemFree ( pres );
			}
			pResult->Release ( );
		}

	}

	pDialog->Release ( );
	pCurrent->Release ( );

	return bRet;

}

void SetExeDirToCurrentDir ( void )
{
	TCHAR ExeFile_Dir [ MAX_PATH + 1 ];

	//実行ファイル名を取得
	GetModuleFileName ( NULL , ExeFile_Dir , sizeof ( TCHAR )*MAX_PATH );

	//ファイル名のみ取り除く
	PathRemoveFileSpec ( ExeFile_Dir );

	//カレントディレクトリに設定
	SetCurrentDirectory ( ExeFile_Dir );

	return;

}

