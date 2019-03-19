#include "LoadMesh.h"
#include <fstream>
#include <algorithm>

#include <GL/glew.h>
#include "assimp/Importer.hpp"
#include "assimp/PostProcess.h"

// Create an instance of the Importer class
Assimp::Importer importer;
// scale factor for the model to fit in the window

void GetBoundingBox(const aiMesh* mesh, aiVector3D* min, aiVector3D* max)
{
   min->x = min->y = min->z =  1e10f;
	max->x = max->y = max->z = -1e10f;

   for (unsigned int t = 0; t < mesh->mNumVertices; ++t) 
   {
      aiVector3D tmp = mesh->mVertices[t];

      min->x = std::min(min->x,tmp.x);
      min->y = std::min(min->y,tmp.y);
      min->z = std::min(min->z,tmp.z);

      max->x = std::max(max->x,tmp.x);
      max->y = std::max(max->y,tmp.y);
      max->z = std::max(max->z,tmp.z);
   }
	
}


void GetBoundingBoxForNode (const aiScene* scene, const aiNode* nd, aiVector3D* min, aiVector3D* max)
{
	unsigned int n = 0, t;

	for (; n < nd->mNumMeshes; ++n) 
   {
		const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) 
      {

			aiVector3D tmp = mesh->mVertices[t];

			min->x = std::min(min->x,tmp.x);
			min->y = std::min(min->y,tmp.y);
			min->z = std::min(min->z,tmp.z);

			max->x = std::max(max->x,tmp.x);
			max->y = std::max(max->y,tmp.y);
			max->z = std::max(max->z,tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) 
   {
		GetBoundingBoxForNode(scene, nd->mChildren[n],min,max);
	}
}


void GetBoundingBox (const aiScene* scene, aiVector3D* min, aiVector3D* max)
{

	min->x = min->y = min->z =  1e10f;
	max->x = max->y = max->z = -1e10f;
	GetBoundingBoxForNode(scene, scene->mRootNode, min, max);
}

MeshData LoadMesh( const std::string& pFile)
{

   MeshData mesh;

	//check if file exists
	std::ifstream fin(pFile.c_str());
	if(!fin.fail()) 
   {
		fin.close();
	}
	else
   {
		printf("Couldn't open file: %s\n", pFile.c_str());
		printf("%s\n", importer.GetErrorString());
		return mesh;
	}

	mesh.mScene = importer.ReadFile( pFile, aiProcessPreset_TargetRealtime_Quality);//|aiProcess_FlipWindingOrder);

	// If the import failed, report it
	if( !mesh.mScene)
	{
		printf("%s\n", importer.GetErrorString());
		return mesh;
	}

	// Now we can access the file's contents.
	printf("Import of scene %s succeeded.", pFile.c_str());

	GetBoundingBox(mesh.mScene, &mesh.mBbMin, &mesh.mBbMax);
   aiVector3D diff = mesh.mBbMax-mesh.mBbMin;
   float w = std::max(diff.x, std::max(diff.y, diff.z));

	mesh.mScaleFactor = 1.0f / w;

	
   BufferIndexedVerts(mesh);
   mesh.mNumIndices = mesh.mScene->mMeshes[0]->mNumFaces*3;
	return mesh;
}

void BufferIndexedVerts(MeshData& meshdata)
{

   if(meshdata.mVao != -1)
   {
      glDeleteVertexArrays(1, &meshdata.mVao);
   }

   if(meshdata.mIndexBuffer != -1)
   {
      glDeleteBuffers(1, &meshdata.mIndexBuffer);
   }

   if(meshdata.mVboVerts != -1)
   {
      glDeleteBuffers(1, &meshdata.mVboVerts);
   }

   if(meshdata.mVboTexCoords != -1)
   {
      glDeleteBuffers(1, &meshdata.mVboTexCoords);
   }

   if(meshdata.mVboNormals != -1)
   {
      glDeleteBuffers(1, &meshdata.mVboNormals);
   }

   //shader attrib locations
   int pos_loc = -1;
   int tex_coord_loc = -1;
   int normal_loc = -1;

   GLint program = -1;
   glGetIntegerv(GL_CURRENT_PROGRAM, &program);

   pos_loc = glGetAttribLocation(program, "pos_attrib");
   tex_coord_loc = glGetAttribLocation(program, "tex_coord_attrib");
   normal_loc = glGetAttribLocation(program, "normal_attrib");

   glGenVertexArrays(1, &meshdata.mVao);
	glBindVertexArray(meshdata.mVao);
   const aiMesh* mesh = meshdata.mScene->mMeshes[0];
   unsigned int numFaces = mesh->mNumFaces;

      unsigned int *faceArray;
		faceArray = (unsigned int *)malloc(sizeof(unsigned int) * numFaces * 3);
		unsigned int faceIndex = 0;

		for (unsigned int t = 0; t < numFaces; ++t) 
      {
			const aiFace* face = &mesh->mFaces[t];

			memcpy(&faceArray[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
			faceIndex += 3;
		}

   //Buffer indices
   glGenBuffers(1, &meshdata.mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshdata.mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * numFaces * 3, faceArray, GL_STATIC_DRAW);
   free(faceArray);

	//Buffer vertices
   if (mesh->HasPositions()) {
			glGenBuffers(1, &meshdata.mVboVerts);
			glBindBuffer(GL_ARRAY_BUFFER, meshdata.mVboVerts);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(pos_loc);
			glVertexAttribPointer(pos_loc, 3, GL_FLOAT, 0, 0, 0);
		}

   // buffer for vertex texture coordinates
		if (mesh->HasTextureCoords(0)) {
			float *texCoords = (float *)malloc(sizeof(float)*2*mesh->mNumVertices);
			for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
				texCoords[k*2]   = mesh->mTextureCoords[0][k].x;
				texCoords[k*2+1] = mesh->mTextureCoords[0][k].y; 
			}
			glGenBuffers(1, &meshdata.mVboTexCoords);
			glBindBuffer(GL_ARRAY_BUFFER, meshdata.mVboTexCoords);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
			glEnableVertexAttribArray(tex_coord_loc);
			glVertexAttribPointer(tex_coord_loc, 2, GL_FLOAT, 0, 0, 0);
         free(texCoords);
		}

      if (mesh->HasNormals()) {
			float *normals = (float *)malloc(sizeof(float)*3*mesh->mNumVertices);
			for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
				normals[k*3]   = mesh->mNormals[k].x;
				normals[k*3+1] = mesh->mNormals[k].y;
            normals[k*3+2] = mesh->mNormals[k].z; 
			}
			glGenBuffers(1, &meshdata.mVboNormals);
			glBindBuffer(GL_ARRAY_BUFFER, meshdata.mVboNormals);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->mNumVertices, normals, GL_STATIC_DRAW);
			glEnableVertexAttribArray(normal_loc);
			glVertexAttribPointer(normal_loc, 3, GL_FLOAT, 0, 0, 0);
         free(normals);
		}

	   glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}
