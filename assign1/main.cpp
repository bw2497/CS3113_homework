/**
* Author: Bo Wen
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif 

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                
#include "glm/gtc/matrix_transform.hpp"  
#include "ShaderProgram.h"               
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640*2,
WINDOW_HEIGHT = 480*2;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;


constexpr GLint NUMBER_OF_TEXTURES = 1, 
LEVEL_OF_DETAIL = 0, 
TEXTURE_BORDER = 0; 

constexpr char sw_SPRITE_FILEPATH[] = "silverwolf.png",
herta_SPRITE_FILEPATH[] = "herta.png";

constexpr glm::vec3 INIT_SCALE = glm::vec3(5.0f, 5.98f, 0.0f);


constexpr float ORBIT_RADIUS = 2.0f;
constexpr float DIAG_SPEED = 0.5f;
constexpr float ROT_SPEED = 1.0f;
constexpr float SW_MOVEMENT_RANGE = 2.0f;

//constexpr float GROWTH_FACTOR = 1.1f;  
//constexpr float SHRINK_FACTOR = 0.9f;  
//constexpr int MAX_FRAME = 60;           
constexpr float SCALE_SPEED = 1.0f;
constexpr float SCALE_MIN = 0.5f, SCALE_MAX = 2.0f;

int g_frame_counter = 0;
bool g_is_growing = true;

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;
ShaderProgram g_shader_program = ShaderProgram();


glm::mat4 g_view_matrix,
g_projection_matrix, 
g_sw_matrix, 
g_herta_matrix;

glm::vec3 g_position_sw = glm::vec3(-3.0f, 0.0f, 0.0f);
glm::vec3 g_position_herta = glm::vec3(3.0f, 0.0f, 0.0f);

float g_previous_ticks = 0.0f;
float g_herta_rotation_angle = 0.0f;
float g_sw_rotation_angle = 0.0f;



GLuint g_sw_texture_id,
g_herta_texture_id;


GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Assign1!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;

        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_sw_matrix = glm::mat4(1.0f);
    g_herta_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);


    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    g_sw_texture_id = load_texture(sw_SPRITE_FILEPATH);
    g_herta_texture_id = load_texture(herta_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update() {

    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_herta_rotation_angle += ROT_SPEED * delta_time*2;
    g_sw_rotation_angle += ROT_SPEED * delta_time;

    g_position_sw.x = SW_MOVEMENT_RANGE * sin(ticks);
    g_position_sw.y = SW_MOVEMENT_RANGE * sin(ticks);

    glm::vec3 scale_vector;
/*
    g_frame_counter += 1;

    if (g_frame_counter >= MAX_FRAME)
    {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }

    scale_vector = glm::vec3(g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
        g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
        1.0f);
    */

    float scale_factor = SCALE_MIN + (SCALE_MAX - SCALE_MIN) * (0.5f + 0.5f * sin(ticks * SCALE_SPEED));
    scale_vector = glm::vec3(scale_factor, scale_factor, 1.0f);

    g_sw_matrix = glm::mat4(1.0f);
    g_sw_matrix = glm::translate(g_sw_matrix, g_position_sw);
    g_sw_matrix = glm::scale(g_sw_matrix, scale_vector);
    g_sw_matrix = glm::rotate(g_sw_matrix, g_sw_rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));

    g_position_herta.x = g_position_sw.x + ORBIT_RADIUS * cos(ticks);
    g_position_herta.y = g_position_sw.y + ORBIT_RADIUS * sin(ticks);

    g_herta_matrix = glm::mat4(1.0f);
    g_herta_matrix = glm::translate(g_herta_matrix, g_position_herta);
    g_herta_matrix = glm::rotate(g_herta_matrix, g_herta_rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));

}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); 
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,  // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f   // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_sw_matrix, g_sw_texture_id);
    draw_object(g_herta_matrix, g_herta_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();  
        update();        
        render();         
    }

    shutdown();  
    return 0;
}
