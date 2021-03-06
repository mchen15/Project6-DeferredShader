#include "main.h"

#include "Utility.h"

#include "SOIL.h"
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_projection.hpp>
#include <glm/gtc/matrix_operation.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/verbose_operator.hpp>

#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;
using namespace glm;

const float PI = 3.14159f;

int width, height;

device_mesh_t uploadMesh(const mesh_t & mesh) {
    device_mesh_t out;
    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex 
    //attributes and the buffers they are bound to
    //Different vertex array per mesh.
    glGenVertexArrays(1, &(out.vertex_array));
    glBindVertexArray(out.vertex_array);

    //Allocate vbos for data
    glGenBuffers(1, &(out.vbo_vertices));
    glGenBuffers(1, &(out.vbo_normals));
    glGenBuffers(1, &(out.vbo_indices));
    glGenBuffers(1, &(out.vbo_texcoords));

    //Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size()*sizeof(vec3), 
            &mesh.vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::POSITION);
    //cout << mesh.vertices.size() << " verts:" << endl;
    //for(int i = 0; i < mesh.vertices.size(); ++i)
    //    cout << "    " << mesh.vertices[i][0] << ", " << mesh.vertices[i][1] << ", " << mesh.vertices[i][2] << endl;

    //Upload normal data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, mesh.normals.size()*sizeof(vec3), 
            &mesh.normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::NORMAL, 3, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::NORMAL);
    //cout << mesh.normals.size() << " norms:" << endl;
    //for(int i = 0; i < mesh.normals.size(); ++i)
    //    cout << "    " << mesh.normals[i][0] << ", " << mesh.normals[i][1] << ", " << mesh.normals[i][2] << endl;

    //Upload texture coord data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_texcoords);
    glBufferData(GL_ARRAY_BUFFER, mesh.texcoords.size()*sizeof(vec2), 
            &mesh.texcoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::TEXCOORD);
    //cout << mesh.texcoords.size() << " texcos:" << endl;
    //for(int i = 0; i < mesh.texcoords.size(); ++i)
    //    cout << "    " << mesh.texcoords[i][0] << ", " << mesh.texcoords[i][1] << endl;

    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size()*sizeof(GLushort), 
            &mesh.indices[0], GL_STATIC_DRAW);
    out.num_indices = mesh.indices.size();
    //Unplug Vertex Array
    glBindVertexArray(0);

    out.texname = mesh.texname;
    out.color = mesh.color;
    return out;
}


int num_boxes = 3;
const int DUMP_SIZE = 1024;

vector<device_mesh_t> draw_meshes;
void initMesh() {
    for(vector<tinyobj::shape_t>::iterator it = shapes.begin();
            it != shapes.end(); ++it)
    {
        tinyobj::shape_t shape = *it;
        int totalsize = shape.mesh.indices.size() / 3;
        int f = 0;
        while(f<totalsize){
            mesh_t mesh;
            int process = std::min(10000, totalsize-f);
            int point = 0;
            for(int i=f; i<process+f; i++){
                int idx0 = shape.mesh.indices[3*i];
                int idx1 = shape.mesh.indices[3*i+1];
                int idx2 = shape.mesh.indices[3*i+2];
                vec3 p0 = vec3(shape.mesh.positions[3*idx0],
                               shape.mesh.positions[3*idx0+1],
                               shape.mesh.positions[3*idx0+2]);
                vec3 p1 = vec3(shape.mesh.positions[3*idx1],
                               shape.mesh.positions[3*idx1+1],
                               shape.mesh.positions[3*idx1+2]);
                vec3 p2 = vec3(shape.mesh.positions[3*idx2],
                               shape.mesh.positions[3*idx2+1],
                               shape.mesh.positions[3*idx2+2]);

                mesh.vertices.push_back(p0);
                mesh.vertices.push_back(p1);
                mesh.vertices.push_back(p2);

                if(shape.mesh.normals.size() > 0)
                {
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx0],
                                                shape.mesh.normals[3*idx0+1],
                                                shape.mesh.normals[3*idx0+2]));
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx1],
                                                shape.mesh.normals[3*idx1+1],
                                                shape.mesh.normals[3*idx1+2]));
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx2],
                                                shape.mesh.normals[3*idx2+1],
                                                shape.mesh.normals[3*idx2+2]));
                }
                else
                {
                    vec3 norm = normalize(glm::cross(normalize(p1-p0), normalize(p2-p0)));
                    mesh.normals.push_back(norm);
                    mesh.normals.push_back(norm);
                    mesh.normals.push_back(norm);
                }

                if(shape.mesh.texcoords.size() > 0)
                {
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx0],
                                                  shape.mesh.positions[2*idx0+1]));
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx1],
                                                  shape.mesh.positions[2*idx1+1]));
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx2],
                                                  shape.mesh.positions[2*idx2+1]));
                }
                else
                {
                    vec2 tex(0.0);
                    mesh.texcoords.push_back(tex);
                    mesh.texcoords.push_back(tex);
                    mesh.texcoords.push_back(tex);
                }
                mesh.indices.push_back(point++);
                mesh.indices.push_back(point++);
                mesh.indices.push_back(point++);
            }

            mesh.color = vec4(shape.material.diffuse[0],
                              shape.material.diffuse[1],
                              shape.material.diffuse[2],
							  shape.material.shininess);
            mesh.texname = shape.material.diffuse_texname;
            draw_meshes.push_back(uploadMesh(mesh));
            f=f+process;
        }
    }
}


device_mesh2_t device_quad;
void initQuad() {
    vertex2_t verts [] = { {vec3(-1,1,0),vec2(0,1)},
        {vec3(-1,-1,0),vec2(0,0)},
        {vec3(1,-1,0),vec2(1,0)},
        {vec3(1,1,0),vec2(1,1)}};

    unsigned short indices[] = { 0,1,2,0,2,3};

    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex attributes and the buffers they are bound too
    //Different vertex array per mesh.
    glGenVertexArrays(1, &(device_quad.vertex_array));
    glBindVertexArray(device_quad.vertex_array);

    //Allocate vbos for data
    glGenBuffers(1,&(device_quad.vbo_data));
    glGenBuffers(1,&(device_quad.vbo_indices));

    //Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, device_quad.vbo_data);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    //Use of strided data, Array of Structures instead of Structures of Arrays
    glVertexAttribPointer(quad_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),0);
    glVertexAttribPointer(quad_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),(void*)sizeof(vec3));
    glEnableVertexAttribArray(quad_attributes::POSITION);
    glEnableVertexAttribArray(quad_attributes::TEXCOORD);

    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLushort), indices, GL_STATIC_DRAW);
    device_quad.num_indices = 6;
    //Unplug Vertex Array
    glBindVertexArray(0);
}


vector<Light> lights;
// initialize all point light sources
void initLights()
{
	float minI = 0.3;
	float maxI = 5.3;
	float minJ = 0.3;
	float maxJ = 5.3;
	float minK = 0.3;
	float maxK = 5.3;
	float step = 1;
	float lightStr = 0.8;

	float botBaseRate = 300;
	float topBaseRate = 300;
	float leftBaseRate = 300;
	float rightBaseRate = 300;
	float backBaseRate = -300;

	float botOffset = 1;
	float topOffset = 1;
	float rightOffset = 1;
	float leftOffset = 1;
	float backOffset = 1;

	for (float j = minJ; j <= maxJ; j += step)
	{
		for (float i = minI; i <= maxI; i += step)
		{
		
			Light L1(vec3(i, -j, minK), lightStr, botBaseRate, botOffset); // bottom plane
			Light L2(vec3(i, -j, maxK), lightStr, topBaseRate, topOffset); // top plane
			lights.push_back(L1);
			lights.push_back(L2);

			
		}
		
		botOffset += 0.5;
		topOffset += 0.5;
	}

	for (float i = minI; i <= maxI; i += step)
	{
		for (float k = minK+1; k <= maxK-1; k += step) // skip area that are already covered by the top / bottom plane of lights
		{
			Light L1(vec3(i, -maxJ, k), lightStr, backBaseRate); // back plane
			lights.push_back(L1);
		}
	}

	for (float j = minJ; j <= maxJ-1; j += step)
	{
		for (float k = minK+1; k <= maxK-1; k += step)
		{
			Light L1(vec3(minI, -j, k), lightStr, leftBaseRate, leftOffset); // left plane
			Light L2(vec3(maxI, -j, k), lightStr, rightBaseRate, rightOffset); // right plane
			lights.push_back(L1);
			lights.push_back(L2);
		}

		rightOffset += 0.5;
		leftOffset += 0.5;
	}

	vec3 centerLight1Pos = vec3((minI + maxI) * 0.5, (minJ + maxJ) * -0.5, (minK + maxK) * 0.5);
	
	
	float centerLight1Str = 2.5f;
	float centerLightRate = 500;
	float centerLightOffset = 1;

	Light centerLight(centerLight1Pos, centerLight1Str, centerLightRate);
	lights.push_back(centerLight);
}

GLuint depthTexture = 0;
GLuint normalTexture = 0;
GLuint positionTexture = 0;
GLuint colorTexture = 0;
GLuint specTexture = 0;
GLuint bloomMapTexture = 0;
GLuint bloomMapPass1Texture = 0;
GLuint postTexture = 0;
GLuint FBO[3] = {0, 0, 0};

GLuint pass_prog;
GLuint point_prog;
GLuint ambient_prog;
GLuint diagnostic_prog;
GLuint post_prog;
GLuint bloompass1_prog;

void initShader() {
#ifdef WIN32
	const char * pass_vert = "../../../res/shaders/pass.vert";
	const char * shade_vert = "../../../res/shaders/shade.vert";
	const char * post_vert = "../../../res/shaders/post.vert";
	const char * bloomPass1_vert = "../../../res/shaders/bloomPass1.vert";

	const char * pass_frag = "../../../res/shaders/pass.frag";
	const char * diagnostic_frag = "../../../res/shaders/diagnostic.frag";
	const char * ambient_frag = "../../../res/shaders/ambient.frag";
	const char * point_frag = "../../../res/shaders/point.frag";
	const char * post_frag = "../../../res/shaders/post.frag";
	const char * bloomPass1_frag = "../../../res/shaders/bloomPass1.frag";

#else
	const char * pass_vert = "../res/shaders/pass.vert";
	const char * shade_vert = "../res/shaders/shade.vert";
	const char * post_vert = "../res/shaders/post.vert";

	const char * pass_frag = "../res/shaders/pass.frag";
	const char * diagnostic_frag = "../res/shaders/diagnostic.frag";
	const char * ambient_frag = "../res/shaders/ambient.frag";
	const char * point_frag = "../res/shaders/point.frag";
	const char * post_frag = "../res/shaders/post.frag";
#endif

	/////////////////////////////
	// pass_prog

	Utility::shaders_t shaders = Utility::loadShaders(pass_vert, pass_frag);

    pass_prog = glCreateProgram();

    glBindAttribLocation(pass_prog, mesh_attributes::POSITION, "Position");
    glBindAttribLocation(pass_prog, mesh_attributes::NORMAL, "Normal");
    //glBindAttribLocation(pass_prog, mesh_attributes::TEXCOORD, "Texcoord");

    Utility::attachAndLinkProgram(pass_prog,shaders);

	/////////////////////////////
	// diagnostic_prog
	shaders = Utility::loadShaders(shade_vert, diagnostic_frag);

    diagnostic_prog = glCreateProgram();

    glBindAttribLocation(diagnostic_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(diagnostic_prog, quad_attributes::TEXCOORD, "Texcoord");
	glBindFragDataLocation(diagnostic_prog, 0, "out_Color");		//LOOK: NEED TO DO THIS TO MAKE SURE THAT THE OUTPUTS ARE THE SAME INDEX
	glBindFragDataLocation(diagnostic_prog, 1, "out_Spec");			//In initFBO, the portion where I am binding the output textures for these
	glBindFragDataLocation(diagnostic_prog, 2, "out_BloomMap");		//shaders that need to write to the same textures, they need to have the same index
																	//for the output variable.
																	//out_BloomMap will hold the information of whether or not each pixel will be applied the bloom effect.
																	//this map is later taken in as input by bloomPass and post as u_BloomMapTex. To change which object
																	//should be applied the bloom effect, adjust alpha channel of the object material in the .mtl files that came w/ the obj file.
    Utility::attachAndLinkProgram(diagnostic_prog, shaders);

	/////////////////////////////
	// ambient_prog
	shaders = Utility::loadShaders(shade_vert, ambient_frag);

    ambient_prog = glCreateProgram();

    glBindAttribLocation(ambient_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(ambient_prog, quad_attributes::TEXCOORD, "Texcoord");
	glBindFragDataLocation(ambient_prog, 0, "out_Color"); //LOOK: NEED TO DO THIS TO MAKE SURE THAT THE OUTPUTS ARE THE SAME INDEX
	glBindFragDataLocation(ambient_prog, 1, "out_Spec");
	glBindFragDataLocation(ambient_prog, 2, "out_BloomMap");

    Utility::attachAndLinkProgram(ambient_prog, shaders);

	/////////////////////////////
	// point_prog
	shaders = Utility::loadShaders(shade_vert, point_frag);

    point_prog = glCreateProgram();

    glBindAttribLocation(point_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(point_prog, quad_attributes::TEXCOORD, "Texcoord");
	glBindFragDataLocation(point_prog, 0, "out_Color"); //LOOK: NEED TO DO THIS TO MAKE SURE THAT THE OUTPUTS ARE THE SAME INDEX
	glBindFragDataLocation(point_prog, 1, "out_Spec");
	glBindFragDataLocation(point_prog, 2, "out_BloomMap");
    Utility::attachAndLinkProgram(point_prog, shaders);

	/////////////////////////////
	// post_prog
	shaders = Utility::loadShaders(post_vert, post_frag);

    post_prog = glCreateProgram();

    glBindAttribLocation(post_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(post_prog, quad_attributes::TEXCOORD, "Texcoord");

    Utility::attachAndLinkProgram(post_prog, shaders);
	
	/////////////////////////////
	// BloomPass1_prog
#if IS_TWO_PASS_BLOOM == 1
	shaders = Utility::loadShaders(bloomPass1_vert, bloomPass1_frag);

	bloompass1_prog = glCreateProgram();
	glBindAttribLocation(bloompass1_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(bloompass1_prog, quad_attributes::TEXCOORD, "Texcoord");

	glBindFragDataLocation(bloompass1_prog, 0, "out_Color");

	Utility::attachAndLinkProgram(bloompass1_prog, shaders);
#endif
}

void freeFBO() {
    glDeleteTextures(1,&depthTexture);
    glDeleteTextures(1,&normalTexture);
    glDeleteTextures(1,&positionTexture);
    glDeleteTextures(1,&colorTexture);
    glDeleteTextures(1,&postTexture);
	glDeleteTextures(1,&bloomMapTexture);
	glDeleteTextures(1,&specTexture);

#if IS_TWO_PASS_BLOOM == 1
	glDeleteTextures(1,&bloomMapPass1Texture);
	glDeleteFramebuffers(1,&FBO[2]);
#endif

    glDeleteFramebuffers(1,&FBO[0]);
    glDeleteFramebuffers(1,&FBO[1]);

}

void checkFramebufferStatus(GLenum framebufferStatus) {
    switch (framebufferStatus) {
        case GL_FRAMEBUFFER_COMPLETE_EXT: break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
                                          printf("Attachment Point Unconnected\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
                                          printf("Missing Attachment\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                                          printf("Dimensions do not match\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
                                          printf("Formats\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
                                          printf("Draw Buffer\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
                                          printf("Read Buffer\n");
                                          break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
                                          printf("Unsupported Framebuffer Configuration\n");
                                          break;
        default:
                                          printf("Unkown Framebuffer Object Failure\n");
                                          break;
    }
}


GLuint random_normal_tex;
GLuint random_scalar_tex;
void initNoise() {  
#ifdef WIN32
	const char * rand_norm_png = "../../../res/random_normal.png";
	const char * rand_png = "../../../res/random.png";
#else
	const char * rand_norm_png = "../res/random_normal.png";
	const char * rand_png = "../res/random.png";
#endif
	random_normal_tex = (unsigned int)SOIL_load_OGL_texture(rand_norm_png,0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

	random_scalar_tex = (unsigned int)SOIL_load_OGL_texture(rand_png,0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void initFBO(int w, int h) {
    GLenum FBOstatus;

	///////////////////////////////////////////////////////////////////
	// setting up framebuffer and textures for pass_prog to output to
    glActiveTexture(GL_TEXTURE9);

    glGenTextures(1, &depthTexture);
    glGenTextures(1, &normalTexture);
    glGenTextures(1, &positionTexture);
    glGenTextures(1, &colorTexture);

    //Set up depth FBO
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0); // http://www.opengl.org/sdk/docs/man/xhtml/glTexImage2D.xml

    //Set up normal FBO
    glBindTexture(GL_TEXTURE_2D, normalTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    //Set up position FBO
    glBindTexture(GL_TEXTURE_2D, positionTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    //Set up color FBO
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F , w, h, 0, GL_RGBA, GL_FLOAT,0); // Use GL_RGBA32F so I can pack things in the alpha ch.

    // create a framebuffer object
    glGenFramebuffers(1, &FBO[0]); // http://www.opengl.org/sdk/docs/man/xhtml/glGenFramebuffers.xml
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]); 

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_NONE);
    GLint normal_loc = glGetFragDataLocation(pass_prog,"out_Normal");
    GLint position_loc = glGetFragDataLocation(pass_prog,"out_Position");
    GLint color_loc = glGetFragDataLocation(pass_prog,"out_Color");
	
    GLenum draws [3];
    draws[normal_loc] = GL_COLOR_ATTACHMENT0;
    draws[position_loc] = GL_COLOR_ATTACHMENT1;
    draws[color_loc] = GL_COLOR_ATTACHMENT2;
    glDrawBuffers(3, draws); //This call will define an array of buffers into which outputs from the fragment shader data will be written
							 //http://www.opengl.org/sdk/docs/man/xhtml/glDrawBuffers.xml

    // attach the texture to FBO depth attachment point
	// Buffers (specific locations in the framebuffer) in FBOs are also called "attachment points"; they're the locations where images can be attached
    int test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);		// Look, GL_DEPTH_ATTACHMENT tells openGL to render depth information to depthTexture
    glBindTexture(GL_TEXTURE_2D, normalTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[normal_loc], normalTexture, 0);		// Note that draw[i] is already defined by glDrawBuffers as the locations where the outputs
    glBindTexture(GL_TEXTURE_2D, positionTexture);									// from the fragment shaders data will be written. If we bind these textures to the framebuffer,
    glFramebufferTexture(GL_FRAMEBUFFER, draws[position_loc], positionTexture, 0);  // we are essentially writing the output of the fragment shaders to these textures! Very cool!
    glBindTexture(GL_TEXTURE_2D, colorTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[color_loc], colorTexture, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[0]\n");
        checkFramebufferStatus(FBOstatus);
    }


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// setting up framebuffer and textures for ambient_prog, point_prog, and diagnostic_prog to output to
    // Post Processing buffer! These are input textures to post.frag. Values are filled by the previous stage in the pipeline
    glActiveTexture(GL_TEXTURE9);

    glGenTextures(1, &postTexture);
	glGenTextures(1, &specTexture);
	glGenTextures(1, &bloomMapTexture);

    //Set up post FBO
    glBindTexture(GL_TEXTURE_2D, postTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	//Set up spec FBO
	glBindTexture(GL_TEXTURE_2D, specTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	//Set up bloomMap FBO
	glBindTexture(GL_TEXTURE_2D, bloomMapTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    // create a framebuffer object and bind it to contex
    glGenFramebuffers(1, &FBO[1]); 
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
	// LOOK: For FBO[1], the output texture is being set at ambient_prog's out_Color
    //glReadBuffer(GL_BACK);
	glReadBuffer(GL_NONE);
    color_loc = glGetFragDataLocation(ambient_prog,"out_Color");			// LOOK: This works because in initShaders(), I am explicitly assigning the out_Color,
	GLint spec_loc = glGetFragDataLocation(ambient_prog, "out_Spec");		// out_Spec, and out_BloomMap to have the same index.
	GLint bloom_loc = glGetFragDataLocation(ambient_prog, "out_BloomMap");
    GLenum draw[3];
    draw[color_loc] = GL_COLOR_ATTACHMENT0;
	draw[spec_loc] = GL_COLOR_ATTACHMENT1;
	draw[bloom_loc] = GL_COLOR_ATTACHMENT2;
    glDrawBuffers(3, draw);
	
    // attach the texture to FBO colfor attachment point
    test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, postTexture);
    glFramebufferTexture(GL_FRAMEBUFFER, draw[color_loc], postTexture, 0);
	glBindTexture(GL_TEXTURE_2D, specTexture);
	glFramebufferTexture(GL_FRAMEBUFFER, draw[spec_loc], specTexture, 0);
	glBindTexture(GL_TEXTURE_2D, bloomMapTexture);
	glFramebufferTexture(GL_FRAMEBUFFER, draw[bloom_loc], bloomMapTexture, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[1]\n");
        checkFramebufferStatus(FBOstatus);
    }

#if IS_TWO_PASS_BLOOM == 1
	////////////////////////////////////////////////////////////////////////
	// setting up framebuffer and textures for bloompass1_prog to output to
	glActiveTexture(GL_TEXTURE9);
	glGenTextures(1, &bloomMapPass1Texture);
	glBindTexture(GL_TEXTURE_2D, bloomMapPass1Texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	glGenFramebuffers(1, &FBO[2]); 
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[2]);
	glReadBuffer(GL_NONE);
	color_loc = glGetFragDataLocation(bloompass1_prog, "out_Color");
	GLenum drawBuff[1];
	drawBuff[color_loc] = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, drawBuff);

	glBindTexture(GL_TEXTURE_2D, bloomMapPass1Texture);
	glFramebufferTexture(GL_FRAMEBUFFER, drawBuff[color_loc], bloomMapPass1Texture, 0);

	// check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[2]\n");
        checkFramebufferStatus(FBOstatus);
    }
#endif

    // switch back to window-system-provided framebuffer (i.e the default buffer provided by the OpenGL context)
    glClear(GL_DEPTH_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Given buf (index of the FBO array that we have created), this function will bind FBO[buf] to the current OpenGL contex
void bindFBO(int buf) {
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0); //Bad mojo to unbind the framebuffer using the texture
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[buf]);
    glClear(GL_DEPTH_BUFFER_BIT);
    //glColorMask(false,false,false,false);
    glEnable(GL_DEPTH_TEST);
}

void setTextures() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0); 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glColorMask(true,true,true,true);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
}



Camera cam(vec3(2.8, 6.7, 2.8),
        normalize(vec3(0,-1,0)),
        normalize(vec3(0,0,1)));

    void
Camera::adjust(float dx, // look left right
        float dy, //look up down
        float dz,
        float tx, //strafe left right
        float ty,
        float tz)//go forward) //strafe up down
{

    if (abs(dx) > 0) {
        rx += dx;
        rx = fmod(rx,360.0f);
    }

    if (abs(dy) > 0) {
        ry += dy;
        ry = clamp(ry,-70.0f, 70.0f);
    }

    if (abs(tx) > 0) {
        vec3 dir = glm::gtx::rotate_vector::rotate(start_dir,rx + 90,up);
        vec2 dir2(dir.x,dir.y);
        vec2 mag = dir2 * tx;
        pos += mag;	
    }

    if (abs(ty) > 0) {
        z += ty;
    }

    if (abs(tz) > 0) {
        vec3 dir = glm::gtx::rotate_vector::rotate(start_dir,rx,up);
        vec2 dir2(dir.x,dir.y);
        vec2 mag = dir2 * tz;
        pos += mag;
    }
}

mat4x4 Camera::get_view() {
    vec3 inclin = glm::gtx::rotate_vector::rotate(start_dir,ry,start_left);
    vec3 spun = glm::gtx::rotate_vector::rotate(inclin,rx,up);
    vec3 cent(pos, z);
    return lookAt(cent, cent + spun, up);
}

mat4x4 get_mesh_world() {
    vec3 tilt(1.0f,0.0f,0.0f);
    //mat4 translate_mat = glm::translate(glm::vec3(0.0f,.5f,0.0f));
    mat4 tilt_mat = glm::rotate(mat4(), 90.0f, tilt);
    mat4 scale_mat = glm::scale(mat4(), vec3(0.01));
    return tilt_mat * scale_mat; //translate_mat;
}


float FARP;
float NEARP;

// uses pass_prog (pass.vert and pass.frag) to compute position, normal, and color.
// The fragment shader will output those values to the corresponding textures.
void draw_mesh() {
    FARP = 100.0f;
    NEARP = 0.1f;

    glUseProgram(pass_prog);


    mat4 model = get_mesh_world();
    mat4 view = cam.get_view();
    mat4 persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    mat4 inverse_transposed = transpose(inverse(view*model));

    glUniform1f(glGetUniformLocation(pass_prog, "u_Far"), FARP);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_Model"),1,GL_FALSE,&model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_View"),1,GL_FALSE,&view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_Persp"),1,GL_FALSE,&persp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_InvTrans") ,1,GL_FALSE,&inverse_transposed[0][0]);

    for(int i=0; i<draw_meshes.size(); i++){
        glUniform4fv(glGetUniformLocation(pass_prog, "u_Color"), 1, &(draw_meshes[i].color[0]));
        glBindVertexArray(draw_meshes[i].vertex_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_meshes[i].vbo_indices);
        glDrawElements(GL_TRIANGLES, draw_meshes[i].num_indices, GL_UNSIGNED_SHORT,0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}


enum Display display_type = DISPLAY_TOTAL;

// set up quads with either point_prog (shade.vert, point.frag) & ambient_prog (shade.vert, ambient.frag), 
// or diagnostic_prog (vert.frag, diagnostic.frag)
void setup_quad(GLuint prog)
{
    glUseProgram(prog);
    glEnable(GL_TEXTURE_2D);

    mat4 persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    vec4 test(-2,0,10,1);
    vec4 testp = persp * test;
    vec4 testh = testp / testp.w;
    vec2 coords = vec2(testh.x, testh.y) / 2.0f + 0.5f;
    glUniform1i(glGetUniformLocation(prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(prog, "u_ScreenWidth"), width);
    glUniform1f(glGetUniformLocation(prog, "u_Far"), FARP);
    glUniform1f(glGetUniformLocation(prog, "u_Near"), NEARP);
    glUniform1i(glGetUniformLocation(prog, "u_DisplayType"), display_type);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_Persp"),1, GL_FALSE, &persp[0][0] );

	// passing the textures as input.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Depthtex"),0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Normaltex"),1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Positiontex"),2);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Colortex"),3);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glUniform1i(glGetUniformLocation(prog, "u_RandomNormaltex"),4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glUniform1i(glGetUniformLocation(prog, "u_RandomScalartex"),5);
}

void draw_quad() {

    glBindVertexArray(device_quad.vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);

    glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT,0);

    glBindVertexArray(0);
}

void draw_light(vec3 pos, float strength, mat4 sc, mat4 vp, float NEARP) {
    float radius = strength;
    vec4 light = cam.get_view() * vec4(pos, 1.0); 
    //if( light.z > NEARP)
    //{
    //    return;
    //}
    light.w = radius;
    glUniform4fv(glGetUniformLocation(point_prog, "u_Light"), 1, &(light[0]));
    glUniform1f(glGetUniformLocation(point_prog, "u_LightIl"), strength);

    vec4 left = vp * vec4(pos + radius*cam.start_left, 1.0);
    vec4 up = vp * vec4(pos + radius*cam.up, 1.0);
    vec4 center = vp * vec4(pos, 1.0);

    left = sc * left;
    up = sc * up;
    center = sc * center;

    left /= left.w;
    up /= up.w;
    center /= center.w;

    float hw = glm::distance(left, center);
    float hh = glm::distance(up, center);

    float r = (hh > hw) ? hh : hw;

    float x = center.x-r;
    float y = center.y-r;

    glScissor(x, y, 2*r, 2*r); // http://www.opengl.org/sdk/docs/man/xhtml/glScissor.xml
    draw_quad();
}

void updateDisplayText(char * disp) {
    switch(display_type) {
        case(DISPLAY_DEPTH):
            sprintf(disp, "Displaying Depth");
            break; 
        case(DISPLAY_NORMAL):
            sprintf(disp, "Displaying Normal");
            break; 
        case(DISPLAY_COLOR):
            sprintf(disp, "Displaying Color");
            break;
        case(DISPLAY_POSITION):
            sprintf(disp, "Displaying Position");
            break;
        case(DISPLAY_TOTAL):
            sprintf(disp, "Displaying Diffuse + Specular (TOTAL)");
            break;
        case(DISPLAY_LIGHTS):
            sprintf(disp, "Displaying Lights");
            break;
		case(DISPLAY_TOON):
			sprintf(disp, "Displaying Toon Shading");
			break;
		case(DISPLAY_BLOOM):
			sprintf(disp, "Displaying Bloom Effect");
			break;
		case(DISPLAY_AA):
			sprintf(disp, "Display Diffuse + AA");
			break;
		case(DISPLAY_SPECULAR):
			sprintf(disp, "Display Specular");
			break;
    }
}

int frame = 0;
int currenttime = 0;
int timebase = 0;
char title[1024];
char disp[1024];
char occl[1024];

void updateTitle() {
    updateDisplayText(disp);
    //calculate the frames per second
    frame++;

    //get the current time
    currenttime = glutGet(GLUT_ELAPSED_TIME);

    //check if a second has passed
    if (currenttime - timebase > 1000) 
    {
        sprintf(title, "CIS565 OpenGL Frame | %s FPS: %4.2f", disp, frame*1000.0/(currenttime-timebase));
        //sprintf(title, "CIS565 OpenGL Frame | %4.2f FPS", frame*1000.0/(currenttime-timebase));
        glutSetWindowTitle(title);
        timebase = currenttime;		
        frame = 0;
		bloomBound = 18;
    }
}

bool doIScissor = true;
void display(void)
{
    // Stage 1 -- RENDER TO G-BUFFER
    bindFBO(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_mesh();

    // Stage 2 -- RENDER TO P-BUFFER
    setTextures(); // unnecessary?
    bindFBO(1);
    glEnable(GL_BLEND); // enable alpha value calculations
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE); // This sums the outputs of the http://www.opengl.org/sdk/docs/man/xhtml/glBlendFunc.xml
    glClear(GL_COLOR_BUFFER_BIT);
    if(display_type == DISPLAY_LIGHTS || display_type == DISPLAY_TOTAL || display_type == DISPLAY_TOON || display_type == DISPLAY_BLOOM 
		|| display_type == DISPLAY_AA || display_type == DISPLAY_SPECULAR)
    {
        setup_quad(point_prog); // used to render the light source and compute light from point light
        if(doIScissor) glEnable(GL_SCISSOR_TEST);
        mat4 vp = perspective(45.0f,(float)width/(float)height,NEARP,FARP) * 
                  cam.get_view();
        mat4 sc = mat4(width, 0.0,    0.0, 0.0,
                       0.0,   height, 0.0, 0.0,
                       0.0,   0.0,    1.0, 0.0,
                       0.0,   0.0,    0.0, 1.0) *
                  mat4(0.5, 0.0, 0.0, 0.0,
                       0.0, 0.5, 0.0, 0.0,
                       0.0, 0.0, 1.0, 0.0,
                       0.5, 0.5, 0.0, 1.0);
#if MULTI_LIGHTS == 1
			
		for (int i = 0; i < lights.size(); ++i)
		{
			float str = 0;
			if (pauseLightAnim)
				str = lights[i].sampleStrength(currenttime);
			else
				str = lights[i].getStrength();

			
			//draw_light(lights[i].getPosition(), lights[i].getStrength(), sc, vp, NEARP);
			draw_light(lights[i].getPosition(), str, sc, vp, NEARP);
		}


#else
		draw_light(vec3(2.5, -2.5, 5.0), 0.50, sc, vp, NEARP);
#endif
        glDisable(GL_SCISSOR_TEST);
        vec4 dir_light(0.1, 1.0, 1.0, 0.0);
        dir_light = cam.get_view() * dir_light; 
        dir_light = normalize(dir_light);
        dir_light.w = 0.3;
        float strength = 0.09;
        setup_quad(ambient_prog); // render scene with directional light
        glUniform4fv(glGetUniformLocation(ambient_prog, "u_Light"), 1, &(dir_light[0]));
        glUniform1f(glGetUniformLocation(ambient_prog, "u_LightIl"), strength);
        draw_quad();
    }
    else
    {
        setup_quad(diagnostic_prog);
        draw_quad();
    }
    glDisable(GL_BLEND);

#if IS_TWO_PASS_BLOOM == 1
	//Stage 2.5 -- Perform first pass of bloom if enabled
	setTextures();
	bindFBO(2);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(bloompass1_prog);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bloomMapTexture);
    glUniform1i(glGetUniformLocation(bloompass1_prog, "u_BloomMapTex"),0);
	glUniform1i(glGetUniformLocation(bloompass1_prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(bloompass1_prog, "u_ScreenWidth"), width);
	glUniform1i(glGetUniformLocation(bloompass1_prog, "u_DisplayType"), display_type);
	glUniform1i(glGetUniformLocation(bloompass1_prog, "u_Bound"), bloomBound);
	draw_quad();

#endif
    
	//Stage 3 -- RENDER TO SCREEN
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
    setTextures();
    glUseProgram(post_prog);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, postTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Posttex"),0);

	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Normaltex"),1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, specTexture);
	glUniform1i(glGetUniformLocation(post_prog, "u_SpecTex"),2);
    
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, bloomMapTexture);
	glUniform1i(glGetUniformLocation(post_prog, "u_BloomMapTex"),3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomNormaltex"),4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomScalartex"),5);

    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenWidth"), width);
	glUniform1i(glGetUniformLocation(post_prog, "u_DisplayType"), display_type);

#if IS_TWO_PASS_BLOOM == 1
	glUniform1i(glGetUniformLocation(post_prog, "isTwoPassBloom"), 1);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, bloomMapPass1Texture);
	glUniform1i(glGetUniformLocation(post_prog, "u_BloomMapPass1Tex"),6);
	glUniform1i(glGetUniformLocation(post_prog, "u_Bound"), bloomBound);
#else
	glUniform1i(glGetUniformLocation(post_prog, "isTwoPassBloom"), 0);
#endif

    draw_quad();

	glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    updateTitle();
	updateBloomBound(30,6);
    glutPostRedisplay();
    glutSwapBuffers();
}



void reshape(int w, int h)
{
    width = w;
    height = h;
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    if (FBO[0] != 0 || depthTexture != 0 || normalTexture != 0 ) {
        freeFBO();
    }
    initFBO(w,h);
}


int mouse_buttons = 0;
int mouse_old_x = 0;
int mouse_old_y = 0;
void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        mouse_buttons |= 1<<button;
    } else if (state == GLUT_UP) {
        mouse_buttons = 0;
    }

    mouse_old_x = x;
    mouse_old_y = y;
}

void motion(int x, int y)
{
    float dx, dy;
    dx = (float)(x - mouse_old_x);
    dy = (float)(y - mouse_old_y);

    if (mouse_buttons & 1<<GLUT_RIGHT_BUTTON) {
        cam.adjust(0,0,dx,0,0,0);;
    }
    else {
        cam.adjust(-dx*0.2f,-dy*0.2f,0,0,0,0);
    }

    mouse_old_x = x;
    mouse_old_y = y;
}

void keyboard(unsigned char key, int x, int y) {
    float tx = 0;
    float ty = 0;
    float tz = 0;
    switch(key) {
        case(27):
            exit(0.0);
            break;
        case('w'):
            tz = 0.1;
            break;
        case('s'):
            tz = -0.1;
            break;
        case('d'):
            tx = -0.1;
            break;
        case('a'):
            tx = 0.1;
            break;
        case('q'):
            ty = 0.1;
            break;
        case('z'):
            ty = -0.1;
            break;
        case('1'):
            display_type = DISPLAY_DEPTH;
            break;
        case('2'):
            display_type = DISPLAY_NORMAL;
            break;
        case('3'):
            display_type = DISPLAY_COLOR;
            break;
        case('4'):
            display_type = DISPLAY_POSITION;
            break;
        case('5'):
            display_type = DISPLAY_LIGHTS;
            break;
		case('6'):
			display_type = DISPLAY_TOON;
			break;
		case('7'):
			display_type = DISPLAY_BLOOM;
			break;
		case('8'):
			display_type = DISPLAY_AA;
			break;
		case('9'):
			display_type = DISPLAY_SPECULAR;
			break;
        case('0'):
            display_type = DISPLAY_TOTAL;
            break;
        case('x'):
            doIScissor ^= true;
            break;
        case('r'):
            initShader();
            break;
		case('l') :
			pauseLightAnim ^= true;
			break;
		case('c') :
			printCamPosition();
			break;
    }

    if (abs(tx) > 0 ||  abs(tz) > 0 || abs(ty) > 0) {
        cam.adjust(0,0,0,tx,ty,tz);
    }
}

void updateBloomBound(int max, int min)
{
	if (bloomBound > max)
		incBloomBound = false;

	if (bloomBound < min)
		incBloomBound = true;

	if (incBloomBound)
		bloomBound++;
	else
		bloomBound--;
}

void printCamPosition()
{
	cout << "Camera Position : " << cam.pos.x << "," << cam.pos.y << "," << cam.z << endl;
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f,1.0f);
}

int main (int argc, char* argv[])
{
    bool loadedScene = false;
    for(int i=1; i<argc; i++){
        string header; string data;
        istringstream liness(argv[i]);
        getline(liness, header, '='); getline(liness, data, '=');
        if(strcmp(header.c_str(), "mesh")==0){
            int found = data.find_last_of("/\\");
            string path = data.substr(0,found+1);
            cout << "Loading: " << data << endl;
            string err = tinyobj::LoadObj(shapes, data.c_str(), path.c_str());
            if(!err.empty())
            {
                cerr << err << endl;
                return -1;
            }
            loadedScene = true;
        }
    }

    if(!loadedScene){
        cout << "Usage: mesh=[obj file]" << endl; 
        std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
        return 0;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    width = 1280;
    height = 720;
    glutInitWindowSize(width,height);
    glutCreateWindow("CIS565 OpenGL Frame");
    glewInit();
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        cout << "glewInit failed, aborting." << endl;
        exit (1);
    }
    cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;
    cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;

    initNoise();
    initShader();
    initFBO(width,height);
    init();
    initMesh();
    initQuad();
	initLights();


    glutDisplayFunc(display);
    glutReshapeFunc(reshape);	
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}
