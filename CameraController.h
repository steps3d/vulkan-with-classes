
//
// Controller to handle camera
//

#pragma once

#include	"Camera.h"
#include	"Controller.h"

class	CameraController : public Controller
{
	Camera	camera;
	float		yaw       = 0;
	float		pitch     = 0;
	float		roll      = 0;
	float		fov       = 60;
	float		step      = 0.2f;
	float		sideStep  = 0.2f;
	double		lastX = -1;
	double		lastY = -1;
	bool		forwardPressed  = false;
	bool		leftPressed     = false;
	bool		rightPressed    = false;
	bool		backwardPressed = false;
	
public:
	CameraController  ( VulkanWindow * ptr ) : Controller ( ptr ), camera ( glm::vec3(1),  0, 0, 0 ) 
	{
		camera.setRightHanded ( false );
	}

	~CameraController () {}

	void	setStep ( float v )
	{
		step = v;	
	}
	
	void	setSideStep ( float v )
	{
		sideStep = v;	
	}
	
	virtual	glm::mat4	getModelView  () const
	{
		glm::vec3 up   = camera.getUpDir   ();
		glm::vec3 view = camera.getViewDir ();
		glm::vec3 pos  = camera.getPos     ();
		
		return glm::lookAt ( pos, pos + view, up );
	}

	virtual	glm::mat4	getProjection () const
	{
		return camera.getProjection ();
	}

	glm::mat3 normalMatrix ( const glm::mat4& mv ) const
	{
		return glm::inverseTranspose ( glm::mat3 ( mv ) );
	}
	
	virtual	glm::vec3	getEye () const
	{
		return camera.getPos ();
	}

	virtual	float		getFov () const
	{
		return fov;
	}

		// handlers
	virtual	void	mouseMotion ( double x, double y )
	{
		if ( lastX == -1 )				// not initialized
		{
			lastX = x;
			lastY = y;
		}

		yaw   += float (x - lastX) * 0.01f;
		pitch += float (y - lastY) * 0.01f;
		lastX  = x;
		lastY  = y;

		camera.setEulerAngles ( yaw, pitch, roll );
	}
/*
	virtual	void	mouseClick  ( int button, int action, int mods ) 
	{
		if ( button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS )
		{
			double x, y;

			glfwGetCursorPos ( window->getWindow (), &x, &y );

			mouseOldX = x;
			mouseOldY = y;
			pressed   = true;
		}
		else if ( button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE )
			pressed = false;
	}
*/
	virtual	void	keyTyped    ( int key, int scancode, int action, int mods ) 
	{
		if ( action == GLFW_RELEASE )
		{
			if ( key == 'W' )
				forwardPressed = false;
			else
			if ( key == 'X' )
				backwardPressed = false;
			else
			if ( key == 'A' )
				leftPressed = false;
			else
			if ( key == 'D' )
				rightPressed = false;
		}
		
		log () << "Key typed: " << key << " " << action << " " << mods << Log::endl;

		if ( action == GLFW_PRESS )
		{
			if ( key == 'W' )
			{
				forwardPressed = true;
				camera.moveBy ( step * camera.getViewDir () );
			}
			else
			if ( key == 'X' )
			{
				backwardPressed = true;
				camera.moveBy ( -step * camera.getViewDir () );
			}
			else
			if ( key == 'A' )
			{
				leftPressed = true;
				camera.moveBy ( -sideStep * camera.getSideDir () );
			}
			else
			if ( key == 'D' )
			{
				rightPressed = true;
				camera.moveBy ( sideStep * camera.getSideDir () );
			}
		}
	}
	
	virtual	void	timeElapsed ( double delta ) override
	{
		float	dt = (float) delta;

		if ( forwardPressed )
			camera.moveBy ( dt * step * camera.getViewDir () );
			
		if ( backwardPressed )
			camera.moveBy ( -dt * step * camera.getViewDir () );
		
		if ( leftPressed )
			camera.moveBy ( -dt * sideStep * camera.getSideDir () );

		if ( rightPressed )
			camera.moveBy ( dt * sideStep * camera.getSideDir () );
	}
};
