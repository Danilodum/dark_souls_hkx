
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

	int modelCount = rapi->Noesis_GetLoadedModelCount();
	
	if (modelCount <= 0)
	{
		return NULL;
	}
	CArrayList<noesisModel_t *> models;

	noesisModel_t * mdl = rapi->Noesis_GetLoadedModel(0);

	if (!mdl)
	{
		return NULL;
	}
	
	rapi->LogOutput("\ngot something@!\n");

	rapi->Noesis_GetLastCheckedName();

	//convert anim keys to matrices
	/*RichMat43 *mats = (RichMat43 *) rapi->Noesis_UnpooledAlloc(sizeof(RichMat43)*numAnimKeys);
	for (int i = 0; i < numAnimKeys; i++)
	{
		psaAnimKey_t *key = animKeys + i;
		RichQuat q = key->rot;
		modelBone_t *bone = b + (i%numBones);
		if (bone->eData.parent)
		{ //flip handedness from ut
			q[0] = -q[0];
			q[1] = -q[1];
			q[2] = -q[2];
		}
		mats[i] = q.ToMat43(true);
		mats[i][3] = key->trans;
	}

	int totalFrames = numAnimKeys / numBones;
	float frameRate = animInfos[0].frameRate;
	noesisAnim_t *anims = rapi->rpgAnimFromBonesAndMatsFinish(b, numBones, (modelMatrix_t *) mats, totalFrames, frameRate);
	rapi->Noesis_UnpooledFree(mats);
	if (na)
	{
		na->aseq = rapi->Noesis_AnimSequencesAlloc(numAnimInfos, totalFrames);
		for (int i = 0; i < numAnimInfos; i++)
		{ //fill in the sequence info
			psaAnimInfo_t *animInfo = animInfos + i;
			noesisASeq_t *seq = na->aseq->s + i;

			seq->name = rapi->Noesis_PooledString(animInfo->name);
			seq->startFrame = animInfo->firstFrame;
			seq->endFrame = animInfo->firstFrame + animInfo->numFrames - 1;
			seq->frameRate = animInfo->frameRate;
		}

		numMdl = 1; //this is still important to set for animation output
		static float mdlAngOfs[3] = { 0.0f, -90.0f, 0.0f };
		rapi->SetPreviewAngOfs(mdlAngOfs); //this'll rotate the model (only in preview mode) into ut-friendly coordinates
		return rapi->Noesis_AllocModelContainer(NULL, na, 1);
	}*/

	//rapi->Noesis_SetModelAnims(mdl, /*noesisAnim_t * */anims,/* int numAnims*/ 1);

	numMdl = 1;

	models.Append(mdl);

	noesisModel_t *mdlList = rapi->Noesis_ModelsFromList(models, numMdl);
	models.Clear();

	return mdlList;
}