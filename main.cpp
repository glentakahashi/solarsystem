// ------------------------
// GLUT harness v. 1.0
// ------------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <assert.h>
#include <math.h>
#include <vector>
#include <string>
#include <sstream>

//file needed for vector arrays
#include "Angel.h"
#include "Quaternion.h"

//include openGL files based on OS
#if defined(__APPLE__)
#include <GLUT/glut.h>
#elif defined(_WIN32)
#include "windows/glut/glut.h"
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

//how many points make up a single trajectory
const int TRAJECTORY_SIZE = 32;
//the number of stars to generate
const int NUM_STARS = 10000;
//number of solar systems to generate
const int NUM_SOLAR_SYSTEMS = 24;
//dimensions of space
const int SPACE_X = 3000;
const int SPACE_Y = 3000;
const int SPACE_Z = 3000;

//the programs for the set of shaders
GLuint planetsProgram;
GLuint starsProgram;
GLuint textProgram;

//prev x,y locations for mouse
int prevX;
int prevY;
//camera location/rotation
float xLoc;
float yLoc;
float zLoc;
float zRot;
float yRot;
//whether or not to animate things
bool spinning;
//the camera (planet) we are attached to
int camera;
//if we are staring at the sun
bool staring;
//draw trjaectories or no
bool drawTrajectories;
//draw axes or not
bool drawAxes;
//the field of view
float fov;

//the origin of main solar system
vec4 origin(10.0,10.0,10.0,1.0);

//the vertex arrays for our shapes
GLuint spheres[8];
GLuint circle;
GLuint axes;
GLuint stars;

//the number of vertcies for our spheres
int sphereVertices[8];

//set defaults
void setDefaults() {
    xLoc = 9.0;
    yLoc = 38.0;
    zLoc = -40.0;
    //-30 degrees
    zRot = -0.523598776;
    yRot = 0;
    spinning = true;
    camera = -1;
    staring = false;
    drawTrajectories = true;
    drawAxes = true;
    fov = 75.0;
}

// Matrix stack that can be used to push and pop the modelview matrix.
class MatrixStack {
    int    _index;
    int    _size;
    mat4*  _matrices;

    public:
    MatrixStack( int numMatrices = 32 ):_index(0), _size(numMatrices)
    { _matrices = new mat4[numMatrices]; }

    ~MatrixStack()
    { delete[]_matrices; }

    void push( const mat4& m ) {
        assert( _index + 1 < _size );
        _matrices[_index++] = m;
    }

    mat4& pop( void ) {
        assert( _index - 1 >= 0 );
        _index--;
        return _matrices[_index];
    }
};

// RGBA colors
vec4 colors[8] = {
    vec4( 1.0, 1.0, 1.0, 1.0 ),  // white
    vec4( 1.0, 1.0, 0.0, 1.0 ),  // yellow
    vec4( 1.0, 0.0, 1.0, 1.0 ),  // magenta
    vec4( 1.0, 0.0, 0.0, 1.0 ),  // red
    vec4( 0.0, 1.0, 1.0, 1.0 ),  // cyan
    vec4( 0.0, 1.0, 0.0, 1.0 ),  // green
    vec4( 0.0, 0.0, 1.0, 1.0 ),  // blue
    vec4( 0.0, 0.0, 0.0, 1.0 )   // black
};

//the model view stack
MatrixStack mvstack;
//the model view matrix
mat4 model_view;
//location of model view in planetsProgram
GLuint mloc;
//the camera view matrix
mat4 camera_view;
//location of camera view in planetsProgram
GLuint cloc;
//location of camera view in starsProgram
GLuint clocs;
//the projection view matrix
mat4 projection_view;
//the location of projection in planetsProgram
GLuint ploc;
//the location of projection in starsProgram
GLuint plocs;
//the location of camera position in planetsProgram
GLuint cploc;
//the location of the rendertype in planetsProgram
GLuint rtloc;

//generate a rotation mat4 matrix around a given vec3 axis and degree in radians
mat4 rotateAroundAxis(vec3 axis, const float theta) {
    Quaternion q;
    GLfloat angle = DegreesToRadians * theta;
    q.FromAxis(axis, angle);
    return q.getMatrix();
}

//render an axis based on the current model view
void renderAxes() {
    //store the model_view
    mvstack.push(model_view);
        //scale the matrix so it is double the size of the model_view
        model_view *= Scale(2,2,2);
        //push model_view to the gpu
        glUniformMatrix4fv(mloc, 1, GL_TRUE, model_view);
        //push the render type (-1 = no shading)
        glUniform1i( rtloc, -1 );
        //bind the vertex array for axes
        glBindVertexArray(axes);
        //get vColor locaiton
        GLuint loc = glGetUniformLocation(planetsProgram, "vColor");
        //draw the 3 lines and set colors accordingly
        //red = x axis
        //green = y axis
        //blus = z axis
        glUniform4fv(loc, 1, colors[3]);
        glDrawArrays(GL_LINE_LOOP,0,2);
        glUniform4fv(loc, 1, colors[5]);
        glDrawArrays(GL_LINE_LOOP,2,2);
        glUniform4fv(loc, 1, colors[6]);
        glDrawArrays(GL_LINE_LOOP,4,2);
   //reset the model_view
   model_view = mvstack.pop();
}

//draw a trajectory with a radius, color, and rotated
//rotMatrix is a matrix that rotates around a given yaw and pitch
void renderTrajectory(mat4 rotMatrix, float radius, vec4 color) {
    //store the model view
    mvstack.push(model_view);
        //the following are done in reverse (because matrix math)
        //rotate it based on the rotationMatrix
        model_view *= rotMatrix;
        //scale the matrix based on radius (in all directions)
        model_view *= Scale(radius,radius,radius);
        //get and set the color
        GLuint loc = glGetUniformLocation(planetsProgram, "vColor");
        glUniform4fv(loc, 1, color);
        //push the model_view to gpu
        glUniformMatrix4fv(mloc, 1, GL_TRUE, model_view);
        //no shading
        glUniform1i( rtloc, -1 );
        //bind the vertex array
        glBindVertexArray(circle);
        //draw it in a line loop
        glDrawArrays(GL_LINE_LOOP,0,TRAJECTORY_SIZE);
    //reset the model_view
    model_view = mvstack.pop();
}

class Satellite {
    private:
        //rotation in orbit (degrees)
        float rot;
        //the axis of rotation
        vec3 axis;
        //the rotation matrix to offset the orbit (orthagonal to the axis)
        mat4 rotMatrix;
        //the render type of this satellite
        int renderType;
        //speeds
        float rotSpeed;
        //how far out orbit is
        float radius;
        //the center of the orbit offset
        vec4 center;
        //complexity/resolution of the sphere
        int complexity;
        //radius of the sphere
        float size;
        vec4 color;
        float ambient;
        float diffuse;
        float specular;
        float shininess;
        //all the child satellites
        std::vector<Satellite*> sats;
        //the name of the satellite
        std::string name;

        //location of the satellite
        vec4 loc;

    public:
        Satellite( float rotHoriz, float rotVert, float rotSpeed, float radius,
                vec4 center, int complexity, float size, vec4 color, int renderType,
                float ambient, float diffuse, float specular, float shininess, std::string name ) {
            //generate the rotation matrix and the axis of rotation
            //kinda tricky math hard to explain
            Quaternion q1;
            q1.FromAxis(vec3(0.0,0.0,1.0),DegreesToRadians * rotVert);
            Quaternion q2;
            q2.FromAxis(vec3(0.0,1.0,0.0),DegreesToRadians * rotHoriz);
            this->rotMatrix = (q1 * q2).getMatrix();
            vec4 y = this->rotMatrix * vec4(0.0,1.0,0.0,0.0);
            this->axis = vec3(y.x,y.y,y.z);
            this->rot = 0;
            this->rotSpeed = rotSpeed;
            this->radius = radius;
            this->center = center;
            this->complexity = complexity;
            this->size = size;
            this->color = color;
            this->renderType = renderType;
            this->ambient = ambient;
            this->diffuse = diffuse;
            this->specular = specular;
            this->shininess = shininess;
            this->name = name;
            this->loc = vec4(0.0,0.0,0.0,1.0);
        }

        //return a string of the stats for our planet
        std::string getStats() {
            std::ostringstream stats;
            stats << this->name << std::endl;
            stats << "Location: " << this->loc.x << ", " << this->loc.y << ", " << this->loc.z << std::endl;
            stats << "Shading type: ";
            switch(this->renderType) {
                case 0:
                    stats << "flat" << std::endl;
                    break;
                case 1:
                    stats << "gouraud" << std::endl;
                    break;
                case 2:
                    stats << "phong" << std::endl;
                    break;
            }
            return stats.str();
        }

        //add a satellite to our planet
        void addSatellite( Satellite* sat ) {
            sats.push_back(sat);
        }

        //if we are spinning, update the rotation and all the child satellites rotations
        void tick() {
            if(spinning) {
                this->rot += this->rotSpeed;
                for(std::vector<Satellite*>::iterator i = sats.begin(); i != sats.end(); ++i) {
                    (*i)->tick();
                }
            }
        }

        //return the center of our satellite
        //useful for determining lightposition of the suns
        vec4 getCenter() {
            return this->center;
        }

        //increase the speed of rotaiton
        void increaseSpeed( float speed ) {
            this->rotSpeed += speed;
        }

        //return the angle of this planet
        float getAngle() {
            return this->rot;
        }

        vec4 getCamera() {
            //return the location plus a little bit more so we are above planet
            return this->loc+vec4(0,this->size*2,0,1.0);
        }

        void render() {
            //just set our loc to be 0 for now
            this->loc = vec4(0.0,0.0,0.0,1.0);
            mvstack.push(model_view);
                //draw trajectories if we enabled
                //at this time there should be nothing modified to the model_view
                //so we are basically drawing the trajectory centered on the parent satellite
                if(drawTrajectories) {
                    renderTrajectory(this->rotMatrix, this->radius, this->color);
                }
                //these are "computed" in reverse becausee of matrix math
                //offset of the orbit
                model_view *= Translate(center.x,center.y,center.z);
                //rotate it around our axis of rotation
                model_view *= rotateAroundAxis(axis, rot);
                //rotate it into the orbit
                model_view *= this->rotMatrix;
                //translate it out of the radius of the orbit
                model_view *= Translate(this->radius,0,0);
                //render each child satellite with our modified model_view
                for(std::vector<Satellite*>::iterator i = this->sats.begin(); i != this->sats.end(); ++i) {
                    (*i)->render();
                }
                //store the location
                this->loc = model_view * this->loc;
                //scale the planet
                model_view *= Scale(this->size,this->size,this->size);
                //push planet properties
                glUniform1f( glGetUniformLocation(planetsProgram, "shininess"), this->shininess );
                glUniform1f( glGetUniformLocation(planetsProgram, "specularAmt"), this->specular );
                glUniform1f( glGetUniformLocation(planetsProgram, "diffuseAmt"), this->diffuse );
                glUniform1f( glGetUniformLocation(planetsProgram, "ambientAmt"), this->ambient );
                GLuint loc = glGetUniformLocation(planetsProgram, "vColor");
                glUniform4fv(loc, 1, this->color);
                //bind model view
                glUniformMatrix4fv(mloc, 1, GL_TRUE, model_view);
                glBindVertexArray(spheres[complexity]);
                glUniform1i( rtloc, this->renderType );
                glDrawArrays(GL_TRIANGLES,0,sphereVertices[complexity]);
                //if we have axis on, draw them
                if(drawAxes) {
                    renderAxes();
                }
            model_view = mvstack.pop();
        }
};

//all the suns
std::vector<Satellite*> sats;
//all the satellites
std::vector<Satellite*> suns;

void triangle(vec3 flatNormals[], vec3 normals[], vec4 points[], const vec4& a, const vec4& b, const vec4& c, int& index )
{
    vec3 flatNormal = normalize( cross(b - a, c - b) );
    vec3 normal;
    normal.x = a.x;
    normal.y = a.y;
    normal.z = a.z;
    flatNormals[index] = flatNormal; normals[index] = normal;  points[index] = a;  index++;
    normal.x = b.x;
    normal.y = b.y;
    normal.z = b.z;
    flatNormals[index] = flatNormal; normals[index] = normal;  points[index] = b;  index++;
    normal.x = c.x;
    normal.y = c.y;
    normal.z = c.z;
    flatNormals[index] = flatNormal; normals[index] = normal;  points[index] = c;  index++;
}

//----------------------------------------------------------------------------

vec4 unit( const vec4& p )
{
    float len = p.x*p.x + p.y*p.y + p.z*p.z;

    vec4 t;
    if ( len > DivideByZeroTolerance ) {
        t = p / sqrt(len);
        t.w = 1.0;
    }

    return t;
}

void divide_triangle( vec3 flatNormals[], vec3 normals[], vec4 points[], const vec4& a, const vec4& b, const vec4& c, int count, int& index )
{
    if ( count > 0 ) {
        vec4 v1 = unit( a + b );
        vec4 v2 = unit( a + c );
        vec4 v3 = unit( b + c );
        divide_triangle( flatNormals, normals, points, a, v1, v2, count - 1, index );
        divide_triangle( flatNormals, normals, points, c, v2, v3, count - 1, index );
        divide_triangle( flatNormals, normals, points, b, v3, v1, count - 1, index );
        divide_triangle( flatNormals, normals, points, v1, v3, v2, count - 1, index );
    }
    else {
        triangle( flatNormals, normals, points, a, b, c, index );
    }
}

void tetrahedron( vec3 flatNormals[], vec3 normals[], vec4 points[], int count )
{

    vec4 v[4] = {
        vec4( 0.0, 0.0, 1.0, 1.0 ),
        vec4( 0.0, 0.942809, -0.333333, 1.0 ),
        vec4( -0.816497, -0.471405, -0.333333, 1.0 ),
        vec4( 0.816497, -0.471405, -0.333333, 1.0 )
    };

    int index = 0;
    divide_triangle( flatNormals, normals, points, v[0], v[1], v[2], count, index );
    divide_triangle( flatNormals, normals, points, v[3], v[2], v[1], count, index );
    divide_triangle( flatNormals, normals, points, v[0], v[3], v[1], count, index );
    divide_triangle( flatNormals, normals, points, v[0], v[2], v[3], count, index );
}

void initSphere() {
    //bind to planets now
    glUseProgram(planetsProgram);
    //genreate 8 vertex arrays, one for each complexity
    glGenVertexArrays(8, spheres);
    for(int numDivisions = 0; numDivisions < 8; numDivisions++) {
        int numVertices = 3 * pow(4,numDivisions+1);
        //store the number of vertices
        sphereVertices[numDivisions] = numVertices;

        //generate points, flatnormals and normals
        vec4 points[numVertices];
        vec3 flatNormals[numVertices];
        vec3 normals[numVertices];
        tetrahedron(flatNormals,normals,points,numDivisions);

        // Create and initialize a buffer object
        GLuint buffer;
        glGenBuffers( 1, &buffer );
        glBindBuffer( GL_ARRAY_BUFFER, buffer );
        glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals) + sizeof(flatNormals),
                NULL, GL_STATIC_DRAW );
        glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
        glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals );
        glBufferSubData( GL_ARRAY_BUFFER, sizeof(points)+sizeof(normals), sizeof(flatNormals), flatNormals );

        glBindVertexArray(spheres[numDivisions]);

        // set up vertex arrays
        GLuint vPosition = glGetAttribLocation( planetsProgram, "vPosition" );
        glEnableVertexAttribArray( vPosition );
        glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                BUFFER_OFFSET(0) );

        GLuint vNormal = glGetAttribLocation( planetsProgram, "vNormal" );
        glEnableVertexAttribArray( vNormal );
        glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                BUFFER_OFFSET(sizeof(points)) );

        GLuint vFlatNormal = glGetAttribLocation( planetsProgram, "vFlatNormal" );
        glEnableVertexAttribArray( vFlatNormal );
        glVertexAttribPointer( vFlatNormal, 3, GL_FLOAT, GL_FALSE, 0,
                BUFFER_OFFSET(sizeof(points)+sizeof(normals)) );
    }
}

void initCircle() {
    //bind to planets now
    glUseProgram(planetsProgram);
    vec4 points[TRAJECTORY_SIZE];
    //generate the number of points around the circle
    for(int i = 0; i<TRAJECTORY_SIZE;i++) {
            float angle = 2.0f * M_PI * i / (float) TRAJECTORY_SIZE;
            points[i].x = cos(angle);
            points[i].z = sin(angle);
            points[i].w = 1.0;
    }
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW );
    glGenVertexArrays(1, &circle);
    glBindVertexArray(circle);
    GLuint vPosition = glGetAttribLocation( planetsProgram, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
}

void initAxes() {
    //bind to planets now
    glUseProgram(planetsProgram);
    vec4 points[6] = { vec4(0.0,0.0,0.0,1.0), vec4(1.0,0.0,0.0,1.0),
        vec4(0.0,0.0,0.0,1.0), vec4(0.0,1.0,0.0,1.0),
        vec4(0.0,0.0,0.0,1.0), vec4(0.0,0.0,1.0,1.0),
    };
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW );
    glGenVertexArrays(1, &axes);
    glBindVertexArray(axes);
    GLuint vPosition = glGetAttribLocation( planetsProgram, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
}

void initStars() {
    //bind to the correct program before initing the arrays/variables
    //if you are not on the correct program before calling these, they will fail
    glUseProgram(starsProgram);
    vec4 points[NUM_STARS];
    vec4 colors[NUM_STARS];
    float sizes[NUM_STARS];
    for(int i = 0;i<NUM_STARS;i++) {
        //choose a random color
        float r = (rand() % 500) / 500.0;
        float g = (rand() % 500) / 500.0;
        float b = (rand() % 500) / 500.0;
        //and calpha
        float a = (rand() % 500) / 500.0;
        //and x,y,z location
        float x = (rand() % SPACE_X)-(SPACE_X/2);
        float y = (rand() % SPACE_Y)-(SPACE_Y/2);
        float z = (rand() % SPACE_Z)-(SPACE_Z/2);
        //if it is near origin, try again
        //dont want it overlapping our beautiful default solar system
        if(abs(x)+abs(y)+abs(z) < 200) {
            i--;
            continue;
        }
        //size is between 0.5-1.7
        float s = (rand() % 600)/500.0+0.5;
        points[i] = vec4(x,y,z,1.0);
        //switch these two to change between white/colored stars
        //colors[i] = vec4(1.0,1.0,1.0,a);
        colors[i] = vec4(r,g,b,a);
        sizes[i] = s;
    }

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors) + sizeof(sizes), NULL, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), sizeof(sizes), sizes );

    glGenVertexArrays(1, &stars);
    glBindVertexArray(stars);

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation( starsProgram, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
            BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( starsProgram, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0,
            BUFFER_OFFSET(sizeof(points)) );

    GLuint size = glGetAttribLocation( starsProgram, "size" );
    glEnableVertexAttribArray( size );
    glVertexAttribPointer( size, 1, GL_FLOAT, GL_FALSE, 0,
            BUFFER_OFFSET(sizeof(points)+sizeof(colors)) );
}

void initSolarSystem() {
    //bind to planets now
    glUseProgram(planetsProgram);
    //zero
    vec4 zero(0.0,0.0,0.0,1.0);
    //sun
    vec4 orange = 0.5*colors[3] + 0.5*colors[1];
    Satellite *sun = new Satellite(180.0,0.0,0.0,0.0,origin,7,6.0,
            orange,0,1.0,1.0,1.0,9.0,"Sun");
    suns.push_back(sun);
    sats.push_back(sun);
    //icy planet
    vec4 icy = colors[0] - 0.2*colors[3] - 0.2*colors[5];
    Satellite *ice = new Satellite(0.0,0.0,0.7,57.0,zero,1,5.0,
            icy,0,0.5,0.5,0.8,6.0,"Frostivus");
    sats.push_back(ice);
    sun->addSatellite(ice);
    //swampy planet
    vec4 swampy = 0.8*colors[5] + 0.4*colors[3];
    Satellite *swamp = new Satellite(-30.0,15.0,0.75,48.0,zero,2,3.0,
            swampy,1,0.5,0.5,0.0,3.0,"Bogoria");
    sats.push_back(swamp);
    sun->addSatellite(swamp);
    //clammy planet + moon
    vec4 water = 0.9*colors[6] + 0.3*colors[5];
    Satellite *clam = new Satellite(0.0,-15.0,-0.6,37.0,zero,6,5.0,
            water,2,0.4,0.3,0.8,9.0,"Atlantis");
    sats.push_back(clam);
    sun->addSatellite(clam);
    Satellite *moon = new Satellite(0.0,80.0,0.5,8.5,zero,2,2.0,
            colors[2],2,0.4,0.2,0.6,2.3,"Titan");
    sats.push_back(moon);
    clam->addSatellite(moon);
    Satellite *moon2 = new Satellite(0.0,-80.0,0.8,3.5,zero,3,0.5,
            colors[7],0,0.4,0.2,0.6,1.3,"Titan junior");
    sats.push_back(moon2);
    moon->addSatellite(moon2);
    //mud planet
    vec4 muddy = 0.8*colors[3] + 0.3*colors[5] + 0.2*colors[6];
    Satellite *mud = new Satellite(-30,45,1.0,11.0,zero,3,2.0,
            muddy,1,0.4,0.1,0.0,9.0,"Murs");
    sats.push_back(mud);
    sun->addSatellite(mud);
    Satellite *moon3 = new Satellite(0.0,20,1,3.5,zero,3,0.5,
            colors[3],0,0.4,0.2,0.6,1.3,"Dwurf");
    sats.push_back(moon3);
    mud->addSatellite(moon3);
    //murs2
    Satellite *murs = new Satellite(0,-10,1.0,18.0,zero,3,2.0,
            colors[2],1,0.4,0.1,0.0,9.0,"Murs Omega");
    sats.push_back(murs);
    sun->addSatellite(murs);
    //add other random solar systems
    for(int i = 0;i < NUM_SOLAR_SYSTEMS; i++){
        int numPlanets = rand() % 5 + 2;
        vec4 loc(rand()%(SPACE_X/2)-(SPACE_X/4),rand()%(SPACE_Y/2)-(SPACE_Y/4),rand()%(SPACE_Z/2)-(SPACE_Z/4),1.0);
        float speed = 0;
        float radius = 0;
        int complexity = rand()%6;
        int rt = rand()%3;
        float size = rand()%18+3;
        float amb = (rand()%500)/500.0;
        float diff = (rand()%500)/500.0;
        float spec = (rand()%500)/500.0;
        int shininess = rand()%14;
        float angleVert = rand()%70;
        float angleHoriz = rand()%360;
        Satellite *sun = new Satellite(angleHoriz,angleVert,speed,radius,loc,complexity,size,
                colors[rand()%8],rt,amb,diff,spec,shininess,"Unnamed");
        suns.push_back(sun);
        for(int j = 0;j < numPlanets; j++) {
            speed = (rand()%500)/500.0+0.5;
            radius += rand()%30+size;
            complexity = rand()%6;
            rt = rand()%3;
            size = rand()%8+2;
            amb = (rand()%500)/500.0;
            diff = (rand()%500)/500.0;
            spec = (rand()%500)/500.0;
            shininess = rand()%14;
            angleVert = rand()%70;
            angleHoriz = rand()%360;
            Satellite *s = new Satellite(angleHoriz,angleVert,speed,radius,zero,complexity,size,
                    colors[rand()%8],rt,amb,diff,spec,shininess,"Unnamed");
            sun->addSatellite(s);
            if((rand()%4)==0) {
                speed = (rand()%500)/500.0+0.5;
                float radius2 = rand()%10+size;
                complexity = rand()%6;
                rt = rand()%3;
                size = rand()%4+1;
                amb = (rand()%500)/500.0;
                diff = (rand()%500)/500.0;
                spec = (rand()%500)/500.0;
                shininess = rand()%14;
                angleVert = rand()%70;
                angleHoriz = rand()%360;
                Satellite *m = new Satellite(angleHoriz,angleVert,speed,radius2,zero,complexity,size,
                        colors[rand()%8],rt,amb,diff,spec,shininess,"Unnamed");
                s->addSatellite(m);
            }
        }
    }
}

void init()
{

    model_view = mat4(1.0f);
    camera_view = mat4(1.0f);
    projection_view = mat4(1.0f);

    // Load shaders and use the resulting shader program
    planetsProgram = InitShader( "vshader.glsl", "fshader.glsl" );
    starsProgram = InitShader( "vshaderstars.glsl", "fshaderstars.glsl" );
    textProgram = InitShader( "vshadertext.glsl", "fshadertext.glsl" );

    initStars();
    initCircle();
    initAxes();
    initSphere();
    initSolarSystem();

    //store the locations
    mloc = glGetUniformLocation( planetsProgram, "model_view" );
    cloc = glGetUniformLocation( planetsProgram, "camera_view" );
    ploc = glGetUniformLocation( planetsProgram, "projection_view" );
    clocs = glGetUniformLocation( starsProgram, "camera_view" );
    plocs = glGetUniformLocation( starsProgram, "projection_view" );
    cploc = glGetUniformLocation( planetsProgram, "cameraPosition" );
    rtloc = glGetUniformLocation( planetsProgram, "renderType" );

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //allow our stars to be different sizes
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable( GL_BLEND );
    glClearColor( 0.0, 0.0, 0.0, 0.0 ); // black background
}

void initGlut(int& argc, char** argv)
{
    //initialize the window
    glutInit( &argc, argv );
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA );
    glutInitWindowSize(1280, 720);
    glutCreateWindow( "Solar System" );

    printf ("Vendor: %s\n", glGetString (GL_VENDOR));
    printf ("Renderer: %s\n", glGetString (GL_RENDERER));
    printf ("Version: %s\n", glGetString (GL_VERSION));
    printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

    //initialize glew
    glewInit();

    //initialize everything else
    init();
}

void drawSpheres() {
    //make sure we are on planets shaders
    glUseProgram(planetsProgram);
    //render each sun
    for(std::vector<Satellite*>::iterator i = suns.begin(); i != suns.end(); ++i) {
        //set the light location to be the center of the suns
        glUniform4fv( glGetUniformLocation(planetsProgram, "lightPosition"),
                1, (*i)->getCenter() );
        //render it
        (*i)->render();
    }
}

void drawStars() {
    //bind to stars shaders
    glUseProgram(starsProgram);
    //starsssssssssssssssssss
    glBindVertexArray(stars);
    glDrawArrays(GL_POINTS,0,NUM_STARS);
}

void doCamera() {
    //eye and ref are same
    vec4 eye = vec4(xLoc,yLoc,zLoc,1.0);
    vec4 ref = vec4(xLoc,yLoc,zLoc,1.0);
    //direciton based off of angle
    vec4 direction = vec4(cos(zRot)*sin(yRot),
            sin(zRot), cos(zRot)*cos(yRot),0.0);
    //if we are on top of a planet
    //get the eye and ref respectively
    if(camera != -1) {
        eye = ref = sats[camera]->getCamera();
        float angle = sats[camera]->getAngle();
        direction = RotateY(-angle) * direction;
    }
    //add direction to ref to get our direction vector
    ref += direction;
    //calculate right based on angles
    vec4 right = vec4(sin(yRot-M_PI/2.0f), 0 , cos(yRot-M_PI/2.0f), 0.0);
    //if we are staring at sun, this all doesnt matter
    //just set our ref/direction
    if(staring) {
        ref = origin;
        direction = eye-origin;
    }
    vec4 up(0.0,1.0,0.0,0.0);
    //store them in GPU
    camera_view = LookAt(eye,ref,up);
    glUseProgram(planetsProgram);
    glUniform4fv(cploc, 1, ref);
    glUniformMatrix4fv(cloc, 1, GL_TRUE, camera_view);
    glUseProgram(starsProgram);
    glUniformMatrix4fv(clocs, 1, GL_TRUE, camera_view);
}

void doModel() {
    //draw our pretty things
    drawSpheres();
    drawStars();
}

void doProjection() {
    //generate our projection matrix with fov and near/far planes
    projection_view = Perspective(fov,16.0/9.0,0.1f,250.0f);
    glUseProgram(planetsProgram);
    glUniformMatrix4fv(ploc, 1, GL_TRUE, projection_view);
    glUseProgram(starsProgram);
    glUniformMatrix4fv(plocs, 1, GL_TRUE, projection_view);
}

void doOverlay() {
    //unbind shaders so we can draw text
    glUseProgram(textProgram);
    //create an ostream so we can addd floats and things
    std::ostringstream text;
    text << "\n\n\nGlen Takahashi - 704004642";
    text << "\nControls:";
    text << "\n    1-9 = lock onto planet";
    text << "\n    0 = unlock from planet";
    text << "\n    -/= = decrease/increase orbit speed\n      (of currently selected planet)";
    text << "\n    d = stare at sun";
    text << "\n    s = toggle animation";
    text << "\n    t = toggle drawing trajectories";
    text << "\n    a = toggle drawing axes";
    text << "\n    n/w = decrease/increase fov";
    text << "\n    r = reset camera";
    text << "\n    arrow keys = angle camera";
    text << "\n    ijkmuo = camera";
    text << "\n    q = quit";
    text << "\noptions: ";
    if(spinning) text << "spinning ";
    if(staring) text << "staring ";
    if(drawTrajectories) text << "trajectories ";
    if(drawAxes) text << "axes ";
    text << "fov:";
    text << fov << std::endl;
    //set the color to be white
    glColor4f(1.0,1.0,1.0,1.0);
    //set the position to be top left cornerr
    glRasterPos3f(-1, 1, 0);
    //draw the string
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (unsigned char*)text.str().c_str());
    //reset it
    text.clear();
    text.str("");
    //add stats if we have them
    text << "\n\n\n";
    if(camera == -1) {
        text << "satellite: none";
    } else {
        text << sats[camera]->getStats();
    }
    //set color to be white
    glColor4f(1.0,1.0,1.0,1.0);
    //move it slightly left
    glRasterPos3f(-0.6, 1, 0);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (unsigned char*)text.str().c_str());
}

// Called when the window needs to be redrawn.
void callbackDisplay()
{
    //clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //draw our things
    doOverlay();
    doModel();
    //set our camera
    doCamera();
    //do our projection
    doProjection();
    //tell it to redraw
    glutPostRedisplay();
    //not sure...
    glutSwapBuffers();
}

// Called when the window is resized.
void callbackReshape (int w, int h){

}

// Called when a key is pressed. x, y is the current mouse position.
void callbackKeyboard(unsigned char key, int x, int y)
{
    if (key == 27) // esc
        exit(0);
    else if (key == 'q' or key == 'Q') // q
        exit(0);
    else if (key == 'n') {
        fov -= 5.0;
        if (fov < 0.0) fov = 0.0;
    }
    else if (key == 'w') {
        fov += 5.0;
        if (fov > 180.0) fov = 180.0;
    }
    else if (key == 's') {
        spinning = !spinning;
    }
    else if (key == 't') {
        drawTrajectories = !drawTrajectories;
    }
    else if (key == 'a') {
        drawAxes = !drawAxes;
    }
    else if (key == 'd') {
        staring = !staring;
    }
    else if (key == 'r') {
        setDefaults();
    }
    else if (key == '0') {
        camera = -1;
    }
    else if (isdigit(key)) {
        int cam = key - '0';
        if(cam < sats.size()) {
            camera = cam;
        }
    }
    if(camera == -1 && !staring) {
        //move our x/z locations based on what key
        //and what our current angle is at the time
        if (key == 'i') {
            zLoc += 10.0*cos(yRot);
            xLoc += 10.0*sin(yRot);
        }
        else if (key == 'j') {
            zLoc -= 10.0*sin(yRot);
            xLoc += 10.0*cos(yRot);
        }
        else if (key == 'k') {
            zLoc += 10.0*sin(yRot);
            xLoc -= 10.0*cos(yRot);
        }
        else if (key == 'm') {
            zLoc -= 10.0*cos(yRot);
            xLoc -= 10.0*sin(yRot);
        }
        //u and o move up and down
        else if (key == 'u') {
            yLoc += 10.0;
        }
        else if (key == 'o') {
            yLoc -= 10.0;
        }
    } else {
        //if we have a camera, we can use - and + to change speed of planet
        if (key == '-') {
            sats[camera]->increaseSpeed(-0.1);
        } else if (key == '=') {
            sats[camera]->increaseSpeed(0.1);
        }
    }
}

void callbackKeyboardSpecial(int key, int x, int y) {
    if(!staring) {
        //if we aren't staring update the angle
        if (key == GLUT_KEY_LEFT)
            yRot += 0.017;
        else if (key == GLUT_KEY_RIGHT)
            yRot -= 0.017;
        else if (key == GLUT_KEY_DOWN)
            zRot -= 0.017;
        else if (key == GLUT_KEY_UP)
            zRot += 0.017;
    }
}

// Called when a mouse button is pressed or released
void callbackMouse(int button, int state, int x, int y)
{
    //each pixel is worth 1/4000th of a degree
    zRot += (y - prevY) * M_PI / 2000.0;
    yRot += (x - prevX) * M_PI / 2000.0;
    prevX = x;
    prevY = y;
}

// Called when the mouse is moved with a button pressed
void callbackMotion(int x, int y)
{
    zRot += (y - prevY) * M_PI / 2000.0;
    yRot += (x - prevX) * M_PI / 2000.0;
    prevX = x;
    prevY = y;
}

// Called when the mouse is moved with no buttons pressed
void callbackPassiveMotion(int x, int y)
{
    prevX = x;
    prevY = y;
}

// Called when the system is idle. Can be called many times per frame.
void callbackIdle()
{
    //tell all suns to tick
    for(std::vector<Satellite*>::iterator i = suns.begin(); i != suns.end(); ++i) {
        (*i)->tick();
    }
}

// Called when the timer expires
void callbackTimer(int)
{
    glutTimerFunc(1000/30, callbackTimer, 0);
    glutPostRedisplay();
}

void initCallbacks()
{
    glutDisplayFunc(callbackDisplay);
    glutReshapeFunc(callbackReshape);
    glutKeyboardFunc(callbackKeyboard);
    glutSpecialFunc(callbackKeyboardSpecial);
    glutMouseFunc(callbackMouse);
    glutMotionFunc(callbackMotion);
    glutPassiveMotionFunc(callbackPassiveMotion);
    glutIdleFunc(callbackIdle);
    glutTimerFunc(1000/30, callbackTimer, 0);
}

int main(int argc, char** argv)
{
    initGlut(argc, argv);
    initCallbacks();
    setDefaults();
    glutMainLoop();
    return 0;
}
