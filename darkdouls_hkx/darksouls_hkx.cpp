
#include "stdafx.h"
#include "hk_core.h"
#include <iostream>

const char *g_pPluginName = "darksouls_hkx";
const char *g_pPluginDesc = "HKX dark souls animation format handler, by Snaz.";

hkMemoryRouter* memoryRouter;

//called by Noesis to init the plugin
bool NPAPI_InitLocal(void)
{

	memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
	hkBaseSystem::init(memoryRouter, errorReport);
	
	int th = g_nfn->NPAPI_Register("HKX dark souls anims", ".hkx");
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
	bool bparsedXml = false;
	if (inFile && inFile[0])
	{
		char fn[MAX_NOESIS_PATH];
		rapi->Noesis_GetExtensionlessName(fn, inFile);
		rapi->Noesis_GetDirForFilePath(fn, inFile);
		strcat_s(fn, MAX_NOESIS_PATH, "Skeleton-out.hkx");

		// parsing xml
		bparsedXml = ParseSkeleton(&xml, fn, rapi);

	}

	if (!bparsedXml)
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

	hkIstream stream(inFile);
	hkStreamReader *reader = stream.getStreamReader();

	hkVariant root;
	hkResource *resource;

	hkResult res = hkSerializeLoad(reader, root, resource);

	if (!res.isSuccess())
	{
		rapi->LogOutput("bad file!\n");
		return NULL;
	}

	hkRootLevelContainer * scene = resource->getContents<hkRootLevelContainer>();
	hkaAnimationContainer *ac = scene->findObject<hkaAnimationContainer>();

	hkaAnimation * m_animation = ac->m_animations[0];
	hkaAnimationBinding * m_binding = ac->m_bindings[0];

	int FrameNumber = m_animation->getNumOriginalFrames();
	int TrackNumber = m_animation->m_numberOfTransformTracks;
	int FloatNumber = m_animation->m_numberOfFloatTracks;

	float AnimDuration = m_animation->m_duration;
	hkReal incrFrame = m_animation->m_duration / (hkReal) (FrameNumber - 1);

	rapi->LogOutput("no of frames: %d\n", FrameNumber);
	rapi->LogOutput("no of ttracks: %d\n", TrackNumber);
	rapi->LogOutput("no of ftracks: %d\n", FloatNumber);
	rapi->LogOutput("anim duration: %f\n", AnimDuration);
	rapi->LogOutput("incrFrame: %f\n", incrFrame);

	rapi->LogOutput("%s%s", anim_name, "\n");

	hkLocalArray<float> floatsOut(FloatNumber);
	hkLocalArray<hkQsTransform> transformOut(TrackNumber);
	floatsOut.setSize(FloatNumber);
	transformOut.setSize(TrackNumber);
	hkReal startTime = 0.0;

	hkArray<hkInt16> tracks;
	tracks.setSize(TrackNumber);
	for (int i = 0; i<TrackNumber; ++i) tracks[i] = i;

	hkReal time = startTime;

	int numAnimKeys = FrameNumber * TrackNumber;

	std::vector<key_t> AllKeys = std::vector<key_t>(numAnimKeys);
	AllKeys.clear();
	
	for (int iFrame = 0; iFrame<FrameNumber; ++iFrame, time += incrFrame)
	{
		m_animation->samplePartialTracks(time, TrackNumber, transformOut.begin(), FloatNumber, floatsOut.begin());
		hkaSkeletonUtils::normalizeRotations(transformOut.begin(), TrackNumber);

		// assume 1-to-1 transforms
		// loop through animated bones
		for (int i = 0; i<TrackNumber; ++i)
		{
			hkQsTransform& transform = transformOut[i];
			const hkVector4& anim_pos = transform.getTranslation();
			const hkQuaternion& anim_rot = transform.getRotation();

			// Translation 
			float px = anim_pos.getComponent(0);
			float py = anim_pos.getComponent(1);
			float pz = anim_pos.getComponent(2);

			// Rotation
			float rx = anim_rot.m_vec.getComponent(0);
			float ry = anim_rot.m_vec.getComponent(1);
			float rz = anim_rot.m_vec.getComponent(2);
			float rw = anim_rot.m_vec.getComponent(3);

			RichQuat trot = RichQuat(rx, ry, rz, rw);
			RichVec3 ttrn = RichVec3(px, py, pz);
			key_t t = key_t();
			t.rot = trot;
			t.trn = ttrn;
			AllKeys.push_back(t);
		}
	}
	float scale = 100;
	//convert anim keys to matrices
	bool printed = false;
	RichMat43 *mats = (RichMat43 *) rapi->Noesis_UnpooledAlloc(sizeof(RichMat43)*numAnimKeys);
	for (int i = 0; i < numAnimKeys; i++)
	{
		RichQuat q = AllKeys[i].rot;
		modelBone_t *bone = bones + (i % xml.numBones);
		mats[i] = q.ToMat43(true);
		RichVec3 scaled_trans = AllKeys[i].trn;
		scaled_trans *= scale;
		mats[i][3] = scaled_trans;

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
	//noesisModel_t * mdl = rapi->Noesis_GetLoadedModel(0);
	/*sharedModel_t * shared_mdl = rapi->rpgGetSharedModel(my_mdl, 0);

	shared_mdl->numBones = xml.numBones;
	shared_mdl->bones = bones;

	noesisModel_t * final_mdl = rapi->Noesis_ModelFromSharedModel(shared_mdl);

	//rapi->Noesis_FreeModels(my_mdl, 1);*/

	//rapi->Noesis_SetModelAnims(final_mdl, /*noesisAnim_t * */na,/* int numAnims*/ 1);
    rapi->LogOutput("reached hERE!");

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

hkResource* hkSerializeUtilLoad(hkStreamReader* stream
	, hkSerializeUtil::ErrorDetails* detailsOut/*=HK_NULL*/
	, const hkClassNameRegistry* classReg/*=HK_NULL*/
	, hkSerializeUtil::LoadOptions options/*=hkSerializeUtil::LOAD_DEFAULT*/)
{
	__try
	{
		return hkSerializeUtil::load(stream, detailsOut, options);
	} __except (EXCEPTION_EXECUTE_HANDLER)
	{
		//if (detailsOut == NULL)
		//	detailsOut->id = hkSerializeUtil::ErrorDetails::ERRORID_LOAD_FAILED;
		return NULL;
	}
}

void HK_CALL errorReport(const char* msg, void* userContext)
{
	std::cout << msg << std::endl;
}

hkResult hkSerializeLoad(hkStreamReader *reader
	, hkVariant &root
	, hkResource *&resource)
{
	hkTypeInfoRegistry &defaultTypeRegistry = hkTypeInfoRegistry::getInstance();
	hkDefaultClassNameRegistry &defaultRegistry = hkDefaultClassNameRegistry::getInstance();

	hkBinaryPackfileReader bpkreader;
	hkXmlPackfileReader xpkreader;
	resource = NULL;
	hkSerializeUtil::FormatDetails formatDetails;
	hkSerializeUtil::detectFormat(reader, formatDetails);
	hkBool32 isLoadable = hkSerializeUtil::isLoadable(reader);
	if (!isLoadable && formatDetails.m_formatType != hkSerializeUtil::FORMAT_TAGFILE_XML)
	{
		return HK_FAILURE;
	} else
	{
		switch (formatDetails.m_formatType)
		{
		case hkSerializeUtil::FORMAT_PACKFILE_BINARY:
		{
			bpkreader.loadEntireFile(reader);
			bpkreader.finishLoadedObjects(defaultTypeRegistry);
			if (hkPackfileData* pkdata = bpkreader.getPackfileData())
			{
				hkArray<hkVariant>& obj = bpkreader.getLoadedObjects();
				for (int i = 0, n = obj.getSize(); i<n; ++i)
				{
					hkVariant& value = obj[i];
					if (value.m_class->hasVtable())
						defaultTypeRegistry.finishLoadedObject(value.m_object, value.m_class->getName());
				}
				resource = pkdata;
				resource->addReference();
			}
			root = bpkreader.getTopLevelObject();
		}
		break;

		case hkSerializeUtil::FORMAT_PACKFILE_XML:
		{
			xpkreader.loadEntireFileWithRegistry(reader, &defaultRegistry);
			if (hkPackfileData* pkdata = xpkreader.getPackfileData())
			{
				hkArray<hkVariant>& obj = xpkreader.getLoadedObjects();
				for (int i = 0, n = obj.getSize(); i<n; ++i)
				{
					hkVariant& value = obj[i];
					if (value.m_class->hasVtable())
						defaultTypeRegistry.finishLoadedObject(value.m_object, value.m_class->getName());
				}
				resource = pkdata;
				resource->addReference();
				root = xpkreader.getTopLevelObject();
			}
		}
		break;

		case hkSerializeUtil::FORMAT_TAGFILE_BINARY:
		case hkSerializeUtil::FORMAT_TAGFILE_XML:
		default:
		{
			hkSerializeUtil::ErrorDetails detailsOut;
			hkSerializeUtil::LoadOptions loadflags = hkSerializeUtil::LOAD_FAIL_IF_VERSIONING;
			resource = hkSerializeUtilLoad(reader, &detailsOut, &defaultRegistry, loadflags);
			root.m_object = resource->getContents<hkRootLevelContainer>();
			if (root.m_object != NULL)
				root.m_class = &((hkRootLevelContainer*) root.m_object)->staticClass();
		}
		break;
		}
	}
	return root.m_object != NULL ? HK_SUCCESS : HK_FAILURE;
}