///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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
void SceneManager::LoadSceneTextures() {
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.
	***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"./textures/wood.jpg",
		"floor");
	bReturn = CreateGLTexture(
		"./textures/wall.jpg",
		"wall");
	bReturn = CreateGLTexture(
		"./textures/tan.jpg",
		"vase");
	bReturn = CreateGLTexture(
		"./textures/red.jpeg",
		"book");
	bReturn = CreateGLTexture(
		"./textures/brick.jpeg",
		"brick");
	bReturn = CreateGLTexture(
		"./textures/glass.jpg",
		"window");
	bReturn = CreateGLTexture(
		"./textures/green.jpg",
		"book1");
	bReturn = CreateGLTexture(
		"./textures/blue.jpg",
		"book2");
	bReturn = CreateGLTexture(
		"./textures/desk.jpg",
		"desk");
	bReturn = CreateGLTexture(
		"./textures/book3.jpg",
		"book3");
	bReturn = CreateGLTexture(
		"./textures/fur.jpeg",
		"cat");

	BindGLTextures();

}
/***********************************************************
   *  DefineObjectMaterials()
   *
   *  This method is used for configuring the various material
   *  settings for all of the objects within the 3D scene.
   ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5f;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);


	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clothMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(.2f, .2f, .4f);
	glassMaterial.shininess = .5;
	glassMaterial.tag = "cloth";
	m_objectMaterials.push_back(clothMaterial);

}
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
 // this line of code is NEEDED for telling the shaders to render 
 // the 3D scene with custom lighting, if no light sources have
 // been added then the display window will be black - to use the 
 // default OpenGL lighting then comment out the following line
 
//m_pShaderManager->setBoolValue(g_UseLightingName, true);

 /*** STUDENTS - add the code BELOW for setting up light sources ***/
 /*** Up to four light sources can be defined. Refer to the code ***/
 /*** in the OpenGL Sample for help                              ***/
void SceneManager::SetupSceneLights() {

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -1.0f, 1.0f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", .1f, .1f, .1f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 1.0f, 0.6f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);





	//used to make it look like rays of sun are coming through window
	m_pShaderManager->setVec3Value("spotLight.position", 3.0f, 8.0f, -10.0f);
	m_pShaderManager->setVec3Value("spotLight.direction", .5f, -.5f, 1.0f);
	m_pShaderManager->setVec3Value("spotLight.ambient", 1.f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("spotLight.specular", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.30f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.01f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(10.0f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(30.0f)));
	m_pShaderManager->setBoolValue("spotLight.bActive", true);

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
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	RenderFloor();
	RenderWall();
	RenderBookcase();
	RenderWindow();
	RenderVase();
	RenderBook();
	RenderDesk();
	RenderCat();
}

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

		//Render Floor
void SceneManager::RenderFloor()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.5f, 0.0f, 4.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, .0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	SetShaderTexture("wall");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}
	/*///////////////////////////////////////////////////////////////////////////////////////*/
	
	//Renderwall()
void SceneManager::RenderWall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.5f, 0.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 6.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("brick");
	SetTextureUVScale(2.0, 1.0);

	m_basicMeshes->DrawPlaneMesh();


	//wall


		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.8f, 0.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.51f, 6.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0);


	m_basicMeshes->DrawPlaneMesh();
	//wall


		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.8f, 0.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 6.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0);


	m_basicMeshes->DrawPlaneMesh();
}
/*////////////////////////////////////////////////////////////*/

	//RenderiWndow()
	// frame 
	void SceneManager::RenderWindow()
	{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	

	//window

		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 6.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("window");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	}
	/////RenderVase()
	void SceneManager::RenderVase()
	{
		// declare the variables for the transformations
		glm::vec3 scaleXYZ;
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ;

		// set the XYZ scale for the sphere mesh
		scaleXYZ = glm::vec3(.80f, 0.50f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 4.85f, 3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("vase");

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		// set the XYZ scale for the tapered cylinder mesh
		scaleXYZ = glm::vec3(.90f, .70f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 180.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 5.50f, 3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("vase");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("clay");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		// set the XYZ scale for the torus mesh
		scaleXYZ = glm::vec3(1.0f, 0.05f, .8f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 180.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 4.6f, 3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		
		SetShaderTexture("tan");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("clay");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();
	}
	/////////RenderBookCase()
	void SceneManager::RenderBookcase()
	{
		// declare the variables for the transformations
		glm::vec3 scaleXYZ;
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ;

		// set the XYZ scale for the box mesh
		scaleXYZ = glm::vec3(3.00f, .20f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 4.40f, 2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		// set the XYZ scale for the box mesh
		scaleXYZ = glm::vec3(3.00f,.2f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, .10f, 2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();


		// set the XYZ scale for the box mesh
		scaleXYZ = glm::vec3(3.00f, .20f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 2.20f, 2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// set the XYZ scale for the box mesh
		scaleXYZ = glm::vec3(3.00f, 4.70f, .50f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 2.20f, -.70f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");


		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		// set the XYZ scale for the box mesh
		scaleXYZ = glm::vec3(3.00f, 4.70f, .50f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 2.20f, 4.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	/////RenderBook()
	void SceneManager::RenderBook()
	{
		// declare the variables for the transformations
		glm::vec3 scaleXYZ;
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ;
		// set the XYZ scale for the box mesh(book)
		scaleXYZ = glm::vec3(1.50f, 2.0f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 5.40f, 1.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book");



		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		// set the XYZ scale for the box mesh(book)
		scaleXYZ = glm::vec3(1.50f, 1.8f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(9.0f, 3.1f, 3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book");
		m_basicMeshes->DrawBoxMesh();

		// set the XYZ scale for the box mesh(book)
		scaleXYZ = glm::vec3(1.50f, 1.4f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.8f, 3.0f, 2.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book1");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		// set the XYZ scale for the box mesh(book)
		scaleXYZ = glm::vec3(1.50f, 1.4f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.8f, 3.0f, 2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book2");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		scaleXYZ = glm::vec3(1.50f, 1.0f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.8f, 2.8f, 1.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		scaleXYZ = glm::vec3(1.50f, 1.4f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.8f, 5.0f, 2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book2");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		scaleXYZ = glm::vec3(1.50f, 2.f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.8f, 5.3f, 1.2f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book1");

		scaleXYZ = glm::vec3(1.5f, 2.0f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(1.6f, 5.2f, -1.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		scaleXYZ = glm::vec3(1.0f, 1.5f, .20f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(1.6f, 5.0f, -1.2f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("book3");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
		/////RenderDesk()
	void SceneManager::RenderDesk()
	{
		// declare the variables for the transformations
		glm::vec3 scaleXYZ;
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ;
		scaleXYZ = glm::vec3(9.00f, .50f, 2.00f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-2.0f, 4.0f, -2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();


		scaleXYZ = glm::vec3(.50f, 4.0f, 2.00f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-6.2f, 2.0f, -2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		scaleXYZ = glm::vec3(.5f, 4.0f, 2.00f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(2.2f, 2.0f, -2.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
		////////////////////////////////////////////////////////////////////////////
			//RenderCat()
			void SceneManager::RenderCat()
			{
				// declare the variables for the transformations
				glm::vec3 scaleXYZ;
				float XrotationDegrees = 0.0f;
				float YrotationDegrees = 0.0f;
				float ZrotationDegrees = 0.0f;
				glm::vec3 positionXYZ;

				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.6f, 1.9f, .5f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 90.0f;
				YrotationDegrees = 90.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(2.0f, 1.0f, 2.0f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				
				SetShaderColor(0.5f, 0.5f, .5f, 1.0);



				m_basicMeshes->DrawCylinderMesh();

				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.3f, .7f, .5f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 0.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.5f, 1.0f, 2.0f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				m_basicMeshes->DrawCylinderMesh();
				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.6f, .7f, .5f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 0.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.5f, 1.7f, 2.0f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				m_basicMeshes->DrawSphereMesh();
				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.5f, .6f, .5f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 90.0f;
				YrotationDegrees = 90.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.6f, 1.7f, 2.0f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawTaperedCylinderMesh();

				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.2f, .2f, .1f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = -30.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.4f, 2.4f, 2.0f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawPrismMesh();
				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.4f, 1.7f, 2.0f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawTaperedCylinderMesh();

				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.2f, .2f, .1f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = -30.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.4f, 2.4f, 2.2f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawPrismMesh();

				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.2f, .7f, .1f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 0.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.7f, 0.0f, 2.2f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawCylinderMesh();
				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.2f, .7f, .1f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 0.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(2.3f, 0.0f, 2.2f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawCylinderMesh();
				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.2f, .7f, .1f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 0.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(3.7f, 0.0f, 1.7f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawCylinderMesh();

				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.2f, .7f, .1f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = 0.0f;
				YrotationDegrees = 0.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(2.3f, 0.0f, 1.7f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawCylinderMesh();
				// set the XYZ scale for the mesh
				scaleXYZ = glm::vec3(.2f, 1.5f, .1f);

				// set the XYZ rotation for the mesh
				XrotationDegrees = -45.0f;
				YrotationDegrees = 90.0f;
				ZrotationDegrees = 0.0f;

				// set the XYZ position for the mesh
				positionXYZ = glm::vec3(2.3f, 1.0f, 2.0f);

				// set the transformations into memory to be used on the drawn meshes
				SetTransformations(
					scaleXYZ,
					XrotationDegrees,
					YrotationDegrees,
					ZrotationDegrees,
					positionXYZ);

				SetShaderColor(.5f, .5f, .5f, 1.0);
				SetTextureUVScale(2.0, 1.0);
				m_basicMeshes->DrawCylinderMesh();
		}
	

	
	/****************************************************************/

