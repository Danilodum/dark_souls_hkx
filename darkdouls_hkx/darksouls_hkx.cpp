
#include "stdafx.h"
#include "hk_core.h"


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
	class hkLoader* hk_loader;
	//class hkaSkeleton* hk_skeleton;
	class hkaAnimation* hk_anim;
	class hkaAnimationBinding* hk_anim_binding;

	int modelCount = rapi->Noesis_GetLoadedModelCount();
	
	if (modelCount <= 0)
	{
		return NULL;
	}

	noesisModel_t * mdl = rapi->Noesis_GetLoadedModel(0);

	if (!mdl)
	{
		return NULL;
	}
	
	rapi->LogOutput("\ngot something@!\n");

	rapi->LogOutput(rapi->Noesis_GetLastCheckedName());
	rapi->LogOutput("\n");
	rapi->LogOutput(rapi->Noesis_GetInputName());
	rapi->LogOutput("\n");

	sharedModel_t * smdl = rapi->rpgGetSharedModel(mdl, 0);
	int numBones = smdl->numBones;
	modelBone_t * sbones = rapi->Noesis_CopyBones(smdl->bones, smdl->numBones);
	rapi->Noesis_FreeSharedModel(smdl);

	hkMallocAllocator baseMalloc;
	// Need to have memory allocated for the solver. Allocate 1mb for it.
	hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(&baseMalloc, hkMemorySystem::FrameInfo(1024 * 1024));
	hkBaseSystem::init(memoryRouter, errorReport);
	//hkVersionPatchManager patchManager;
	//CustomRegisterPatches(patchManager);
	//hkDefaultClassNameRegistry &defaultRegistry = hkDefaultClassNameRegistry::getInstance();
	//CustomRegisterDefaultClasses();
	//ValidateClassSignatures();

	char* infile = rapi->Noesis_GetLastCheckedName();
	char fn[MAX_NOESIS_PATH];
	rapi->Noesis_GetLocalFileName(fn, infile);

	hkIstream stream(infile);
	hkStreamReader *reader = stream.getStreamReader();

	hkVariant root;
	hkResource *resource;

	hkResult res = hkSerializeLoad(reader, root, resource);

	if (!res.isSuccess())
	{
		rapi->LogOutput("bad file!\n");
		return mdl;
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

	rapi->LogOutput("%s%s", fn, "\n");

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
			//const char* CurrentBoneName = m_skeleton->m_bones[i].m_name;
			//std::string CurBoneNameString = CurrentBoneName;
			//	FbxNode* CurrentJointNode = pScene->GetNode(BoneIDContainer[i]);


			/// GET CURRENT BONE (index?)


			// create curves on frame zero otherwise just get them
			/*if(iFrame == 0)
			{
			// Translation
			lCurve_Trans_X = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
			lCurve_Trans_Y = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
			lCurve_Trans_Z = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

			// Rotation
			lCurve_Rot_X = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
			lCurve_Rot_Y = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
			lCurve_Rot_Z = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
			}
			else
			{
			// Translation
			lCurve_Trans_X = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
			lCurve_Trans_Y = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, false);
			lCurve_Trans_Z = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, false);

			// Rotation
			lCurve_Rot_X = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
			lCurve_Rot_Y = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, false);
			lCurve_Rot_Z = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, false);
			}*/

			hkQsTransform& transform = transformOut[i];
			const hkVector4& anim_pos = transform.getTranslation();
			const hkQuaternion& anim_rot = transform.getRotation();

			// todo support for anything else beside 30 fps?
			//	lTime.SetTime(0,0,0,iFrame,0,0,lTime.eFrames30);

			// Translation first
			/*lCurve_Trans_X->KeyModifyBegin();
			lKeyIndex = lCurve_Trans_X->KeyAdd(lTime);
			lCurve_Trans_X->KeySetValue(lKeyIndex, anim_pos.getSimdAt(0));
			lCurve_Trans_X->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationCubic);
			lCurve_Trans_X->KeyModifyEnd();


			lCurve_Trans_Y->KeyModifyBegin();
			lKeyIndex = lCurve_Trans_Y->KeyAdd(lTime);
			lCurve_Trans_Y->KeySetValue(lKeyIndex, anim_pos.getSimdAt(1));
			lCurve_Trans_Y->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationCubic);
			lCurve_Trans_Y->KeyModifyEnd();

			lCurve_Trans_Z->KeyModifyBegin();
			lKeyIndex = lCurve_Trans_Z->KeyAdd(lTime);
			lCurve_Trans_Z->KeySetValue(lKeyIndex, anim_pos.getSimdAt(2));
			lCurve_Trans_Z->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationCubic);
			lCurve_Trans_Z->KeyModifyEnd();
			*/
			float px = anim_pos.getComponent(0);
			float py = anim_pos.getComponent(1);
			float pz = anim_pos.getComponent(2);

			//rapi->LogOutput("pos bone %003d: (%f, %f, %f)\n", i, px, py, pz);

			// Rotation
			float rx = anim_rot.m_vec.getComponent(0);
			float ry = anim_rot.m_vec.getComponent(1);
			float rz = anim_rot.m_vec.getComponent(2);
			float rw = anim_rot.m_vec.getComponent(3);

			//rapi->LogOutput("rot bone %003d: (%f, %f, %f, %f)\n", i, rx, ry, rz, rw);

			RichQuat trot = RichQuat(rx, ry, rz, rw);
			RichVec3 ttrn = RichVec3(px, py, pz);
			key_t t = key_t();
			t.rot = trot;
			t.trn = ttrn;
			AllKeys.push_back(t);
			/*Quat QuatRotNew = {anim_rot.m_vec.getSimdAt(0), anim_rot.m_vec.getSimdAt(1), anim_rot.m_vec.getSimdAt(2), anim_rot.m_vec.getSimdAt(3)};
			EulerAngles inAngs_Animation = Eul_FromQuat(QuatRotNew, EulOrdXYZs);

			lCurve_Rot_X->KeyModifyBegin();
			lKeyIndex = lCurve_Rot_X->KeyAdd(lTime);
			lCurve_Rot_X->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.x)));
			lCurve_Rot_X->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationCubic);
			lCurve_Rot_X->KeyModifyEnd();

			lCurve_Rot_Y->KeyModifyBegin();
			lKeyIndex = lCurve_Rot_Y->KeyAdd(lTime);
			lCurve_Rot_Y->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.y)));
			lCurve_Rot_Y->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationCubic);
			lCurve_Rot_Y->KeyModifyEnd();

			lCurve_Rot_Z->KeyModifyBegin();
			lKeyIndex = lCurve_Rot_Z->KeyAdd(lTime);
			lCurve_Rot_Z->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.z)));
			lCurve_Rot_Z->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationCubic);
			lCurve_Rot_Z->KeyModifyEnd();*/
		}
	}
	float scale = 1000;
	//rapi->rpgSetOption(RPGOPT_SWAPHANDEDNESS, true);
	//modelBone_t * mb = smdl->bones;
	//int numBones = smdl->numBones;
	//convert anim keys to matrices
	RichMat43 *mats = (RichMat43 *) rapi->Noesis_UnpooledAlloc(sizeof(RichMat43)*numAnimKeys);
	for (int i = 0; i < numAnimKeys; i++)
	{
	//	psaAnimKey_t *key = animKeys + i;

		RichQuat q = AllKeys[i].rot;
		modelBone_t *bone = sbones + (i % numBones);
		if (bone->eData.parent)
		{ //flip handedness from ut
			q[0] = q[0];
			q[1] = q[1];
			q[2] = q[2];
		}
		mats[i] = q.ToMat43(true);
		RichVec3 scaled_trans = AllKeys[i].trn;
		scaled_trans *= scale;
		mats[i][3] = scaled_trans;//AllKeys[i].trn;
	}

	int totalFrames = FrameNumber;
	float frameRate = 30;
	noesisAnim_t *na = rapi->rpgAnimFromBonesAndMatsFinish(sbones, numBones, (modelMatrix_t *) mats, totalFrames, frameRate);
	rapi->Noesis_UnpooledFree(mats);
	if (na)
	{
		na->aseq = rapi->Noesis_AnimSequencesAlloc(1, totalFrames);
		for (int i = 0; i < 1; i++)
		{ //fill in the sequence info
			//psaAnimInfo_t *animInfo = animInfos + i;
			noesisASeq_t *seq = na->aseq->s + i;

			seq->name = rapi->Noesis_PooledString(fn);
			seq->startFrame = 0;
			seq->endFrame = 0 + FrameNumber - 1;
			seq->frameRate = frameRate;
		}

		numMdl = 1; //this is still important to set for animation output
		//static float mdlAngOfs[3] = { 0.0f, -90.0f, 0.0f };
		//rapi->SetPreviewAngOfs(mdlAngOfs); //this'll rotate the model (only in preview mode) into ut-friendly coordinates
		//return rapi->Noesis_AllocModelContainer(NULL, na, 1);
	}

	rapi->Noesis_SetModelAnims(mdl, /*noesisAnim_t * */na,/* int numAnims*/ 1);
    rapi->LogOutput("reached hERE!");

	return mdl;
}