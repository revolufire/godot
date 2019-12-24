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
#include "NormalizeWeightsProcess.h"

#include <assimp/BaseImporter.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Assimp {

NormalizeWeightsProcess::NormalizeWeightsProcess() :
		BaseProcess() {
}

NormalizeWeightsProcess::~NormalizeWeightsProcess() {
	// empty
}

bool NormalizeWeightsProcess::IsActive(unsigned int pFlags) const {
	return true;
}

void NormalizeWeightsProcess::SetupProperties(const Importer *pImp) {
}

void NormalizeWeightsProcess::Execute(aiScene *pScene) {
	ai_assert(nullptr != pScene);
	ai_assert(nullptr != pScene->mRootNode);

	if (nullptr == pScene) {
		return;
	}

	if (nullptr == pScene->mRootNode) {
		return;
	}

	for (unsigned int meshID = 0; meshID < pScene->mNumMeshes; meshID++) {
		aiMesh *mesh = pScene->mMeshes[meshID];
		NormalizeWeights(mesh);
	}
}

unsigned int NormalizeWeightsProcess::NormalizeWeights(const aiMesh *mesh) {
	if (mesh->mNumBones == 0) return 0;
    ASSIMP_LOG_INFO("Normalizing weights of mesh");

	struct BoneWeight {
		unsigned int mBoneIndex; // index of a bone in current mesh
		aiVertexWeight *mVertexWeight; // a pointer to mVertexWeight in meshs[x]->mBones[x]->mWeight for quick visit
	};

	struct Vertex_BoneWeights {
		float mTotalWeight;
		std::vector<BoneWeight> mBoneWeights;
	};

	std::map<unsigned int, Vertex_BoneWeights> map;

	for (unsigned int b = 0; b < mesh->mNumBones; b++) {
		auto bone = mesh->mBones[b];
        //std::cout << "bone weight count: " << bone->mNumWeights << std::endl;
		for (unsigned int w = 0; w < bone->mNumWeights; w++) {

			auto vertexWeight = &bone->mWeights[w];
			auto key = vertexWeight->mVertexId;

			if (map.find(key) == map.end()) {
				map.insert(std::map<unsigned int, Vertex_BoneWeights>::value_type(key, Vertex_BoneWeights()));
			}

			auto &vertex_BoneWeights = map[key];

			BoneWeight boneWeights;
			boneWeights.mBoneIndex = b;
			boneWeights.mVertexWeight = vertexWeight;

			vertex_BoneWeights.mTotalWeight += vertexWeight->mWeight;
			vertex_BoneWeights.mBoneWeights.push_back(boneWeights);
		}
	}

	unsigned int count = 0;
	// normalize all weights:
	// every weight for a same vertex divided by totalWeight of this vertex
	for (auto &item : map) {
		auto &vertex_BoneWeights = item.second;
		auto f = 1.0 / vertex_BoneWeights.mTotalWeight;
		for (unsigned int i = 0; i < vertex_BoneWeights.mBoneWeights.size(); i++) {
			vertex_BoneWeights.mBoneWeights[i].mVertexWeight->mWeight *= f;
			count++;
		}
	}

	return count;
}

} // Namespace Assimp
