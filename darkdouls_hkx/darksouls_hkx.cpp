
#include "stdafx.h"


const char *g_pPluginName = "darksouls_hkx";
const char *g_pPluginDesc = "HKX dark souls animation format handler, by Snaz.";



//called by Noesis to init the plugin
bool NPAPI_InitLocal(void)
{
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


	CArrayList<noesisModel_t *> models;

	void *pgctx = rapi->rpgCreateContext();


	modelBone_t * bones = rapi->Noesis_AllocBones(xml.numBones);

	for (int i = 0; i < xml.numBones; i++)
	{
		modelBone_t *bone = bones + i;
		
		short parent = xml.parentindices[i];
		bone->index = i;
		bone->eData.parent = (parent >= 0) ? bones + parent : NULL;
		sprintf_s(bone->name, xml.names[i].length() + 1, xml.names[i].c_str());
		
		g_mfn->Math_QuatToMat(xml.quat[i].q, &bone->mat, false, false);

		/*
		modelMatrix_t mat1, mat2, mat3;
		g_mfn->Math_RotationMatrix(rot[0], 0, &mat1);
		g_mfn->Math_RotationMatrix(-rot[1], 1, &mat2);
		g_mfn->Math_MatrixMultiply(&mat2, &mat1, &mat3);
		g_mfn->Math_RotationMatrix(rot[2], 2, &mat1);
		g_mfn->Math_MatrixMultiply(&mat3, &mat1, &bone->mat);
		*/
		g_mfn->Math_VecScaleVec(bone->mat.x1, xml.scl[i].v);
		g_mfn->Math_VecScaleVec(bone->mat.x2, xml.scl[i].v);
		g_mfn->Math_VecScaleVec(bone->mat.x3, xml.scl[i].v);
		g_mfn->Math_VecCopy(xml.trn[i].v, bone->mat.o);


	}

	rapi->rpgSetExData_Bones(bones, xml.numBones);
	
	noesisModel_t * mdl = rapi->rpgConstructModel();
	

	rapi->rpgDestroyContext(pgctx);

	if (mdl)
	{
		models.Append(mdl);
	}
	/*noesisModel_t *mdl = Model_Bayo_LoadModel(dfiles, df, rapi);
	if (mdl)
	{
		models.Append(mdl);
	}*/

	if (models.Num() <= 0)
	{
		return NULL;
	}
	noesisModel_t *mdlList = rapi->Noesis_ModelsFromList(models, numMdl);
	models.Clear();

	
	return mdlList;
}

bool ParseSkeleton(xmlSkeleton_t * xml, const char * filePath, noeRAPI_t *rapi)
{
	xml->names.clear();
	xml->parentindices.clear();
	xml->trn.clear();
	xml->quat.clear();
	xml->scl.clear();

	/*

	for hkobject in soup.findAll('hkobject'):
	for hkparam in hkobject.findAll('hkparam'):
	if hkparam['name'] == "parentIndices":
	x.numBones = int(hkparam['numelements'])
	#		logd.write("numbones: " + str(x.numBones) + "\n")
	indices = hkparam.string
	indices = indices.replace("\t","")
	indices = indices.split("\n")
	for j in range(0, len(indices)):
	if len(indices[j]) > 0:
	nums = indices[j].split(" ")
	for num in nums:
	x.parentidx.append(int(num))
	if hkparam['name'] == "bones":
	for hko in hkparam.findAll('hkobject'):
	for hkp in hko.findAll('hkparam'):
	x.names.append(hkp.string)
	break
	#		logd.write("names: " +str(len(x.names)) + " " + str(x.names) + "\n")
	if hkparam['name'] == "referencePose":
	poses = hkparam.string
	poses = poses.split(")")
	#		logd.write("poses: " + str(poses) + "\n")
	for j in range(0, x.numBones):
	k = j*3
	ref = []
	temp = poses[k].replace("\t","").replace("\n","").replace("(","")
	temp = temp.split(" ")
	tempf1 = float(temp[0])
	tempf2 = float(temp[1])
	tempf3 = float(temp[2])
	ref.append(tempf1)
	ref.append(tempf2)
	ref.append(tempf3)

	temp = poses[k+1].replace("\t","").replace("\n","").replace("(","")
	temp = temp.split(" ")
	tempf1 = float(temp[0])
	tempf2 = float(temp[1])
	tempf3 = float(temp[2])
	tempf4 = float(temp[3])
	ref.append(tempf1)
	ref.append(tempf2)
	ref.append(tempf3)
	ref.append(tempf4)

	temp = poses[k+2].replace("\t","").replace("\n","").replace("(","")
	temp = temp.split(" ")
	tempf1 = float(temp[0])
	tempf2 = float(temp[1])
	tempf3 = float(temp[2])
	ref.append(tempf1)
	ref.append(tempf2)
	ref.append(tempf3)

	x.refs.append(ref)
	break

	*/

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
	/*for (std::vector<int>::iterator it = xml->parentindices.begin(); it != xml->parentindices.end(); ++it)
	{
	std::string tmp;
	tmp = std::to_string(*it);
	tmp.append("\n");
	rapi->LogOutput(t*mp.c_str());
	}*/
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
		/*std::string tmp = *it;
		tmp.push_back('\n');
		rapi->LogOutput(tmp.c_str());*/
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