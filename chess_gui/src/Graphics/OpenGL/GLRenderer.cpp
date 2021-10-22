#include "GLRenderer.h"
#include "Graphics/Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <Exception.h>
#include <sstream>


#if !defined(NDEBUG) || !defined(NGLDEBUG)
#define GLDEBUGCALL(X) GLError::GetErrorCode();GLError::errorCodes.clear(); X; GLError::GetErrorCode(); \
	if(!GLError::errorCodes.empty()) throw GLException(__LINE__ , __FILE__, GLError::errorCodes.back(), #X);
#else
#define GLDEBUGCALL(X) X
#endif




namespace GLError
{
    std::vector<GLenum> errorCodes;
    inline void GetErrorCode()
    {
        GLenum errorCode;
        while((errorCode = glGetError()) != GL_NO_ERROR)
            errorCodes.push_back(errorCode);
    }
};

GLException::GLException(int line, const char* fileName, GLenum errorCode, std::string functionName)
    :
        CException(line, fileName)
{
    this->errorCode =errorCode;
    this->functionName = functionName;
	type = "GLException";
}

const char* GLException::what() const noexcept
{
	GetOriginalString();
	std::ostringstream oss(whatBuffer);
	oss << "[FUNCTION] :" << functionName << "\n"
		<< "[ERROR CODE] :" << errorCode << "\n"
		<< "[ERROR] :" << CheckError() << std::endl;
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

std::string GLException::CheckError() const noexcept 
{
    std::stringstream original;
    std::string errorName;
    switch(errorCode)
    {
        case GL_INVALID_OPERATION:
            errorName = "Invalid operation";
            break;
        case GL_INVALID_ENUM:
            errorName = "Invalid enum";
            break;
        case GL_INVALID_VALUE:
            errorName = "Invalid value";
            break;
		default:
			errorName = "Unknown Error";
    };
    original << "[ERROR CODE]: " << errorCode << std::endl
        << "[ERROR STRING]: " << errorName  << std::endl
        << "[FUNCTION NAME]: " << functionName << std::endl;
    return original.str();
}




GLIndexBinder::GLIndexBinder(uint32_t rendererID)
{
    this->rendererID = rendererID;
}

void GLIndexBinder::Bind() const noexcept 
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererID);
}

void GLIndexBinder::UnBind() const noexcept
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLVertexBinder::GLVertexBinder(uint32_t rendererID)
{
    this->rendererID = rendererID;
}

void GLVertexBinder::Bind() const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, rendererID);
}

void GLVertexBinder::UnBind() const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLTextureBinder::GLTextureBinder(uint32_t rendererID)
{
    this->rendererID = rendererID;
}

GLTextureBinder::GLTextureBinder(uint32_t rendererID, GLenum target)
{
    this->rendererID = rendererID;
    this->target = target;
}

void GLTextureBinder::Bind() const noexcept
{
    glBindTexture(target, rendererID);
}

void GLTextureBinder::UnBind() const noexcept
{
    glBindTexture(target, 0);
}

NativeShaderHandler<GLRenderer>::NativeShaderHandler(ShaderType shaderType)
{
	type = shaderType;
	GLenum glShaderType;
	switch(type)
	{
		case ShaderType::FRAGMENT:
			glShaderType = GL_FRAGMENT_SHADER;
			break;
		case ShaderType::VERTEX:
			glShaderType = GL_VERTEX_SHADER;
			break;
		default:
			throw CException(__LINE__, __FILE__, "Type Not Found", "Shader Type not found!");
	};
	shader = glCreateShader(glShaderType);
}

void NativeShaderHandler<GLRenderer>::Load()
{    
	GLenum shaderType;
    switch(type)
    {
        case ShaderType::VERTEX:
            shaderType = GL_VERTEX_SHADER;
            break;
        case ShaderType::FRAGMENT:
            shaderType = GL_FRAGMENT_SHADER;
            break;
    };
    const char* shaderSource = source.c_str();
    GLDEBUGCALL(glShaderSource(shader, 1, &shaderSource, nullptr));
    GLDEBUGCALL(glCompileShader(shader));
    int32_t shaderCompiled;
    GLDEBUGCALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled));
    if(shaderCompiled != GL_TRUE)
    {
        char message[1024];
		int32_t logLength;
        GLDEBUGCALL(glGetShaderInfoLog(shader, 1024, &logLength, message));
        throw CException(__LINE__, __FILE__, "Shader Compile Error", std::string(message) + (shaderType == GL_VERTEX_SHADER?"Vertex Shader":"Fragment Shader"));
    }
}

void NativeShaderHandler<GLRenderer>::UnLoad()
{
}

NativeShaderHandler<GLRenderer>::~NativeShaderHandler()
{
	glDeleteShader(shader);
}

NativeShaderStageHandler<GLRenderer>::NativeShaderStageHandler()
{
	program = glCreateProgram();
}

void NativeShaderStageHandler<GLRenderer>::Load()
{
	if(!linked)
	{
		for(auto& i: shaderHandler)
		{
			i->Load();
			GLDEBUGCALL(glAttachShader(program, dynamic_cast<NativeShaderHandler<GLRenderer>*>(i.get())->shader));
		}
		GLDEBUGCALL(glLinkProgram(program));
		int32_t programLinked;
		GLDEBUGCALL(glGetProgramiv(program, GL_LINK_STATUS, &programLinked));
		if(programLinked != GL_TRUE)
		{
			char message[1024];
			int32_t logLength;
			GLDEBUGCALL(glGetProgramInfoLog(program, 1024, &logLength, message));
			throw CException(__LINE__, __FILE__, "GL Program Link Error", message);
		}
		shaderHandler.clear();
	}
	glUseProgram(program);
	renderer->shaderProgram = program;
}

void NativeShaderStageHandler<GLRenderer>::UnLoad()
{
	glUseProgram(0);
}

NativeShaderStageHandler<GLRenderer>::~NativeShaderStageHandler()
{
	UnLoad();
	glDeleteProgram(program);
}

GLRenderer::GLRenderer()
{
    LoadGladGL();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
    GLDEBUGCALL(glGenVertexArrays(1, &pvao));
    glBindVertexArray(pvao);
} 

bool GLRenderer::gladLoaded = false;

void GLRenderer::Uniform1f(const uint32_t count, const float* data, std::string name)
{
	if(count == 1)
    	glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), *data);
	else
	{
		glUniform1fv(glGetUniformLocation(shaderProgram, name.c_str()), count,  data);
	}
}

void GLRenderer::Uniform1u(const uint32_t count, const uint32_t* data, std::string name)
{
	if(count == 1)
		glUniform1ui(glGetUniformLocation(shaderProgram, name.c_str()), *data);
	else
	{
		glUniform1uiv(glGetUniformLocation(shaderProgram, name.c_str()), count, data);
	}
}

void GLRenderer::Uniform1i(const uint32_t count, const int32_t* data, std::string name)
{
	if(count == 1)
		glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), *data);
	else
	{
		glUniform1iv(glGetUniformLocation(shaderProgram, name.c_str()), count, data);
	}
}

void GLRenderer::Uniform2f(const uint32_t count, const Vect2* data, std::string name)
{
	if(count == 1)
    	glUniform2f(glGetUniformLocation(shaderProgram, name.c_str()), data->x, data->y);
	else
	{
		glUniform2fv(glGetUniformLocation(shaderProgram, name.c_str()), count, (float*)data);
	}
	
}

void GLRenderer::Uniform3f(const uint32_t count, const Vect3* data, std::string name)
{
	if(count == 1)
    	glUniform3f(glGetUniformLocation(shaderProgram, name.c_str()), data->x, data->y, data->z);
	else
	{
	}
}

void GLRenderer::Uniform4f(const uint32_t count, const Vect4* data, std::string name)
{
	if(count == 1)
		glUniform4f(glGetUniformLocation(shaderProgram, name.c_str()), data->x, data->y, data->z, data->w);
	else
	{
		glUniform4fv(glGetUniformLocation(shaderProgram, name.c_str()), count,  (float*)data);
	}
}

void GLRenderer::UniformMat(const uint32_t count, const Mat* mat, std::string name)
{
    GLDEBUGCALL(uint32_t uLocation = glGetUniformLocation(shaderProgram, name.c_str()));
	float* answer = new float[mat->sizet * count];
	for(uint32_t i = 0; i < count; i++)
	{
		std::copy(mat[i].buffer.get(), mat[i].buffer.get() + mat[i].sizet, answer + (i * mat->sizet * sizeof(float)));
	}
    switch(mat->dimension.column)
    {
        case 2:
            switch(mat->dimension.row)
            {
                case 2:
                    glUniformMatrix2fv(uLocation, count, GL_FALSE, answer);
                    break;
                case 3:
                    glUniformMatrix2x3fv(uLocation, count, GL_FALSE, answer);
                    break;
                case 4:
                    glUniformMatrix2x4fv(uLocation, count,  GL_FALSE, answer);
                    break;
            }
            break;
        case 3:
            switch(mat->dimension.row)
            {
                case 2:
                    glUniformMatrix3x2fv(uLocation, count, GL_FALSE, answer);
                    break;
                case 3:
                    glUniformMatrix3fv(uLocation, count, GL_FALSE, answer);
                    break;
                case 4:
                    glUniformMatrix3x4fv(uLocation, count, GL_FALSE, answer);
                    break;
            }
            break;
        case 4:
            switch(mat->dimension.row)
            {
                case 2:
                    glUniformMatrix4x2fv(uLocation, count, GL_FALSE, answer);
                    break;
                case 3:
                    glUniformMatrix4x3fv(uLocation, count, GL_FALSE, answer);
                    break;
                case 4:
                    GLDEBUGCALL(glUniformMatrix4fv(uLocation, count, GL_FALSE, answer));
                    break;
            }
            break;

    }
}

void GLRenderer::ClearColor(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
}

void GLRenderer::ClearDepth(float depthLevel)
{
    glClearDepth(depthLevel);
}

void GLRenderer::FinalizeVertexSpecification()
{
	
}



void GLRenderer::LoadBuffer(GBuffer* gBuffer)
{
	glBindVertexArray(pvao);
    GLenum flags;
    GLenum bufferType;
    uint32_t rendererID;
    switch(gBuffer->bufferStyle.cpuFlags)
    {
        case GBuffer::GBufferStyle::CPUFlags::STATIC:
            switch(gBuffer->bufferStyle.usage)
            {
                case GBuffer::GBufferStyle::Usage::DRAW:
                    flags = GL_STATIC_DRAW;
                    break;
                case GBuffer::GBufferStyle::Usage::READ:
                    flags = GL_STATIC_READ;
                    break;
                case GBuffer::GBufferStyle::Usage::COPY:
                    flags = GL_STATIC_COPY;
                    break;
            };
            break;
        case GBuffer::GBufferStyle::CPUFlags::DYNAMIC:
            switch(gBuffer->bufferStyle.usage)
            {
                case GBuffer::GBufferStyle::Usage::DRAW:
                    flags = GL_DYNAMIC_DRAW;
                    break;
                case GBuffer::GBufferStyle::Usage::READ:
                    flags = GL_DYNAMIC_READ;
                    break;
                case GBuffer::GBufferStyle::Usage::COPY:
                    flags = GL_DYNAMIC_COPY;
                    break;
            };
            break;        
        case GBuffer::GBufferStyle::CPUFlags::STREAM:
            switch(gBuffer->bufferStyle.usage)
            {
                case GBuffer::GBufferStyle::Usage::DRAW:
                    flags = GL_STREAM_DRAW;
                    break;
                case GBuffer::GBufferStyle::Usage::READ:
                    flags = GL_STREAM_READ;
                    break;
                case GBuffer::GBufferStyle::Usage::COPY:
                    flags = GL_STREAM_COPY;
                    break;
            };
            break;
    }
    glGenBuffers(1, &rendererID);
    switch(gBuffer->bufferStyle.type)
    {
        case GBuffer::GBufferStyle::BufferType::VERTEX:
            bufferType = GL_ARRAY_BUFFER;
            gBuffer->gBinder.reset(new GLVertexBinder(rendererID));
            break;        
        case GBuffer::GBufferStyle::BufferType::INDEX:
            bufferType = GL_ELEMENT_ARRAY_BUFFER;
            gBuffer->gBinder.reset(new GLIndexBinder(rendererID));
            break;
    };

    GLDEBUGCALL(gBuffer->Bind());
    GLDEBUGCALL(glBufferData(bufferType, gBuffer->sizet, gBuffer->data, flags));
}

NativeShaderHandlerParent* GLRenderer::CreateShader(ShaderType type)
{
	return new NativeShaderHandler<GLRenderer>(type);
}

void GLRenderer::LoadTexture(Texture* texture, GBuffer* gBuffer)
{
    uint32_t rendererID;
	gBuffer = new GBuffer;
    GLDEBUGCALL(glGenTextures(1, &rendererID));
    GLenum type;
    switch(texture->type)
    {
        case Texture::Type::T1D:
            type = GL_TEXTURE_1D;
            glBindTexture(type, rendererID);
            glTexImage1D(type, 0, GL_RGBA, texture->width, 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data);
            break;
        case Texture::Type::T2D:
            type = GL_TEXTURE_2D;
            glBindTexture(type, rendererID);
            GLDEBUGCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data));
            break;
        case Texture::Type::T3D:
            type = GL_TEXTURE_3D;
            glBindTexture(type, rendererID);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texture->width, texture->height, texture->depth,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data);
            break;
    };
    gBuffer->gBinder = std::unique_ptr<GBinder>(new GLTextureBinder(rendererID, type));
    gBuffer->Bind();
    GLDEBUGCALL(glGenerateMipmap(type));
}

ShaderStageHandler* GLRenderer::CreateShaderStage()
{
	auto tmp = new NativeShaderStageHandler<GLRenderer>;
	tmp->renderer = this;
	return tmp;
}

void GLRenderer::UseShaderStage(ShaderStageHandler* shaderStageHandler)
{
	shaderStageHandler->Load();
}

void GLRenderer::Draw(DrawPrimitive drawPrimitive, GBuffer* gBuffer)
{
	gBuffer->Bind();
    GLDEBUGCALL(glDrawElements(GL_TRIANGLES, gBuffer->count, GL_UNSIGNED_INT, (const void*) 0));
}

void GLRenderer::DrawInstanced(DrawPrimitive drawPrimitive, GBuffer* gBuffer)
{
	gBuffer->Bind();
}

void GLRenderer::DrawArrays(DrawPrimitive drawPrimitive, GBuffer* gBuffer)
{
	GLenum target;
	switch(drawPrimitive)
	{
		case DrawPrimitive::POINTS:
			target = GL_POINTS;
			break;
		case DrawPrimitive::TRIANGLES:
			target = GL_TRIANGLES;
			break;
		case DrawPrimitive::TRIANGLES_ADJACENCIES:
			target = GL_TRIANGLES_ADJACENCY;
			break;
		case DrawPrimitive::TRIANGLES_STRIPS:
			target = GL_TRIANGLE_STRIP;
			break;
		default:
			throw CException(__LINE__, __FILE__, "Primitive Not found", "Primitive is not supported by this renderer \nPrimitive no: " 
					+ std::to_string(static_cast<uint32_t>(drawPrimitive)));
	};
	gBuffer->Bind();
	glDrawArrays(target, 0, gBuffer->count);
}

void GLRenderer::DrawBuffer(DrawPrimitive drawPrimitive, GBuffer* gBuffer)
{
	gBuffer->Bind();
	GLDEBUGCALL(glDrawBuffer(GL_TRIANGLES));
}

void GLRenderer::Clear()
{
    GLDEBUGCALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void GLRenderer::SetLayout(const uint32_t layout)
{
	glBindVertexArray(vaos[layout].first);
	auto& tmp = vaos[layout].second;
	uint32_t offset = 0, totalSize = tmp.back();
	for(uint32_t i = 0; i < tmp.size() - 1; i++)
	{
    	GLDEBUGCALL(glVertexAttribPointer(i, tmp[i], GL_FLOAT, GL_FALSE, 
                	totalSize, (const void*)offset));
        offset+= tmp[i] * sizeof(float);
	}
}

uint32_t GLRenderer::AddSpecification(VertexSpecification& vertexSpecification)
{
	uint32_t pvao;
	glGenVertexArrays(1, &pvao);
	glBindVertexArray(pvao);
    uint32_t totalSize = 0;
    uint32_t offset = 0;
    totalSize *= sizeof(float);
	vaos.emplace_back();
	vaos.back().first = pvao;
    for(uint32_t i = 0; i < vertexSpecification.size(); i++)
    {
		totalSize += vertexSpecification[i].second;
        GLDEBUGCALL(glBindAttribLocation(shaderProgram, i, vertexSpecification[i].first.c_str()));
		vaos.back().second.push_back(vertexSpecification[i].second);
        GLDEBUGCALL(glEnableVertexAttribArray(i));
    }
	vaos.back().second.push_back(totalSize);
	return vaos.size() - 1;
}


void GLRenderer::LoadGladGL()
{
	if(!gladLoaded)
	{
    	if(!gladLoadGLLoader(SDL_GL_GetProcAddress))
    	{
    	    if(!gladLoadGL())
    	    {
    	        throw CException(__LINE__, __FILE__, "glad Load error", "Can't load glad");
    	    }
    	}
		gladLoaded = true;
	}
}