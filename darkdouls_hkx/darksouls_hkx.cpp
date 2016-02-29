
#include "stdafx.h"

#include <iostream>

const char *g_pPluginName = "darksouls_hkx";
const char *g_pPluginDesc = "HKX dark souls animation format handler, by Snaz.";


//called by Noesis to init the plugin
bool NPAPI_InitLocal(void)
{
	int th = g_nfn->NPAPI_Register("HKX dark souls anims", ".damnhavok");
	if (th < 0)
	{
		return false;
	}

	g_nfn->NPAPI_PopupDebugLog(0);

	//set the data handlers for this format
	g_nfn->NPAPI_SetTypeHandler_TypeCheck(th, Anim_DS_Check);
	g_nfn->NPAPI_SetTypeHandler_LoadModel(th, Anim_DS_Load);

	return true;
}

//called by Noesis before the plugin is freed
void NPAPI_ShutdownLocal(void)
{
	//nothing to do in this plugin
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	return TRUE;
}

bool Anim_DS_Check(BYTE *fileBuffer, int bufferLen, noeRAPI_t *rapi)
{

	return true;
}

noesisModel_t *Anim_DS_Load(BYTE *fileBuffer, int bufferLen, int &numMdl, noeRAPI_t *rapi)
{
	//for vanquish, append any matching dtt files (they're just paired dat files)
	char *inFile = rapi->Noesis_GetInputName();
	BYTE *skeletonFile = NULL;
	xmlSkeleton_t xml;
	xmlAnim_t xml_anim;
	bool bparsedXml = false;
	bool bparsedAnim = false;
	if (inFile && inFile[0])
	{
		char fn[MAX_NOESIS_PATH];
		rapi->Noesis_GetExtensionlessName(fn, inFile);
		rapi->Noesis_GetDirForFilePath(fn, inFile);
		strcat_s(fn, MAX_NOESIS_PATH, "Skeleton-out.hkx");

		// parsing xml
		bparsedXml = ParseSkeleton(&xml, fn, rapi);
		bparsedAnim = ParseAnim(&xml_anim, inFile, rapi);
	}

	if (!bparsedXml || !bparsedAnim)
	{
		rapi->LogOutput("An error occured while parsing the skeleton xml file...");
		return NULL;
	}

	modelBone_t * bones = rapi->Noesis_AllocBones(xml.numBones);

	for (int i = 0; i < xml.numBones; i++)
	{
		modelBone_t *bone = bones + i;

		short parent = xml.parentindices[i];
		bone->index = i;
		bone->eData.parent = (parent >= 0) ? bones + parent : NULL;
		sprintf_s(bone->name, xml.names[i].length() + 1, xml.names[i].c_str());

		g_mfn->Math_QuatToMat(xml.quat[i].q, &bone->mat, false, false);

		g_mfn->Math_VecScaleVec(bone->mat.x1, xml.scl[i].v);
		g_mfn->Math_VecScaleVec(bone->mat.x2, xml.scl[i].v);
		g_mfn->Math_VecScaleVec(bone->mat.x3, xml.scl[i].v);
		g_mfn->Math_VecCopy(xml.trn[i].v, bone->mat.o);
	}

	rapi->rpgMultiplyBones(bones, xml.numBones);

	//char* infile = rapi->Noesis_GetInputName();
	char anim_name[MAX_NOESIS_PATH];
	rapi->Noesis_GetLocalFileName(anim_name, inFile);
	std::string anim_name_t;
	anim_name_t.append(anim_name);
	for (int i = 0; i < 10; ++i)
	{
		anim_name_t.pop_back();
	}

	strcpy_s(anim_name, anim_name_t.c_str());

	int FrameNumber = xml_anim.FrameNumber;
	int TrackNumber = xml_anim.TrackNumber;
	int FloatNumber = xml_anim.FloatNumber;

	float AnimDuration = xml_anim.AnimDuration;
	float incrFrame = xml_anim.incFrame;
	int numAnimKeys = xml_anim.numAnimKeys;

	//convert anim keys to matrices
	bool printed = false;
	RichMat43 *mats = (RichMat43 *) rapi->Noesis_UnpooledAlloc(sizeof(RichMat43)*numAnimKeys);
	for (int i = 0; i < numAnimKeys; i++)
	{
		RichQuat q = xml_anim.keys[i].rot;
		modelBone_t *bone = bones + (i % xml.numBones);
		mats[i] = q.ToMat43(true);
		mats[i][3] = xml_anim.keys[i].trn;

		mats[i][1][0] = -mats[i][1][0];
		mats[i][2][0] = -mats[i][2][0];
		mats[i][3][0] = -mats[i][3][0];
		mats[i][0][1] = -mats[i][0][1];
		mats[i][0][2] = -mats[i][0][2];
	}

	int totalFrames = FrameNumber;
	float frameRate = 30;
	noesisAnim_t *na = rapi->rpgAnimFromBonesAndMatsFinish(bones, xml.numBones, (modelMatrix_t *) mats, totalFrames, frameRate);
	rapi->Noesis_UnpooledFree(mats);
	if (na)
	{
		na->aseq = rapi->Noesis_AnimSequencesAlloc(1, totalFrames);
		for (int i = 0; i < 1; i++)
		{ //fill in the sequence info
			noesisASeq_t *seq = na->aseq->s + i;

			seq->name = rapi->Noesis_PooledString(anim_name);
			seq->startFrame = 0;
			seq->endFrame = 0 + FrameNumber - 1;
			seq->frameRate = frameRate;
		}

		numMdl = 1;
	}

	noesisModel_t * my_mdl = rapi->Noesis_AllocModelContainer(NULL, na, 1);

	return my_mdl;
}

bool ParseSkeleton(xmlSkeleton_t * xml, const char * filePath, noeRAPI_t *rapi)
{
	xml->names.clear();
	xml->parentindices.clear();
	xml->trn.clear();
	xml->quat.clear();
	xml->scl.clear();

	TiXmlNode* hkobject = 0;
	TiXmlElement* hkparam = 0;
	TiXmlElement* todoElement = 0;
	TiXmlElement* itemElement = 0;

	TiXmlDocument doc(filePath);
	bool loadOkay = doc.LoadFile();

	if (!loadOkay)
	{
		rapi->LogOutput("Exiting, opening xml skeleton file failed\n");
		return false;
	}

	rapi->LogOutput("Loaded xml skeleton file.\n");
	
	hkobject = doc.FirstChild("hkpackfile")->FirstChild("hksection")->FirstChild("hkobject");
	hkparam = hkobject->FirstChildElement("hkparam")->NextSiblingElement("hkparam");
	if (strcmp(hkparam->Attribute("name"), "parentIndices") != 0)
		return false;

	xml->numBones = atoi(hkparam->Attribute("numelements"));

	const char * c_strIndices = hkparam->GetText();

	int write_length = 0;
	int write_start = 0;
	for (unsigned int i = 0; i <= strlen(c_strIndices); ++i)
	{
		char tmp = c_strIndices[i];

		if (!isspace(tmp))
		{
			if (write_length == 0)
				write_start = i;
			++write_length;
		} else
		{
			if (write_length > 0)
			{
				std::stringstream ss;
				std::string tmp_s;
				while (write_start != i)
				{
					ss << c_strIndices[write_start];
					++write_start;
				}
				ss >> tmp_s;
				tmp_s.append("\n");
				//rapi->LogOutput(tmp_s.c_str());
				xml->parentindices.push_back(atoi(tmp_s.c_str()));
				write_length = 0;
			}
		}
		if (i == strlen(c_strIndices) && write_length > 0)
		{
			std::stringstream ss;
			std::string tmp_s;
			while (write_start != i)
			{
				ss << c_strIndices[write_start];
				++write_start;
			}
			ss >> tmp_s;
			tmp_s.append("\n");
			//rapi->LogOutput(tmp_s.c_str());
			xml->parentindices.push_back(atoi(tmp_s.c_str()));
			write_length = 0;
		}
	}
	//rapi->LogOutput(std::to_string(xml->parentindices.size()).c_str());

	hkparam = hkparam->NextSiblingElement("hkparam");
	if (!hkparam || strcmp(hkparam->Attribute("name"), "bones") != 0)
		return false;
	TiXmlNode * hkobjtmp = hkparam->FirstChild("hkobject");
	for (int i = 0; i < xml->numBones; ++i)
	{
		TiXmlElement * hkp = hkobjtmp->FirstChildElement("hkparam");
		xml->names.push_back(hkp->GetText());
		if (i + 1 != xml->numBones)
		{
			hkobjtmp = hkobjtmp->NextSibling();
		}
	}

	for (int i = 0; i < xml->names.size(); ++i)
	{
		std::string tmp = xml->names[i];
		tmp.push_back('\n');
		rapi->LogOutput(tmp.c_str());
	}
	//rapi->LogOutput(std::to_string(xml->names.size()).c_str());

	hkparam = hkparam->NextSiblingElement("hkparam");
	if (!hkparam || strcmp(hkparam->Attribute("name"), "referencePose") != 0)
		return false;

	const char * c_strRefs = hkparam->GetText();
	std::stringstream ss;
	std::string s;
	for (unsigned int i = 0; i <= strlen(c_strRefs); ++i)
	{
		s.push_back(c_strRefs[i]);
		ss << c_strRefs[i];
	}
	//ss >> s;
	//rapi->LogOutput(s.c_str());

	std::vector<std::string> delimited_refs;
	Split(s, ' ', delimited_refs);

	for (unsigned int i = 0; i < delimited_refs.size(); ++i)
	{
		i = ExtractTrn(delimited_refs, i, xml);
		i = ExtractQuat(delimited_refs, i, xml);
		i = ExtractScl(delimited_refs, i, xml);
	}

	return true;
}

std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return elems;
}

unsigned int ExtractTrn(std::vector<std::string> & v, unsigned int i, xmlSkeleton_t * xml)
{
	std::string xs, ys, zs;

	std::vector<std::string> xv;
	Split(v[i], '(', xv);

	xs = xv[xv.size() - 1];
	++i;

	ys = v[i];
	++i;

	std::vector<std::string> zv;
	Split(v[i], ')', zv);

	zs = zv[0];

	RichVec3 trn(std::stof(xs), std::stof(ys), std::stof(zs));
	xml->trn.push_back(trn);

	return i;
}

unsigned int ExtractQuat(std::vector<std::string> & v, unsigned int i, xmlSkeleton_t * xml)
{
	std::string xs, ys, zs, ws;

	std::vector<std::string> xv;
	Split(v[i], '(', xv);

	xs = xv[xv.size() - 1];
	++i;

	ys = v[i];
	++i;

	zs = v[i];
	++i;

	std::vector<std::string> wv;
	Split(v[i], ')', wv);

	ws = wv[0];

	RichQuat quat(std::stof(xs), std::stof(ys), std::stof(zs), std::stof(ws));
	xml->quat.push_back(quat);

	return i;
}

unsigned int ExtractScl(std::vector<std::string> & v, unsigned int i, xmlSkeleton_t * xml)
{
	std::string xs, ys, zs;

	std::vector<std::string> xv;
	Split(v[i], '(', xv);

	xs = xv[xv.size() - 1];
	++i;

	ys = v[i];
	++i;

	std::vector<std::string> zv;
	Split(v[i], ')', zv);

	zs = zv[0];

	RichVec3 scl(std::stof(xs), std::stof(ys), std::stof(zs));
	xml->scl.push_back(scl);

	return i;
}

bool ParseAnim(xmlAnim_t *xml, const char * filePath, noeRAPI_t *rapi)
{
	xml->keys.clear();

	TiXmlDocument doc(filePath);
	bool loadOkay = doc.LoadFile();

	if (!loadOkay)
	{
		rapi->LogOutput("Exiting, opening xml animation file failed\n");
		return false;
	}

	rapi->LogOutput("Loaded xml animation file.\n");

	TiXmlElement* data = doc.FirstChildElement("anim")->FirstChildElement("data");

	xml->FrameNumber = atoi(data->Attribute("FrameNumber"));
	xml->TrackNumber = atoi(data->Attribute("TrackNumber"));
	xml->FloatNumber = std::stof(data->Attribute("FloatNumber"));
	xml->AnimDuration = std::stof(data->Attribute("AnimDuration"));
	xml->numAnimKeys = atoi(data->Attribute("numAnimKeys"));
	xml->incFrame = std::stof(data->Attribute("incFrame"));

	rapi->LogOutput("no of frames: %d\n", xml->FrameNumber);
	rapi->LogOutput("no of ttracks: %d\n", xml->TrackNumber);
	rapi->LogOutput("no of ftracks: %d\n", xml->FloatNumber);
	rapi->LogOutput("anim duration: %f\n", xml->AnimDuration);
	rapi->LogOutput("incrFrame: %f\n", xml->incFrame);
	rapi->LogOutput("incrFrame: %d\n", xml->numAnimKeys);


	TiXmlElement* key = data->FirstChildElement("key");
	for (int i = 0; i < xml->numAnimKeys; ++i)
	{
		key_t animKey;
		
		animKey.trn = RichVec3(std::stof(key->Attribute("px")), std::stof(key->Attribute("py")), std::stof(key->Attribute("pz")));
		animKey.rot = RichQuat(std::stof(key->Attribute("rx")), std::stof(key->Attribute("ry")), std::stof(key->Attribute("rz")), std::stof(key->Attribute("rw")));
		xml->keys.push_back(animKey);

		key = key->NextSiblingElement();
	}

	return true;
}