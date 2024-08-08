
#pragma once

class	Controller
{
protected:
	VulkanWindow * window = nullptr;

public:
	Controller  ( VulkanWindow * ptr ) : window ( ptr ) {}
	~Controller () = default;

	virtual	glm::mat4	getModelView  () const = 0;
	virtual	glm::mat4	getProjection () const = 0;
	virtual	glm::vec3	getEye        () const = 0;
	virtual	float		getFov        () const = 0;

		// return normal matrix from modelView
	virtual	glm::mat4	getNormalMatrix () const
	{
		return glm::inverseTranspose ( getModelView () );
	}

		// handlers
	virtual	void	mouseMotion ( double x, double y ) {}
	virtual	void	mouseClick  ( int button, int action, int mods ) {}
	virtual	void	mouseWheel  ( double xOffset, double yOffset ) {}
	virtual	void	keyTyped    ( int key, int scancode, int action, int mods ) {}
	virtual	void	timeElapsed ( double delta ) {}
};

class	RotateController : public Controller
{
	double		mouseOldX = 0;
	double		mouseOldY = 0;
	float		fov       = 60;
	float		zNear     = 0.01f;
	float		zFar      = 200.0f;
	glm::vec3	rot       = glm::vec3 ( 0.0f );
	glm::vec3	eye       = glm::vec3 ( 5, 5, 5 );
	bool		pressed   = false;

public:
	RotateController  ( VulkanWindow * ptr ) : Controller ( ptr ) {}
	RotateController  ( VulkanWindow * ptr, const glm::vec3& e ) : Controller ( ptr ), eye ( e ) {}
	~RotateController () {}

	void	setDepthRange ( float n = 0.01f, float f = 200.0f )
	{
		zNear = n;
		zFar  = f;
	}

	virtual	glm::mat4	getModelView  () const
	{
		glm::mat4 m ( 1 );

		m = glm::rotate ( m, glm::radians(-rot.z), glm::vec3(0, 0, 1) );
		m = glm::rotate ( m, glm::radians(rot.y),  glm::vec3(0, 1, 0) );
		m = glm::rotate ( m, glm::radians(rot.x),  glm::vec3(1, 0, 0) );

		return glm::lookAt ( eye, glm::vec3 ( 0.0f ), glm::vec3 ( 0, 0, 1 ) ) * m;
	}

	virtual	glm::mat4	getProjection () const
	{
		//glm::mat4 proj = glm::perspective ( glm::radians( fov ), window->getAspect (), zNear, zFar );
		//proj [1][1] *= -1;

		return projectionMatrix ( fov, window->getAspect (), zNear, zFar );
	}

	virtual	void	setEye ( const glm::vec3& e ) 
	{
		eye = e;
	}

	virtual	glm::vec3	getEye () const
	{
		return eye;
	}

	virtual	float		getFov () const
	{
		return fov;
	}

		// handlers
	virtual	void	mouseMotion ( double x, double y )
	{
		if ( !pressed )
			return;

		rot.x -= (float)((mouseOldY - y) * 180.0f) / 200.0f;
		rot.z -= (float)((mouseOldX - x) * 180.0f) / 200.0f;
		rot.y  = 0;

		if ( rot.z > 360 )
			rot.z -= 360;

		if ( rot.z < -360 )
			rot.z += 360;

		if ( rot.y > 360 )
			rot.y -= 360;

		if ( rot.y < -360 )
			rot.y += 360;

		mouseOldX = x;
		mouseOldY = y;
	}

	virtual	void	mouseClick  ( int button, int action, int mods ) 
	{
		if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS )
		{
			double x, y;

			glfwGetCursorPos ( window->getWindow (), &x, &y );

			mouseOldX = x;
			mouseOldY = y;
			pressed   = true;
		}
		else if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE )
			pressed = false;
	}

	virtual	void	mouseWheel  ( double xOffset, double yOffset )
	{
		eye -= float(yOffset) * glm::normalize ( eye );
	}
};
