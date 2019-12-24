/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team


All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file  FBXDConverter.h
 *  @brief FBX DOM to aiScene conversion
 */
#ifndef INCLUDED_AI_FBX_CONVERTER_H
#define INCLUDED_AI_FBX_CONVERTER_H

#include "FBXDocument.h"
#include "FBXImporter.h"
#include "FBXMeshGeometry.h"
#include "FBXParser.h"
#include "FBXProperties.h"
#include "FBXUtil.h"

#include <assimp/StringComparison.h>
#include <assimp/anim.h>
#include <assimp/camera.h>
#include <assimp/light.h>
#include <assimp/material.h>
#include <assimp/texture.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

struct aiScene;
struct aiNode;
struct aiMaterial;

struct morphKeyData {
	std::vector<unsigned int> values;
	std::vector<float> weights;
};
typedef std::map<int64_t, morphKeyData *> morphAnimData;

namespace Assimp {
namespace FBX {

class Document;
/** 
 *  Convert a FBX #Document to #aiScene
 *  @param out Empty scene to be populated
 *  @param doc Parsed FBX document
 *  @param removeEmptyBones Will remove bones, which do not have any references to vertices.
 */
void ConvertToAssimpScene(aiScene *out, const Document &doc, bool removeEmptyBones);

/** Dummy class to encapsulate the conversion process */
class FBXConverter {
public:
	/**
    *  The different parts that make up the final local transformation of a fbx-node
    */
	enum TransformationComp {
		TransformationComp_Translation,
		TransformationComp_Scaling,
		TransformationComp_Rotation,
		TransformationComp_RotationOffset,
		TransformationComp_RotationPivot,
		TransformationComp_PreRotation,
		TransformationComp_PostRotation,
		TransformationComp_ScalingOffset,
		TransformationComp_ScalingPivot,
		TransformationComp_GeometricTranslation,
		TransformationComp_GeometricRotation,
		TransformationComp_GeometricScaling,
		TransformationComp_MAXIMUM
	};

	enum PivotStatus {
		PivotStatus_Active = 0, // this means that the pivot has to be used for animation sampling and basic transform data
		PivotStatus_Reference = 1, // For lamens this means that the value exists
		// but should not be used in computation for animations or nodes.
	};

public:
	FBXConverter(aiScene *out, const Document &doc, bool removeEmptyBones);
	~FBXConverter();

private:
    std::map<uint64_t, aiMatrix4x4> bind_matricies;
	//
	// Animation stack used for pivot calculations
	// very important for resampling the right node, if there are duplicates
	void GenerateAnimStack();
	void ResampleAnimationsWithPivots(int64_t targetId, aiMatrix4x4 transform);
	std::vector<aiNodeAnim *> GetNodeAnimsFromStack(const std::string &node_name);

	// pass into resample
	// input of ResampleFunction
	// list of the first node
	// from each animation

	std::map<aiAnimation *, std::vector<aiNodeAnim *> > animation_stack;

	// we must still overwrite their node counterparts though.

	bool IsBone(const int64_t& element_id) {
	    if(bone_id_map.empty())
        {
	        ASSIMP_LOG_WARN("IsBone, bone list is empty so can't check for bones!");
        }

        bool count = bone_id_map.count(element_id) > 0;
	    ASSIMP_LOG_INFO_F("element id: ", element_id, " is bone:" ,count);

	    return count;
	}



	// ------------------------------------------------------------------------------------------------
	// find scene root and trigger recursive scene conversion
	void ConvertRootNode();

	// ------------------------------------------------------------------------------------------------
	// collect and assign child nodes
    void ConvertNodes(uint64_t id, aiNode *parent, aiNode *root_node, aiMatrix4x4 inverse_geometric_xform,
                      aiMatrix4x4 world_transform);

	// ------------------------------------------------------------------------------------------------
	void ConvertLights(const Model &model, const std::string &orig_name);

	// ------------------------------------------------------------------------------------------------
	void ConvertCameras(const Model &model, const std::string &orig_name);

	// ------------------------------------------------------------------------------------------------
	void ConvertLight(const Light &light, const std::string &orig_name);

	// ------------------------------------------------------------------------------------------------
	void ConvertCamera(const Camera &cam, const std::string &orig_name);

	// ------------------------------------------------------------------------------------------------
	void GetUniqueName(const std::string &name, std::string &uniqueName);

	// ------------------------------------------------------------------------------------------------
	// this returns unified names usable within assimp identifiers (i.e. no space characters -
	// while these would be allowed, they are a potential trouble spot so better not use them).
	const char *NameTransformationComp(TransformationComp comp);

	// ------------------------------------------------------------------------------------------------
	// Returns an unique name for a node or traverses up a hierarchy until a non-empty name is found and
	// then makes this name unique
	std::string MakeUniqueNodeName(const Model *const model, const aiNode &parent);

	// ------------------------------------------------------------------------------------------------
	// note: this returns the REAL fbx property names
	const char *NameTransformationCompProperty(TransformationComp comp);

	// ------------------------------------------------------------------------------------------------
	aiVector3D TransformationCompDefaultValue(TransformationComp comp);

	// ------------------------------------------------------------------------------------------------
	void GetRotationMatrix(Model::RotOrder mode, const aiVector3D &rotation, aiMatrix4x4 &out);
	// ------------------------------------------------------------------------------------------------
	/**
    *  checks if a node has more than just scaling, rotation and translation components
    */
	bool NeedsComplexTransformationChain(const Model &model);

	aiMatrix4x4 GeneratePivotTransform( const Model& model, aiMatrix4x4 &geometric_transform );

	aiMatrix4x4 GeneratePivotTransform(const PropertyTable &props, const Model::RotOrder &rot,
                                       aiMatrix4x4 &geometric_transform);

	void MagicPivotAlgorithm(
			aiMatrix4x4 chain[TransformationComp_MAXIMUM],
			aiMatrix4x4 &result,
			aiMatrix4x4 &geometric_transform);

	// ------------------------------------------------------------------------------------------------
	void SetupNodeMetadata(const Model &model, aiNode *nd);

	// ------------------------------------------------------------------------------------------------
	void ConvertModel(const Model &model, aiNode *parent, aiNode *root_node,
			const aiMatrix4x4 &absolute_transform);

	// ------------------------------------------------------------------------------------------------
	// MeshGeometry -> aiMesh, return mesh index + 1 or 0 if the conversion failed
	std::vector<unsigned int>
	ConvertMesh(const MeshGeometry &mesh, const Model &model, aiNode *parent, aiNode *root_node,
			const aiMatrix4x4 &absolute_transform);

	// ------------------------------------------------------------------------------------------------
	std::vector<unsigned int> ConvertLine(const LineGeometry &line, const Model &model,
			aiNode *parent, aiNode *root_node);

	// ------------------------------------------------------------------------------------------------
	aiMesh *SetupEmptyMesh(const Geometry &mesh, aiNode *parent);

	// ------------------------------------------------------------------------------------------------
	unsigned int ConvertMeshSingleMaterial(const MeshGeometry &mesh, const Model &model,
			const aiMatrix4x4 &absolute_transform, aiNode *parent,
			aiNode *root_node);

	// ------------------------------------------------------------------------------------------------
	std::vector<unsigned int>
	ConvertMeshMultiMaterial(const MeshGeometry &mesh, const Model &model, aiNode *parent, aiNode *root_node,
			const aiMatrix4x4 &absolute_transform);

	// ------------------------------------------------------------------------------------------------
	unsigned int ConvertMeshMultiMaterial(const MeshGeometry &mesh, const Model &model, MatIndexArray::value_type index,
			aiNode *parent, aiNode *root_node, const aiMatrix4x4 &absolute_transform);

	// ------------------------------------------------------------------------------------------------
	static const unsigned int NO_MATERIAL_SEPARATION = /* std::numeric_limits<unsigned int>::max() */
			static_cast<unsigned int>(-1);

	// ------------------------------------------------------------------------------------------------
	/**
    *  - if materialIndex == NO_MATERIAL_SEPARATION, materials are not taken into
    *    account when determining which weights to include.
    *  - outputVertStartIndices is only used when a material index is specified, it gives for
    *    each output vertex the DOM index it maps to.
    */
	void ConvertWeights(aiMesh *out, const Model &model, const MeshGeometry &geo, const aiMatrix4x4 &absolute_transform,
			aiNode *parent = NULL, aiNode *root_node = NULL,
			unsigned int materialIndex = NO_MATERIAL_SEPARATION,
			std::vector<unsigned int> *outputVertStartIndices = NULL);
	// lookup
	static const aiNode *GetNodeByName(const aiString &name, aiNode *current_node);
	// ------------------------------------------------------------------------------------------------
	void ConvertCluster(const Model &model, std::vector<aiBone *> &local_mesh_bones, const Cluster *cl,
			std::vector<size_t> &out_indices, std::vector<size_t> &index_out_indices,
			std::vector<size_t> &count_out_indices, const aiMatrix4x4 &absolute_transform,
			aiNode *parent, aiNode *root_node);

	// ------------------------------------------------------------------------------------------------
	void ConvertMaterialForMesh(aiMesh *out, const Model &model, const MeshGeometry &geo,
			MatIndexArray::value_type materialIndex);

	// ------------------------------------------------------------------------------------------------
	unsigned int GetDefaultMaterial();

	// ------------------------------------------------------------------------------------------------
	// Material -> aiMaterial
	unsigned int ConvertMaterial(const Material &material, const MeshGeometry *const mesh);

	// ------------------------------------------------------------------------------------------------
	// Video -> aiTexture
	unsigned int ConvertVideo(const Video &video);

	// ------------------------------------------------------------------------------------------------
	// convert embedded texture if necessary and return actual texture path
	aiString GetTexturePath(const Texture *tex);

	// ------------------------------------------------------------------------------------------------
	void TrySetTextureProperties(aiMaterial *out_mat, const TextureMap &textures,
			const std::string &propName,
			aiTextureType target, const MeshGeometry *const mesh);

	// ------------------------------------------------------------------------------------------------
	void TrySetTextureProperties(aiMaterial *out_mat, const LayeredTextureMap &layeredTextures,
			const std::string &propName,
			aiTextureType target, const MeshGeometry *const mesh);

	// ------------------------------------------------------------------------------------------------
	void SetTextureProperties(aiMaterial *out_mat, const TextureMap &textures, const MeshGeometry *const mesh);

	// ------------------------------------------------------------------------------------------------
	void SetTextureProperties(aiMaterial *out_mat, const LayeredTextureMap &layeredTextures, const MeshGeometry *const mesh);

	// ------------------------------------------------------------------------------------------------
	aiColor3D GetColorPropertyFromMaterial(const PropertyTable &props, const std::string &baseName,
			bool &result);
	aiColor3D GetColorPropertyFactored(const PropertyTable &props, const std::string &colorName,
			const std::string &factorName, bool &result, bool useTemplate = true);
	aiColor3D GetColorProperty(const PropertyTable &props, const std::string &colorName,
			bool &result, bool useTemplate = true);

	// ------------------------------------------------------------------------------------------------
	void SetShadingPropertiesCommon(aiMaterial *out_mat, const PropertyTable &props);
	void SetShadingPropertiesRaw(aiMaterial *out_mat, const PropertyTable &props, const TextureMap &textures, const MeshGeometry *const mesh);

	// ------------------------------------------------------------------------------------------------
	// get the number of fps for a FrameRate enumerated value
	static double FrameRateToDouble(FileGlobalSettings::FrameRate fp, double customFPSVal = -1.0);

	// ------------------------------------------------------------------------------------------------
	// convert animation data to aiAnimation et al
	void ConvertAnimations();

	// ------------------------------------------------------------------------------------------------
	// takes a fbx node name and returns the identifier to be used in the assimp output scene.
	// the function is guaranteed to provide consistent results over multiple invocations
	// UNLESS RenameNode() is called for a particular node name.
	std::string FixNodeName(const std::string &name);
	std::string FixAnimMeshName(const std::string &name);

	typedef std::map<const AnimationCurveNode *, const AnimationLayer *> LayerMap;

	// anim node item for list of anim curves
	struct AnimNodeItem {
		AnimNodeItem(const std::string &_name, std::vector<const AnimationCurveNode *> &_curves) :
				name(_name),
				curves(_curves) {}
		std::string name;
		std::vector<const AnimationCurveNode *> &curves;
	};

	// ------------------------------------------------------------------------------------------------
	void ConvertAnimationStack(const AnimationStack &st);

	// ------------------------------------------------------------------------------------------------
	void ProcessMorphAnimDatas(std::map<std::string, morphAnimData *> *morphAnimDatas, const BlendShapeChannel *bsc, const AnimationCurveNode *node);

	// ------------------------------------------------------------------------------------------------
	void GenerateNodeAnimations(
			std::vector<aiNodeAnim *> &node_anims,
			const std::string &fixed_name,
			const std::vector<const AnimationCurveNode *> &curves,
			const LayerMap &layer_map,
			int64_t start, int64_t stop,
			double &max_time,
			double &min_time,
			aiMatrix4x4 geometric_pivot_data);

	// ------------------------------------------------------------------------------------------------
	bool IsRedundantAnimationData(const Model &target,
			TransformationComp comp,
			const std::vector<const AnimationCurveNode *> &curves);

	// key (time), value, mapto (component index)
	typedef std::tuple<std::shared_ptr<KeyTimeList>, std::shared_ptr<KeyValueList>, unsigned int> KeyFrameList;
	typedef std::vector<KeyFrameList> KeyFrameListList;

	// ------------------------------------------------------------------------------------------------
	KeyFrameListList GetKeyframeList(const std::vector<const AnimationCurveNode *> &nodes, int64_t start, int64_t stop);

	// ------------------------------------------------------------------------------------------------
	KeyTimeList GetKeyTimeList(const KeyFrameListList &inputs);

	// ------------------------------------------------------------------------------------------------
	void InterpolateKeys(aiVectorKey *valOut, const KeyTimeList &keys, const KeyFrameListList &inputs,
			const aiVector3D &def_value,
			double &max_time,
			double &min_time);

	// ------------------------------------------------------------------------------------------------
	void InterpolateKeys(aiQuatKey *valOut, const KeyTimeList &keys, const KeyFrameListList &inputs,
			const aiVector3D &def_value,
			double &maxTime,
			double &minTime,
			Model::RotOrder order);

	// ------------------------------------------------------------------------------------------------
	void ConvertTransformOrder_TRStoSRT(aiQuatKey *out_quat, aiVectorKey *out_scale,
			aiVectorKey *out_translation,
			const KeyFrameListList &scaling,
			const KeyFrameListList &translation,
			const KeyFrameListList &rotation,
			const KeyTimeList &times,
			double &maxTime,
			double &minTime,
			Model::RotOrder order,
			const aiVector3D &def_scale,
			const aiVector3D &def_translate,
			const aiVector3D &def_rotation);

	// ------------------------------------------------------------------------------------------------
	// euler xyz -> quat
	aiQuaternion EulerToQuaternion(const aiVector3D &rot, Model::RotOrder order);

	// ------------------------------------------------------------------------------------------------
	void ConvertScaleKeys(aiNodeAnim *na, const std::vector<const AnimationCurveNode *> &nodes, const LayerMap & /*layers*/,
			int64_t start, int64_t stop,
			double &maxTime,
			double &minTime);

	// ------------------------------------------------------------------------------------------------
	void ConvertTranslationKeys(aiNodeAnim *na, const std::vector<const AnimationCurveNode *> &nodes,
			const LayerMap & /*layers*/,
			int64_t start, int64_t stop,
			double &maxTime,
			double &minTime);

	// ------------------------------------------------------------------------------------------------
	void ConvertRotationKeys(aiNodeAnim *na, const std::vector<const AnimationCurveNode *> &nodes,
			const LayerMap & /*layers*/,
			int64_t start, int64_t stop,
			double &maxTime,
			double &minTime,
			Model::RotOrder order);

	void ConvertGlobalSettings();

	// ------------------------------------------------------------------------------------------------
	// copy generated meshes, animations, lights, cameras and textures to the output scene
	void TransferDataToScene();

	// ------------------------------------------------------------------------------------------------
	// FBX file could have embedded textures not connected to anything
	void ConvertOrphantEmbeddedTextures();

private:
	// 0: not assigned yet, others: index is value - 1
	unsigned int defaultMaterialIndex;

	std::vector<aiMesh *> meshes;
	std::vector<aiMaterial *> materials;
	std::vector<aiAnimation *> animations;
	std::map<int64_t, const LimbNode*> bone_id_map;
	// anim target mapping to allow us to lookup direct node anims.
	std::map<aiNodeAnim*, int64_t> anim_target_map;

	std::vector<int64_t> resampled_anim;

	//std::map<int64_t, aiSkin *> skin_id_map;
	std::vector<aiLight *> lights;
	std::vector<aiCamera *> cameras;
	std::vector<aiTexture *> textures;

	using MaterialMap = std::fbx_unordered_map<const Material *, unsigned int>;
	MaterialMap materials_converted;

	using VideoMap = std::fbx_unordered_map<const Video, unsigned int>;
	VideoMap textures_converted;

	using MeshMap = std::fbx_unordered_map<const Geometry *, std::vector<unsigned int> >;
	MeshMap meshes_converted;

	// fixed node name -> which trafo chain components have animations?
	using NodeAnimBitMap = std::fbx_unordered_map<std::string, unsigned int>;
	NodeAnimBitMap node_anim_chain_bits;

	// number of nodes with the same name
	using NodeNameCache = std::fbx_unordered_map<std::string, unsigned int>;
	NodeNameCache mNodeNames;

	// Deformer name is not the same as a bone name - it does contain the bone name though :)
	// Deformer names in FBX are always unique in an FBX file.
	std::map<const std::string, aiBone *> bone_map;


	double anim_fps;

	aiScene *const out;
	const FBX::Document &doc;

	static void BuildBoneList(aiNode *current_node, const aiNode *root_node, const aiScene *scene,
			std::vector<aiBone *> &bones);

	void BuildBoneStack(aiNode *current_node, const aiNode *root_node, const aiScene *scene,
			const std::vector<aiBone *> &bones,
			std::map<aiBone *, aiNode *> &bone_stack,
			std::vector<aiNode *> &node_stack);

	static void BuildNodeList(aiNode *current_node, std::vector<aiNode *> &nodes);

	static aiNode *GetNodeFromStack(const aiString &node_name, std::vector<aiNode *> &nodes);

	static aiNode *GetArmatureRoot(aiNode *bone_node, std::vector<aiBone *> &bone_list);

	static bool IsBoneNode(const aiString &bone_name, std::vector<aiBone *> &bones);

    void FindAllBones(const Model &model, int64_t parent_id);

    void CacheNodeInformation(uint64_t id);
};

} // namespace FBX
} // namespace Assimp

#endif // INCLUDED_AI_FBX_CONVERTER_H
