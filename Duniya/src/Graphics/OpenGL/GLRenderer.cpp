#include "GLRenderer.hpp"

#include <SDL2/SDL.h>

#include <Exception.hpp>
#include <SDLUtiliy.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ECS/GraphicsComponent.hpp"
#include "Graphics/Renderer.hpp"
#include "SDL_gamecontroller.h"

#if !defined(NDEBUG) || !defined(NGLDEBUG)
#define GLDEBUGCALL(X)                \
    GLError::GetErrorCode();          \
    GLError::errorCodes.clear();      \
    X;                                \
    GLError::GetErrorCode();          \
    if (!GLError::errorCodes.empty()) \
	throw GLException(__LINE__, __FILE__, GLError::errorCodes.back(), #X);
#else
#define GLDEBUGCALL(X) X
#endif

namespace GLError {
std::vector<GLenum> errorCodes;
inline void GetErrorCode() {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
	errorCodes.push_back(errorCode);
}
};  // namespace GLError

GLException::GLException(int line, const char* fileName, GLenum errorCode,
			 std::string functionName)
    : CException(line, fileName) {
    this->errorCode = errorCode;
    this->functionName = functionName;
    type = "GLException";
}

const char* GLException::what() const noexcept {
    GetOriginalString();
    std::ostringstream oss(whatBuffer);
    oss << "[FUNCTION] :" << functionName << "\n"
	<< "[ERROR CODE] :" << errorCode << "\n"
	<< "[ERROR] :" << CheckError() << std::endl;
    whatBuffer = oss.str();
    return whatBuffer.c_str();
}

std::string GLException::CheckError() const noexcept {
    std::stringstream original;
    std::string errorName;
    switch (errorCode) {
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
	    errorName =
		"Unknown Error\n[ERROR CODE]: " + std::to_string(errorCode);
    };
    original << "[ERROR STRING]: " << errorName << std::endl
	     << "[FUNCTION NAME]: " << functionName << std::endl;
    return original.str();
}

GLIndexBinder::GLIndexBinder(uint32_t pvao, uint32_t rendererID)
    : pvao(pvao), rendererID(rendererID) {}

void GLIndexBinder::Bind() const noexcept {
    glBindVertexArray(pvao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererID);
}

void GLIndexBinder::UnBind() const noexcept {
    glBindVertexArray(pvao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLVertexBinder::GLVertexBinder(uint32_t pvao, uint32_t rendererID)
    : pvao(pvao), rendererID(rendererID) {}

void GLVertexBinder::Bind() const noexcept {
    glBindVertexArray(pvao);
    glBindBuffer(GL_ARRAY_BUFFER, rendererID);
}

void GLVertexBinder::UnBind() const noexcept {
    glBindVertexArray(pvao);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLTextureBinder::GLTextureBinder(uint32_t pvao, uint32_t rendererID,
				 GLenum target)
    : pvao(pvao), rendererID(rendererID), target(target) {}

void GLTextureBinder::Bind() const noexcept {
    glBindVertexArray(pvao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, rendererID);
}

void GLTextureBinder::UnBind() const noexcept {
    glBindVertexArray(pvao);
    glBindTexture(target, 0);
}

NativeShaderHandler<GLRenderer>::NativeShaderHandler(ShaderType shaderType) {
    type = shaderType;
    GLenum glShaderType;
    switch (type) {
	case ShaderType::FRAGMENT:
	    glShaderType = GL_FRAGMENT_SHADER;
	    break;
	case ShaderType::VERTEX:
	    glShaderType = GL_VERTEX_SHADER;
	    break;
	default:
	    throw CException(__LINE__, __FILE__, "Type Not Found",
			     "Shader Type not found!");
    };
    shader = glCreateShader(glShaderType);
}

void NativeShaderHandler<GLRenderer>::Load() {
    GLenum shaderType;
    switch (type) {
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
    if (shaderCompiled != GL_TRUE) {
	size_t debugMessageSize = 2048;
	char message[debugMessageSize];
	int32_t logLength;
	GLDEBUGCALL(
	    glGetShaderInfoLog(shader, debugMessageSize, &logLength, message));
	throw CException(__LINE__, __FILE__, "Shader Compile Error",
			 std::string(message) + (shaderType == GL_VERTEX_SHADER
						     ? "Vertex Shader"
						     : "Fragment Shader"));
    }
}

void NativeShaderHandler<GLRenderer>::UnLoad() {}

NativeShaderHandler<GLRenderer>::~NativeShaderHandler() {
    glDeleteShader(shader);
}

NativeShaderStageHandler<GLRenderer>::NativeShaderStageHandler() {
    program = glCreateProgram();
    linked = false;
}

void NativeShaderStageHandler<GLRenderer>::Load() {
    if (!linked) {
	for (auto& i : shaderHandler) {
	    i->Load();
	    GLDEBUGCALL(glAttachShader(
		program, dynamic_cast<NativeShaderHandler<GLRenderer>*>(i.get())
			     ->shader));
	}
	GLDEBUGCALL(glLinkProgram(program));
	int32_t programLinked;
	GLDEBUGCALL(glGetProgramiv(program, GL_LINK_STATUS, &programLinked));
	if (programLinked != GL_TRUE) {
	    size_t debugMessageSize = 2048;
	    char message[debugMessageSize];
	    int32_t logLength;
	    GLDEBUGCALL(glGetProgramInfoLog(program, debugMessageSize,
					    &logLength, message));
	    throw CException(__LINE__, __FILE__, "GL Program Link Error",
			     message);
	}
	shaderHandler.clear();
	linked = true;
    }
    GLDEBUGCALL(glValidateProgram(program));
    GLDEBUGCALL(glUseProgram(program));
    renderer->shaderProgram = program;
}

void NativeShaderStageHandler<GLRenderer>::UnLoad() { glUseProgram(0); }

NativeShaderStageHandler<GLRenderer>::~NativeShaderStageHandler() {
    UnLoad();
    glDeleteProgram(program);
}

GLRenderer::GLRenderer() {
    LoadGladGL();
    glGenVertexArrays(1, &pvao);
    glBindVertexArray(pvao);
    glCullFace(GL_FRONT);
    glDepthMask(GL_TRUE);
    glActiveTexture(GL_TEXTURE0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

bool GLRenderer::gladLoaded = false;

GLenum GLRenderer::GetUniformLocation(const uint32_t& shaderProgram,
				      std::string& uniformName) {
    GLenum location;
    GLDEBUGCALL(location =
		    glGetUniformLocation(shaderProgram, uniformName.c_str()));
    return location;
}

void GLRenderer::Uniform1f(const uint32_t count, const float* data,
			   std::string name) {
    if (count == 1)
	glUniform1f(GetUniformLocation(shaderProgram, name), *data);
    else {
	glUniform1fv(GetUniformLocation(shaderProgram, name), count, data);
    }
}

void GLRenderer::Uniform1u(const uint32_t count, const uint32_t* data,
			   std::string name) {
    if (count == 1)
	glUniform1ui(GetUniformLocation(shaderProgram, name), *data);
    else {
	glUniform1uiv(GetUniformLocation(shaderProgram, name), count, data);
    }
}

void GLRenderer::Uniform1i(const uint32_t count, const int32_t* data,
			   std::string name) {
    if (count == 1)
	glUniform1i(GetUniformLocation(shaderProgram, name), *data);
    else {
	glUniform1iv(GetUniformLocation(shaderProgram, name), count, data);
    }
}

void GLRenderer::Uniform2f(const uint32_t count, const Vect2* data,
			   std::string name) {
    if (count == 1)
	glUniform2f(GetUniformLocation(shaderProgram, name), data->x, data->y);
    else {
	glUniform2fv(GetUniformLocation(shaderProgram, name), count,
		     (float*)data);
    }
}

void GLRenderer::Uniform3f(const uint32_t count, const Vect3* data,
			   std::string name) {
    if (count == 1)
	glUniform3f(GetUniformLocation(shaderProgram, name), data->x, data->y,
		    data->z);
    else {
	glUniform3fv(GetUniformLocation(shaderProgram, name), count,
		     (float*)(data));
    }
}

void GLRenderer::Uniform4f(const uint32_t count, const Vect4* data,
			   std::string name) {
    auto location = GetUniformLocation(shaderProgram, name);
    if (location == -1)
	throw std::runtime_error("uniform location not found\nvariable name:" +
				 name);
    if (count == 1)
	glUniform4f(location, data->x, data->y, data->z, data->w);
    else {
	glUniform4fv(GetUniformLocation(shaderProgram, name), count,
		     (float*)data);
    }
}

void GLRenderer::UniformMat(const uint32_t count, const Mat* mat,
			    std::string name) {
    GLDEBUGCALL(uint32_t uLocation = GetUniformLocation(shaderProgram, name));
    float* answer = new float[mat->sizet * count];
    for (uint32_t i = 0; i < count; i++) {
	std::copy(mat[i].buffer.get(), mat[i].buffer.get() + mat[i].sizet,
		  answer + (i * mat->sizet * sizeof(float)));
    }
    switch (mat->dimension.column) {
	case 2:
	    switch (mat->dimension.row) {
		case 2:
		    glUniformMatrix2fv(uLocation, count, GL_TRUE, answer);
		    break;
		case 3:
		    glUniformMatrix2x3fv(uLocation, count, GL_TRUE, answer);
		    break;
		case 4:
		    glUniformMatrix2x4fv(uLocation, count, GL_TRUE, answer);
		    break;
	    }
	    break;
	case 3:
	    switch (mat->dimension.row) {
		case 2:
		    glUniformMatrix3x2fv(uLocation, count, GL_TRUE, answer);
		    break;
		case 3:
		    glUniformMatrix3fv(uLocation, count, GL_TRUE, answer);
		    break;
		case 4:
		    glUniformMatrix3x4fv(uLocation, count, GL_TRUE, answer);
		    break;
	    }
	    break;
	case 4:
	    switch (mat->dimension.row) {
		case 2:
		    glUniformMatrix4x2fv(uLocation, count, GL_TRUE, answer);
		    break;
		case 3:
		    glUniformMatrix4x3fv(uLocation, count, GL_TRUE, answer);
		    break;
		case 4:
		    GLDEBUGCALL(
			glUniformMatrix4fv(uLocation, count, GL_FALSE, answer));
		    break;
	    }
	    break;
    }
}

void GLRenderer::WireFrameMode(bool swit) {
    if (swit)
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

GLenum GLRenderer::GetDrawTarget(DrawPrimitive drawPrimitive) {
    GLenum target;
    switch (drawPrimitive) {
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
	case DrawPrimitive::QUADS:
	    target = GL_QUADS;
	    break;
	case DrawPrimitive::LINES:
	    target = GL_LINES;
	    break;
	default:
	    throw CException(
		__LINE__, __FILE__, "Primitive Not found",
		"Primitive is not supported by this renderer \nPrimitive no: " +
		    std::to_string(static_cast<uint32_t>(drawPrimitive)));
    };
    return target;
}

void GLRenderer::ClearColor(float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
}

void GLRenderer::ClearDepth(float depthLevel) { glClearDepth(depthLevel); }

void GLRenderer::FinalizeVertexSpecification() {}

void GLRenderer::LoadBuffer(GBuffer* gBuffer) {
    glBindVertexArray(pvao);
    GLenum flags;
    GLenum bufferType;
    uint32_t rendererID;
    switch (gBuffer->bufferStyle.cpuFlags) {
	case GBuffer::GBufferStyle::CPUFlags::STATIC:
	    switch (gBuffer->bufferStyle.usage) {
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
	    switch (gBuffer->bufferStyle.usage) {
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
	    switch (gBuffer->bufferStyle.usage) {
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
    switch (gBuffer->bufferStyle.type) {
	case GBuffer::GBufferStyle::BufferType::VERTEX:
	    bufferType = GL_ARRAY_BUFFER;
	    binders.push_back(
		std::unique_ptr<GBinder>(new GLVertexBinder(pvao, rendererID)));
	    break;
	case GBuffer::GBufferStyle::BufferType::INDEX:
	    bufferType = GL_ELEMENT_ARRAY_BUFFER;
	    binders.push_back(
		std::unique_ptr<GBinder>(new GLIndexBinder(pvao, rendererID)));
	    break;
    };
    gBuffer->bindNo = binders.size() - 1;

    GLDEBUGCALL(this->Bind(*gBuffer));
    std::cout << gBuffer->sizet << std::endl;
    GLDEBUGCALL(glBufferData(bufferType, gBuffer->sizet,
			     resourceBank->resources[gBuffer->data].Get(),
			     flags));
}

NativeShaderHandlerParent* GLRenderer::CreateShader(ShaderType type) {
    return new NativeShaderHandler<GLRenderer>(type);
}

void GLRenderer::LoadTexture(Texture* texture, GBuffer* gBuffer) {
    uint32_t rendererID = 0;
    // gBuffer = new GBuffer;
    GLDEBUGCALL(glGenTextures(1, &rendererID));
    // GLenum type;
    // switch(texture->type)
    //{
    //     case Texture::Type::T1D:
    //         type = GL_TEXTURE_1D;
    //         glBindTexture(type, rendererID);
    //         glTexImage1D(type, 0, GL_RGBA, texture->width,
    //                 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data);
    //         break;
    //     case Texture::Type::T2D:
    //         type = GL_TEXTURE_2D;
    //                     break;
    //     case Texture::Type::T3D:
    //         type = GL_TEXTURE_3D;
    //         glBindTexture(type, rendererID);
    //         glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texture->width,
    //         texture->height, texture->depth,
    //                 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data);
    //         break;
    // };
    //
    GLenum format;
    uint32_t channel = 0;
    switch (texture->format) {
	case Texture::Format::RGBA:
	    format = GL_RGBA;
	    break;
	case Texture::Format::R:
	    format = GL_RED;
	    break;
	case Texture::Format::RGB:
	    format = GL_RGB;
	    break;
    };
    glBindTexture(GL_TEXTURE_2D, rendererID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLDEBUGCALL(glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width,
			     texture->height, 0, format, GL_UNSIGNED_BYTE,
			     resourceBank->resources[texture->data].Get()));

    binders.emplace_back(new GLTextureBinder(pvao, rendererID, GL_TEXTURE_2D));

    gBuffer->bindNo = binders.size() - 1;
    Bind(*gBuffer);
    GLDEBUGCALL(glGenerateMipmap(GL_TEXTURE_2D));
}

ShaderStageHandler* GLRenderer::CreateShaderStage() {
    auto tmp = new NativeShaderStageHandler<GLRenderer>;
    tmp->renderer = this;
    return tmp;
}

GLenum GLRenderer::GetOption(Options option) {
    switch (option) {
	case Options::BLEND:
	    return GL_BLEND;
	case Options::DEPTH_TEST:
	    return GL_DEPTH_TEST;
	case Options::FACE_CULL:
	    return GL_CULL_FACE;
	case Options::WIREFRAME_MODE:
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
	    return 0;
    };
}

void GLRenderer::Enable(Options option) { glEnable(GetOption(option)); }

void GLRenderer::Disable(Options option) { glDisable(GetOption(option)); }

void GLRenderer::UseShaderStage(ShaderStageHandler* shaderStageHandler) {
    shaderStageHandler->Load();
}

void GLRenderer::Draw(DrawPrimitive drawPrimitive, GBuffer* gBuffer) {
    if (gBuffer != nullptr) this->Bind(*gBuffer);
    glBindVertexArray(pvao);
    GLDEBUGCALL(glDrawElements(GetDrawTarget(drawPrimitive), gBuffer->count,
			       GL_UNSIGNED_INT, (const void*)0));
}

void GLRenderer::DrawInstanced(DrawPrimitive drawPrimitive, GBuffer* gBuffer,
			       uint32_t numInstanced) {
    if (gBuffer != nullptr) this->Bind(*gBuffer);
    glBindVertexArray(pvao);
    glDrawElementsInstanced(GetDrawTarget(drawPrimitive), gBuffer->count,
			    GL_UNSIGNED_INT, (const void*)0, numInstanced);
}

void GLRenderer::DrawInstancedArrays(DrawPrimitive drawPrimitive,
				     GBuffer* gBuffer, uint32_t numElements,
				     uint32_t numInstanced) {
    if (gBuffer != nullptr) Bind(*gBuffer);
    glBindVertexArray(pvao);
    glDrawArraysInstanced(GetDrawTarget(drawPrimitive), 0, numElements,
			  numInstanced);
}

void GLRenderer::DrawArrays(DrawPrimitive drawPrimitive, GBuffer* gBuffer,
			    uint32_t numElements) {
    glBindVertexArray(pvao);
    if (gBuffer != nullptr) Bind(*gBuffer);
    glDrawArrays(GetDrawTarget(drawPrimitive), 0, numElements);
}

void GLRenderer::DrawBuffer(DrawPrimitive drawPrimitive, GBuffer* gBuffer) {
    this->Bind(*gBuffer);
    glBindVertexArray(pvao);
    GLDEBUGCALL(glDrawBuffer(GetDrawTarget(drawPrimitive)));
}

void GLRenderer::Clear() {
    GLDEBUGCALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void GLRenderer::SetLayout(const uint32_t layout) {
    glBindVertexArray(vaos[layout].first);
    auto& tmp = vaos[layout].second;
    uint32_t offset = 0;
    for (uint32_t i = 0; i < tmp.size() - 1; i++) {
	GLDEBUGCALL(glVertexAttribPointer(i, tmp[i], GL_FLOAT, GL_FALSE,
					  tmp.back(), (const void*)offset));
	std::cout << tmp[i] << " " << offset << std::endl;
	GLDEBUGCALL(glEnableVertexAttribArray(i));
	offset += tmp[i] * sizeof(float);
    }
}

uint32_t GLRenderer::AddSpecification(
    VertexSpecification& vertexSpecification) {
    glBindVertexArray(pvao);
    vaos.emplace_back();
    vaos.back().first = pvao;
    uint32_t totalSize = 0;
    for (uint32_t i = 0; i < vertexSpecification.size(); i++) {
	totalSize += vertexSpecification[i].second;
	GLDEBUGCALL(glBindAttribLocation(shaderProgram, i,
					 vertexSpecification[i].first.c_str()));
	vaos.back().second.push_back(vertexSpecification[i].second);
    }
    vaos.back().second.push_back(totalSize * sizeof(float));
    return vaos.size() - 1;
}

void GLRenderer::LoadGladGL() {
    if (!gladLoaded) {
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
	    if (!gladLoadGL()) {
		throw CException(__LINE__, __FILE__, "glad Load error",
				 "Can't load glad");
	    }
	}
	gladLoaded = true;
    }
}
