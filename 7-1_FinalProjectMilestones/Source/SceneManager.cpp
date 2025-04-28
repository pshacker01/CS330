///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}
/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/


/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL brickMaterial;
	brickMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	brickMaterial.ambientStrength = 0.2f;
	brickMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	brickMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	brickMaterial.shininess = 0.3f;
	brickMaterial.tag = "brick2";

	m_objectMaterials.push_back(brickMaterial);

	OBJECT_MATERIAL pyramidMaterial;
	pyramidMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	pyramidMaterial.ambientStrength = 0.3f;
	pyramidMaterial.diffuseColor = glm::vec3(0.7f, 0.6f, 0.5f);
	pyramidMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	pyramidMaterial.shininess = 8.0f;
	pyramidMaterial.tag = "pyramid";

	m_objectMaterials.push_back(pyramidMaterial);

}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for loading textures from image files
 *  and assigning them to identifiers (tags) used in the scene.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// Load textures from correct relative directory and assign tags for reference in RenderScene
	CreateGLTexture("../../Projects/7-1_FinalProjectMilestones/Textures/sand.jpg", "ground");        // For desert ground
	CreateGLTexture("../../Projects/7-1_FinalProjectMilestones/Textures/brick.jpg", "wall");         // For temple walls
	CreateGLTexture("../../Projects/7-1_FinalProjectMilestones/Textures/brick.jpg", "cap");          // For tower caps
	CreateGLTexture("../../Projects/7-1_FinalProjectMilestones/Textures/pyramid.jpg", "pillar");     // For the cylinder pillar

	// Bind loaded textures to available texture slots in OpenGL
	BindGLTextures();
}
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Light 0: Dusk sun (bright orange tint from upper right)
	m_pShaderManager->setVec3Value("lightSources[0].position", 10.0f, 10.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.15f, 0.1f); // dusk glow
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.9f, 0.6f, 0.4f);  // warm sunset
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.6f, 0.4f, 0.3f); // matching tint
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.4f);

	// Light 1: Sky fill (cooler ambient from behind)
	m_pShaderManager->setVec3Value("lightSources[1].position", -10.0f, 10.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.15f, 0.18f, 0.25f); // bluish ambient
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.3f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.1f, 0.1f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	// Light 2: Ground bounce (very soft upward fill)
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, -5.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.1f, 0.08f, 0.06f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.15f, 0.12f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.0f);

	// Light 3: Shadow relief light (back left)
	m_pShaderManager->setVec3Value("lightSources[3].position", -20.0f, 5.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load the textures for the scene
	LoadSceneTextures();

	// Load necessary shape meshes for rendering
	m_basicMeshes->LoadPlaneMesh();         // Ground
	m_basicMeshes->LoadBoxMesh();           // Temple walls, towers, caps, and lintel
	m_basicMeshes->LoadCylinderMesh();      // Pillar
	m_basicMeshes->LoadPyramid4Mesh();      // Pyramids
}



/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Variables for transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Ground plane
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("ground");  // Sand texture
	m_basicMeshes->DrawPlaneMesh();

	// Optional rotation if I want the structure canted
	float templeRotation = 0.0f; // Increase to, e.g., 15.0f for a slight angle



	//float templeRotation = 0.0f;

	/********************************************************
	 *  CENTRAL TEMPLE ENTRY
	 ********************************************************/
	scaleXYZ = glm::vec3(1.0f, 3.0f, 1.5f);
	positionXYZ = glm::vec3(0.0f, 1.5f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, templeRotation, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawBoxMesh();

	// Central temple cap
	scaleXYZ = glm::vec3(1.2f, 0.3f, 1.7f);
	positionXYZ = glm::vec3(0.0f, 3.2f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, templeRotation, 0.0f, positionXYZ);
	SetShaderTexture("cap");
	m_basicMeshes->DrawBoxMesh();

	/********************************************************
	 *  LEFT TEMPLE FACADE
	 ********************************************************/
	scaleXYZ = glm::vec3(0.6f, 2.5f, 1.2f);
	positionXYZ = glm::vec3(-2.0f, 1.25f, 0.5f);
	SetTransformations(scaleXYZ, 0.0f, templeRotation, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawBoxMesh();

	/********************************************************
	 *  RIGHT TEMPLE FACADE
	 ********************************************************/
	scaleXYZ = glm::vec3(0.6f, 2.5f, 1.2f);
	positionXYZ = glm::vec3(2.0f, 1.25f, 0.5f);
	SetTransformations(scaleXYZ, 0.0f, templeRotation, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawBoxMesh();

	/********************************************************
	 *  SCATTERED PILLARS
	 ********************************************************/
	 // Front left pillar
	scaleXYZ = glm::vec3(0.3f, 1.5f, 0.3f);
	positionXYZ = glm::vec3(-3.5f, 0.0f, 1.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("pillar");
	m_basicMeshes->DrawCylinderMesh();

	// Front center pillar
	positionXYZ = glm::vec3(0.0f, 0.0f, 1.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	/********************************************************
	 *  PYRAMIDS
	 ********************************************************/
	 // Left pyramid (near)
	scaleXYZ = glm::vec3(6.0f, 10.0f, 6.0f);
	positionXYZ = glm::vec3(-10.0f, 5.0f, -3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("pyramid");
	SetShaderMaterial("pyramid");
	m_basicMeshes->DrawPyramid4Mesh();

	// Center back pyramid (tall)
	scaleXYZ = glm::vec3(6.0f, 12.0f, 6.0f);
	positionXYZ = glm::vec3(0.0f, 6.0f, -8.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawPyramid4Mesh();

	// Right pyramid
	scaleXYZ = glm::vec3(6.0f, 10.0f, 6.0f);
	positionXYZ = glm::vec3(10.0f, 5.0f, -3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawPyramid4Mesh();

	/********************************************************
 *  LEFT HALLWAY WALL (formerly left tower)
 ********************************************************/
	scaleXYZ = glm::vec3(1.0f, 3.0f, 6.0f);  // Increased depth
	positionXYZ = glm::vec3(-1.0f, 1.5f, -3.0f);  // Centered in Z
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawBoxMesh();

	/********************************************************
	 *  RIGHT HALLWAY WALL (formerly right tower)
	 ********************************************************/
	scaleXYZ = glm::vec3(1.0f, 3.0f, 6.0f);  // Same depth as left wall
	positionXYZ = glm::vec3(1.0f, 1.5f, -3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawBoxMesh();

	/********************************************************
 *  DOOR FRAME - Vertical Left Side
 ********************************************************/
	scaleXYZ = glm::vec3(0.2f, 2.5f, 0.2f);  // Thin vertical post
	positionXYZ = glm::vec3(-0.6f, 1.25f, 0.9f);  // Slightly in front
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawBoxMesh();

	/********************************************************
	 *  DOOR FRAME - Vertical Right Side
	 ********************************************************/
	scaleXYZ = glm::vec3(0.2f, 2.5f, 0.2f);
	positionXYZ = glm::vec3(0.6f, 1.25f, 0.9f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawBoxMesh();

	/********************************************************
	 *  DOOR FRAME - Top Lintel
	 ********************************************************/
	scaleXYZ = glm::vec3(1.2f, 0.3f, 0.2f);
	positionXYZ = glm::vec3(0.0f, 2.7f, 0.9f);  // Above the doorway
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("cap");
	m_basicMeshes->DrawBoxMesh();


	/********************************************************
	 *  HALLWAY TOP / LINTEL (roof of hallway)
	 ********************************************************/
	scaleXYZ = glm::vec3(3.0f, 0.3f, 6.0f);  // Covers top across hallway
	positionXYZ = glm::vec3(0.0f, 3.2f, -3.0f);  // Elevated and centered
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("cap");
	m_basicMeshes->DrawBoxMesh();





}