/*==============================================================================
            Copyright (c) 2012 QUALCOMM Austria Research Center GmbH.
            All Rights Reserved.
            Qualcomm Confidential and Proprietary

@file 
    ImageTargets.cpp

@brief
    Sample for ImageTargets

==============================================================================*/


#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef USE_OPENGL_ES_1_1
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#include <QCAR/QCAR.h>
#include <QCAR/CameraDevice.h>
#include <QCAR/Renderer.h>
#include <QCAR/VideoBackgroundConfig.h>
#include <QCAR/Trackable.h>
#include <QCAR/TrackableResult.h>
#include <QCAR/Tool.h>
#include <QCAR/Tracker.h>
#include <QCAR/TrackerManager.h>
#include <QCAR/ImageTracker.h>
#include <QCAR/ImageTargetResult.h>
#include <QCAR/VirtualButtonResult.h>
#include <QCAR/ImageTarget.h>
#include <QCAR/VirtualButton.h>
#include <QCAR/Rectangle.h>
#include <QCAR/Area.h>
#include <QCAR/CameraCalibration.h>
#include <QCAR/UpdateCallback.h>
#include <QCAR/DataSet.h>

#include "SampleUtils.h"
#include "Texture.h"
#include "CubeShaders.h"
#include "LineShaders.h"
#include "models/models.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Textures:
int textureCount                = 0;
Texture** textures              = 0;
//
//// OpenGL ES 2.0 specific:
#ifndef USE_OPENGL_ES_1_1
unsigned int shaderProgramID    = 0;
GLint vertexHandle              = 0;
GLint normalHandle              = 0;
GLint textureCoordHandle        = 0;
GLint mvpMatrixHandle           = 0;
#endif

//OpenGL ES 2.0 specific (Virtual Buttons):
unsigned int vbShaderProgramID 	= 0;
GLint vbVertexHandle			= 0;

// Screen dimensions:
unsigned int screenWidth        = 0;
unsigned int screenHeight       = 0;

// Indicates whether screen is in portrait (true) or landscape (false) mode
bool isActivityInPortraitMode   = false;

// The projection matrix used for rendering virtual objects:
QCAR::Matrix44F projectionMatrix;

// Constants:
//static const float kObjectScale = 3.f;
static const float kObjectScale = 60.f;

static const int SOUND_INDEX_HORSIE = 0;
static const int SOUND_INDEX_DOGGIE = 1;

QCAR::DataSet* dataSetAnimal 			= 0;

bool updateBtns                   = false;

bool soundIsPlayed = false;
JNIEnv* javaEnv;
jobject javaObj;
jclass javaClass;

JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargetsRenderer_initNativeCallback(JNIEnv* env, jobject obj)
{
	LOG("Init Native Callback");
	javaEnv = env;

	javaObj = env->NewGlobalRef(obj);

	jclass objClass = env->GetObjectClass(obj);
	javaClass = (jclass) env->NewGlobalRef(objClass);
}

void playSoundEffect(int soundIndex, float volume = 1.0f)
{
	// Sound playback is always handled by the Android SDK
	// Use the environment and class stored in initNativeCallback
	// to call a Java method that plays a sound
	jmethodID method = javaEnv->GetMethodID(javaClass, "playSound", "(IF)V");
	javaEnv->CallVoidMethod(javaObj, method, soundIndex, volume);
}


// Object to receive update callbacks from QCAR SDK
class ImageTargets_UpdateCallback : public QCAR::UpdateCallback
{   
	virtual void QCAR_onUpdate(QCAR::State& /*state*/)
	{


	}
};



ImageTargets_UpdateCallback updateCallback;

JNIEXPORT int JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_getOpenGlEsVersionNative(JNIEnv *, jobject)
{
#ifdef USE_OPENGL_ES_1_1        
	return 1;
#else
	return 2;
#endif
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_setActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
	isActivityInPortraitMode = isPortrait;
}


JNIEXPORT int JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_initTracker(JNIEnv *, jobject)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_initTracker");

	// Initialize the image tracker:
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
	QCAR::Tracker* tracker = trackerManager.initTracker(QCAR::Tracker::IMAGE_TRACKER);
	if (tracker == NULL)
	{
		LOG("Failed to initialize ImageTracker.");
		return 0;
	}

	LOG("Successfully initialized ImageTracker.");
	return 1;
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_deinitTracker(JNIEnv *, jobject)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_deinitTracker");

	// Deinit the image tracker:
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
	trackerManager.deinitTracker(QCAR::Tracker::IMAGE_TRACKER);
}


JNIEXPORT int JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_loadTrackerData(JNIEnv *, jobject)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_loadTrackerData");

	// Get the image tracker:
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
	QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
			trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
	if (imageTracker == NULL)
	{
		LOG("Failed to load tracking data set because the ImageTracker has not"
				" been initialized.");
		return 0;
	}

	dataSetAnimal = imageTracker->createDataSet();
	if(dataSetAnimal == 0)
	{
		LOG("Failed to create new tracking data");
		return 0;
	}

	if(!dataSetAnimal->load("datasets/Animal_card.xml",QCAR::DataSet::STORAGE_APPRESOURCE))
		//	if(!dataSetAnimal->load("AnimalwithButton.xml",QCAR::DataSet::STORAGE_APPRESOURCE))


	{
		LOG("Failed to load data set.");
		return 0;
	}

	// Activate the data set:
	if (!imageTracker->activateDataSet(dataSetAnimal))
	{
		LOG("Failed to activate data set.");
		return 0;
	}

	LOG("Successfully loaded and activated data set.");
	return 1;
}


JNIEXPORT int JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_destroyTrackerData(JNIEnv *, jobject)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_destroyTrackerData");

	// Get the image tracker:
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
	QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
			trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
	if (imageTracker == NULL)
	{
		LOG("Failed to destroy the tracking data set because the ImageTracker has not"
				" been initialized.");
		return 0;
	}

	if(dataSetAnimal != 0)
	{
		if(imageTracker->getActiveDataSet() == dataSetAnimal &&
				!imageTracker->deactivateDataSet(dataSetAnimal))
		{
			LOG("Failed to destroy the tracking data set Horse because the data set "
					"could not be deactivated.");
			return 0;
		}

		if(!imageTracker->destroyDataSet(dataSetAnimal))
		{
			LOG("Failed to destroy the tracking data set HOrse.");
			return 0;
		}
		LOG("Successfully destroyed the data set Horse");
		dataSetAnimal = 0;
	}
	return 1;
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_onQCARInitializedNative(JNIEnv *, jobject)
{
	// Register the update callback where we handle the data set swap:
	QCAR::registerCallback(&updateCallback);

	// Comment in to enable tracking of up to 2 targets simultaneously and
	// split the work over multiple frames:
	// QCAR::setHint(QCAR::HINT_MAX_SIMULTANEOUS_IMAGE_TARGETS, 2);
	// QCAR::setHint(QCAR::HINT_IMAGE_TARGET_MULTI_FRAME_ENABLED, 1);
}


void testMatrix(QCAR::Matrix44F* matrix){

}

float angleRotate = 0;
JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargetsRenderer_renderFrame(JNIEnv * env, jobject)
{
	//LOG("Java_com_dennis_skripsi_ImageTargets_GLRenderer_renderFrame");
	// Clear color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// Get the state from QCAR and mark the beginning of a rendering section
	QCAR::State state = QCAR::Renderer::getInstance().begin();

	// Explicitly render the Video Background
	QCAR::Renderer::getInstance().drawVideoBackground();

#ifdef USE_OPENGL_ES_1_1
	// Set GL11 flags:
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

#endif

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	float* vertex;
	float* norm;
	float* textCoord;
	int numVert;
	// Did we find any trackables this frame?
	for(int tIdx = 0; tIdx < state.getNumTrackableResults(); tIdx++)
	{
		// Get the trackable:
		const QCAR::TrackableResult* result = state.getTrackableResult(tIdx);
		const QCAR::Trackable & trackable = result->getTrackable();
		QCAR::Matrix44F modelViewMatrix = QCAR::Tool::convertPose2GLMatrix(result->getPose());

		const QCAR::ImageTargetResult* imageResult = static_cast<const QCAR::ImageTargetResult*> (result);

		int textureIndex;

		GLfloat vbVertices[imageResult->getNumVirtualButtons()*24];

		unsigned char vbCounter = 0;

		for(int i = 0; i < imageResult->getNumVirtualButtons(); i++)
		{
			const QCAR::VirtualButtonResult* buttonResult = imageResult->getVirtualButtonResult(i);
			const QCAR::VirtualButton& button = buttonResult-> getVirtualButton();

			LOG("virtual button from %s found , button name: %s", trackable.getName(), button.getName());
			if(buttonResult->isPressed() && !soundIsPlayed)
			{
				LOG("pressed! %s", trackable.getName());
				if(strcmp(trackable.getName(), "horse") == 0)
					playSoundEffect(0,1);
				else if(strcmp(trackable.getName(), "dog") == 0)
					playSoundEffect(1,1);
				else if(strcmp(trackable.getName(), "cat") == 0)
					playSoundEffect(2,1);
				else if(strcmp(trackable.getName(), "cow") == 0)
					playSoundEffect(3,1);
				else if(strcmp(trackable.getName(), "fish") == 0)
					playSoundEffect(4,1);
				else if(strcmp(trackable.getName(), "pig") == 0)
					playSoundEffect(5,1);
				else if(strcmp(trackable.getName(), "tiger") == 0)
					playSoundEffect(6,1);
				soundIsPlayed = true;
			}
			else if(!buttonResult->isPressed()){
				//				LOG("button not pressed");
				soundIsPlayed = false;
			}
			const QCAR::Area* vbArea = &button.getArea();
			assert(vbArea->getType() == QCAR::Area::RECTANGLE);
			const QCAR::Rectangle* vbRectangle = static_cast<const QCAR::Rectangle*>(vbArea);

			// We add the vertices to a common array in order to have one single
			// draw call. This is more efficient than having multiple glDrawArray calls
			vbVertices[vbCounter   ]=vbRectangle->getLeftTopX();
			vbVertices[vbCounter+ 1]=vbRectangle->getLeftTopY();
			vbVertices[vbCounter+ 2]=0.0f;
			vbVertices[vbCounter+ 3]=vbRectangle->getRightBottomX();
			vbVertices[vbCounter+ 4]=vbRectangle->getLeftTopY();
			vbVertices[vbCounter+ 5]=0.0f;
			vbVertices[vbCounter+ 6]=vbRectangle->getRightBottomX();
			vbVertices[vbCounter+ 7]=vbRectangle->getLeftTopY();
			vbVertices[vbCounter+ 8]=0.0f;
			vbVertices[vbCounter+ 9]=vbRectangle->getRightBottomX();
			vbVertices[vbCounter+10]=vbRectangle->getRightBottomY();
			vbVertices[vbCounter+11]=0.0f;
			vbVertices[vbCounter+12]=vbRectangle->getRightBottomX();
			vbVertices[vbCounter+13]=vbRectangle->getRightBottomY();
			vbVertices[vbCounter+14]=0.0f;
			vbVertices[vbCounter+15]=vbRectangle->getLeftTopX();
			vbVertices[vbCounter+16]=vbRectangle->getRightBottomY();
			vbVertices[vbCounter+17]=0.0f;
			vbVertices[vbCounter+18]=vbRectangle->getLeftTopX();
			vbVertices[vbCounter+19]=vbRectangle->getRightBottomY();
			vbVertices[vbCounter+20]=0.0f;
			vbVertices[vbCounter+21]=vbRectangle->getLeftTopX();
			vbVertices[vbCounter+22]=vbRectangle->getLeftTopY();
			vbVertices[vbCounter+23]=0.0f;
			vbCounter+=24;
		}

		int num = imageResult->getNumVirtualButtons();

		if(strcmp(trackable.getName(), "horse") == 0)
		{
			//			LOG("horse found");
			textureIndex  = 0;
			vertex = horseVerts;
			norm = horseNormals;
			textCoord = horseTexCoords;
			numVert = horseNumVerts;
		}
		else if (strcmp(trackable.getName(), "dog") == 0)
		{
			//			LOG("dog found");
			textureIndex = 1;
			vertex = dogVerts;
			norm = dogNormals;
			textCoord = dogTexCoords;
			numVert = dogNumVerts;
		}
		else if (strcmp(trackable.getName(), "cat") == 0)
		{
			//			LOG("dog found");
			textureIndex = 2;
			vertex = catVerts;
			norm = catNormals;
			textCoord = catTexCoords;
			numVert = catNumVerts;
		}
		else if (strcmp(trackable.getName(), "cow") == 0)
		{
			//			LOG("dog found");
			textureIndex = 3;
			vertex = cowVerts;
			norm = cowNormals;
			textCoord = cowTexCoords;
			numVert = cowNumVerts;
		}
		else if (strcmp(trackable.getName(), "fish") == 0)
		{
			//			LOG("dog found");
			textureIndex = 4;
			vertex = fishVerts;
			norm = fishNormals;
			textCoord = fishTexCoords;
			numVert = fishNumVerts;
		}
		else if (strcmp(trackable.getName(), "pig") == 0)
		{
			//			LOG("dog found");
			textureIndex = 5;
			vertex = pigVerts;
			norm = pigNormals;
			textCoord = pigTexCoords;
			numVert = pigNumVerts;
		}
		//		else if (strcmp(trackable.getName(), "tiger") == 0)
		//		{
		//		//			LOG("dog found");
		//			textureIndex = 6;
		//			vertex = tigerVerts;
		//			norm = tigerNormals;
		//			textCoord = tigerTexCoords;
		//			numVert = tigerNumVerts;
		//		}

		else
		{
			textureIndex  = 0;
			vertex = horseVerts;
			norm = horseNormals;
			textCoord = horseTexCoords;
			numVert = horseNumVerts;
		}


		const Texture* const thisTexture = textures[textureIndex];

#ifdef USE_OPENGL_ES_1_1
		// Load projection matrix:
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(projectionMatrix.data);

		// Load model view matrix:
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(modelViewMatrix.data);
		glTranslatef(0.f, 0.f, kObjectScale);
		glScalef(kObjectScale, kObjectScale, kObjectScale);

		// Draw object:
		glBindTexture(GL_TEXTURE_2D, thisTexture->mTextureID);
		glTexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*) &teapotTexCoords[0]);
		glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*) &teapotVertices[0]);
		glNormalPointer(GL_FLOAT, 0,  (const GLvoid*) &teapotNormals[0]);
		glDrawElements(GL_TRIANGLES, NUM_TEAPOT_OBJECT_INDEX, GL_UNSIGNED_SHORT,
				(const GLvoid*) &teapotIndices[0]);
#else

		QCAR::Matrix44F modelViewProjection;

		SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
				&modelViewMatrix.data[0] ,
				&modelViewProjection.data[0]);


		if(vbCounter>0)
		{
			glUseProgram(vbShaderProgramID);

			glVertexAttribPointer(vbVertexHandle, 3, GL_FLOAT,GL_FALSE, 0,
					(const GLvoid*) &vbVertices[0]);

			glEnableVertexAttribArray(vbVertexHandle);
			glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
					(GLfloat*)&modelViewProjection.data[0] );

			// We multiply by 8 because that's the number of vertices per button
			// The reason is that GL_LINES considers only pairs. So some vertices
			// must be repeated.
			glDrawArrays(GL_LINES, 0, imageResult->getNumVirtualButtons()*8);

			SampleUtils::checkGlError("VirtualButtons drawButton");

			glDisableVertexAttribArray(vbVertexHandle);
		}

		SampleUtils::translatePoseMatrix(0.0f, 0.0f, 0.0f,
				&modelViewMatrix.data[0]);
		SampleUtils::scalePoseMatrix(kObjectScale, kObjectScale, kObjectScale,
				&modelViewMatrix.data[0]);
		SampleUtils::rotatePoseMatrix(90,1,0,0, &modelViewMatrix.data[0]);
		SampleUtils::rotatePoseMatrix(angleRotate,0,1,0, &modelViewMatrix.data[0]);
		SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
				&modelViewMatrix.data[0] ,
				&modelViewProjection.data[0]);

		angleRotate+= 1;



		glUseProgram(shaderProgramID);

		glVertexAttribPointer(vertexHandle, 3, GL_FLOAT, GL_FALSE, 0,
				(const GLvoid*) &vertex[0]);
		glVertexAttribPointer(normalHandle, 3, GL_FLOAT, GL_FALSE, 0,
				(const GLvoid*) &norm[0]);
		glVertexAttribPointer(textureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0,
				(const GLvoid*) &textCoord[0]);

		glEnableVertexAttribArray(vertexHandle);
		glEnableVertexAttribArray(normalHandle);
		glEnableVertexAttribArray(textureCoordHandle);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, thisTexture->mTextureID);
		glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
				(GLfloat*)&modelViewProjection.data[0] );

		glDrawArrays(GL_TRIANGLES, 0, numVert);
		SampleUtils::checkGlError("ImageTargets renderFrame");
#endif

	}

	vertex = NULL;
	norm = NULL;
	textCoord =NULL;

	glDisable(GL_DEPTH_TEST);

#ifdef USE_OPENGL_ES_1_1        
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
	glDisableVertexAttribArray(vertexHandle);
	glDisableVertexAttribArray(normalHandle);
	glDisableVertexAttribArray(textureCoordHandle);
#endif

	QCAR::Renderer::getInstance().end();
}

void
configureVideoBackground()
{
	// Get the default video mode:
	QCAR::CameraDevice& cameraDevice = QCAR::CameraDevice::getInstance();
	QCAR::VideoMode videoMode = cameraDevice.
			getVideoMode(QCAR::CameraDevice::MODE_DEFAULT);


	// Configure the video background
	QCAR::VideoBackgroundConfig config;
	config.mEnabled = true;
	config.mSynchronous = true;
	config.mPosition.data[0] = 0.0f;
	config.mPosition.data[1] = 0.0f;

	if (isActivityInPortraitMode)
	{
		//LOG("configureVideoBackground PORTRAIT");
		config.mSize.data[0] = videoMode.mHeight
				* (screenHeight / (float)videoMode.mWidth);
		config.mSize.data[1] = screenHeight;

		if(config.mSize.data[0] < screenWidth)
		{
			LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
			config.mSize.data[0] = screenWidth;
			config.mSize.data[1] = screenWidth *
					(videoMode.mWidth / (float)videoMode.mHeight);
		}
	}
	else
	{
		//LOG("configureVideoBackground LANDSCAPE");
		config.mSize.data[0] = screenWidth;
		config.mSize.data[1] = videoMode.mHeight
				* (screenWidth / (float)videoMode.mWidth);

		if(config.mSize.data[1] < screenHeight)
		{
			LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
			config.mSize.data[0] = screenHeight
					* (videoMode.mWidth / (float)videoMode.mHeight);
			config.mSize.data[1] = screenHeight;
		}
	}

	LOG("Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, screenWidth, screenHeight, config.mSize.data[0], config.mSize.data[1]);

	// Set the config:
	QCAR::Renderer::getInstance().setVideoBackgroundConfig(config);
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_initApplicationNative(
		JNIEnv* env, jobject obj, jint width, jint height)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_initApplicationNative");

	// Store screen dimensions
	screenWidth = width;
	screenHeight = height;

	// Handle to the activity class:
	jclass activityClass = env->GetObjectClass(obj);

	jmethodID getTextureCountMethodID = env->GetMethodID(activityClass,
			"getTextureCount", "()I");
	if (getTextureCountMethodID == 0)
	{
		LOG("Function getTextureCount() not found.");
		return;
	}

	textureCount = env->CallIntMethod(obj, getTextureCountMethodID);
	if (!textureCount)
	{
		LOG("getTextureCount() returned zero.");
		return;
	}

	textures = new Texture*[textureCount];

	jmethodID getTextureMethodID = env->GetMethodID(activityClass,
			"getTexture", "(I)Lcom/dennis/skripsi/ImageTargets/Texture;");

	if (getTextureMethodID == 0)
	{
		LOG("Function getTexture() not found.");
		return;
	}

	// Register the textures
	for (int i = 0; i < textureCount; ++i)
	{

		jobject textureObject = env->CallObjectMethod(obj, getTextureMethodID, i);
		if (textureObject == NULL)
		{
			LOG("GetTexture() returned zero pointer");
			return;
		}

		textures[i] = Texture::create(env, textureObject);
	}
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_initApplicationNative finished");

}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_deinitApplicationNative(
		JNIEnv* env, jobject obj)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_deinitApplicationNative");

	// Release texture resources
	if (textures != 0)
	{
		for (int i = 0; i < textureCount; ++i)
		{
			delete textures[i];
			textures[i] = NULL;
		}

		delete[]textures;
		textures = NULL;

		textureCount = 0;
	}
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_startCamera(JNIEnv *,
		jobject)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_startCamera");

	// Initialize the camera:
	if (!QCAR::CameraDevice::getInstance().init())
		return;

	// Configure the video background
	configureVideoBackground();

	// Select the default mode:
	if (!QCAR::CameraDevice::getInstance().selectVideoMode(
			QCAR::CameraDevice::MODE_DEFAULT))
		return;

	// Start the camera:
	if (!QCAR::CameraDevice::getInstance().start())
		return;

	// Uncomment to enable flash
	//if(QCAR::CameraDevice::getInstance().setFlashTorchMode(true))
	//	LOG("IMAGE TARGETS : enabled torch");

	// Uncomment to enable infinity focus mode, or any other supported focus mode
	// See CameraDevice.h for supported focus modes
	//if(QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_INFINITY))
	//	LOG("IMAGE TARGETS : enabled infinity focus");

	// Start the tracker:
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
	QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
	if(imageTracker != 0)
		imageTracker->start();
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_stopCamera(JNIEnv *, jobject)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_stopCamera");

	// Stop the tracker:
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
	QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
	if(imageTracker != 0)
		imageTracker->stop();

	QCAR::CameraDevice::getInstance().stop();
	QCAR::CameraDevice::getInstance().deinit();
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_setProjectionMatrix(JNIEnv *, jobject)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargets_setProjectionMatrix");

	// Cache the projection matrix:
	const QCAR::CameraCalibration& cameraCalibration =
			QCAR::CameraDevice::getInstance().getCameraCalibration();
	projectionMatrix = QCAR::Tool::getProjectionGL(cameraCalibration, 2.0f,
			2000.0f);
}


JNIEXPORT jboolean JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_activateFlash(JNIEnv*, jobject, jboolean flash)
{
	return QCAR::CameraDevice::getInstance().setFlashTorchMode((flash==JNI_TRUE)) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_autofocus(JNIEnv*, jobject)
{
	return QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_TRIGGERAUTO) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargets_setFocusMode(JNIEnv*, jobject, jint mode)
{
	int qcarFocusMode;

	switch ((int)mode)
	{
	case 0:
		qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_NORMAL;
		break;

	case 1:
		qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_CONTINUOUSAUTO;
		break;

	case 2:
		qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_INFINITY;
		break;

	case 3:
		qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_MACRO;
		break;

	default:
		return JNI_FALSE;
	}

	return QCAR::CameraDevice::getInstance().setFocusMode(qcarFocusMode) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargetsRenderer_initRendering(
		JNIEnv* env, jobject obj)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargetsRenderer_initRendering");

	// Define clear color
	glClearColor(0.0f, 0.0f, 0.0f, QCAR::requiresAlpha() ? 0.0f : 1.0f);

	// Now generate the OpenGL texture objects and add settings
	for (int i = 0; i < textureCount; ++i)
	{
		glGenTextures(1, &(textures[i]->mTextureID));
		glBindTexture(GL_TEXTURE_2D, textures[i]->mTextureID);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i]->mWidth,
				textures[i]->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				(GLvoid*)  textures[i]->mData);
	}
#ifndef USE_OPENGL_ES_1_1

	//OpenGL setup for 3D model

	shaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
			cubeFragmentShader);

	vertexHandle        = glGetAttribLocation(shaderProgramID,
			"vertexPosition");
	normalHandle        = glGetAttribLocation(shaderProgramID,
			"vertexNormal");
	textureCoordHandle  = glGetAttribLocation(shaderProgramID,
			"vertexTexCoord");
	mvpMatrixHandle     = glGetUniformLocation(shaderProgramID,
			"modelViewProjectionMatrix");

	// OpenGL setup for Virtual Buttons
	vbShaderProgramID   = SampleUtils::createProgramFromBuffer(lineMeshVertexShader,
			lineFragmentShader);

	vbVertexHandle      = glGetAttribLocation(vbShaderProgramID, "vertexPosition");

#endif

}


JNIEXPORT void JNICALL
Java_com_dennis_skripsi_ImageTargets_ImageTargetsRenderer_updateRendering(
		JNIEnv* env, jobject obj, jint width, jint height)
{
	LOG("Java_com_dennis_skripsi_ImageTargets_ImageTargetsRenderer_updateRendering");

	// Update screen dimensions
	screenWidth = width;
	screenHeight = height;

	// Reconfigure the video background
	configureVideoBackground();
}


#ifdef __cplusplus
}
#endif
