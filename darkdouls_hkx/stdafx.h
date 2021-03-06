// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "../../pluginshare.h"

#include "tinyxml.h"

#include <string>
#include <vector>
#include <sstream>

#define MAX_BONES 500

typedef struct xmlSkeleton_s
{
	std::vector<RichVec3> trn;
	std::vector<RichQuat> quat;
	std::vector<RichVec3> scl;

	int numBones;

	std::vector<int> parentindices;
	std::vector<std::string> names;

	xmlSkeleton_s::xmlSkeleton_s()
	{
		parentindices = std::vector<int>(MAX_BONES);
		names = std::vector<std::string>(MAX_BONES);
		trn = std::vector<RichVec3>(MAX_BONES);
		quat = std::vector<RichQuat>(MAX_BONES);
		scl = std::vector<RichVec3>(MAX_BONES);
	};

} xmlSkeleton_t;

typedef struct key_s
{
	RichQuat rot;
	RichVec3 trn;
} key_t;

typedef struct xmlAnim_s
{
	int FrameNumber;
	int TrackNumber;
	int FloatNumber;
	float AnimDuration;
	int numAnimKeys;
	float incFrame;

	std::vector<key_t> keys;

	xmlAnim_s::xmlAnim_s()
	{
		keys = std::vector<key_t>(10000);
	};
} xmlAnim_t;

bool Anim_DS_Check(BYTE *fileBuffer, int bufferLen, noeRAPI_t *rapi);
noesisModel_t *Anim_DS_Load(BYTE *fileBuffer, int bufferLen, int &numMdl, noeRAPI_t *rapi);
bool ParseSkeleton(xmlSkeleton_t * xml, const char * filePath, noeRAPI_t *rapi);

extern mathImpFn_t *g_mfn;
extern noePluginFn_t *g_nfn;

std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems);

unsigned int ExtractTrn(std::vector<std::string> & v, unsigned int i, xmlSkeleton_t * xml);
unsigned int ExtractQuat(std::vector<std::string> & v, unsigned int i, xmlSkeleton_t * xml);
unsigned int ExtractScl(std::vector<std::string> & v, unsigned int i, xmlSkeleton_t * xml);

bool ParseAnim(xmlAnim_t *xml, const char * inFile, noeRAPI_t *rapi);