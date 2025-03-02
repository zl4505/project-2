#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif


#define GL_SILENCE_DEPRECATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <vector>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "stb_image.h"
#include <ctime>
#include "cmath"
#include <iostream>

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640,
              WINDOW_HEIGHT = 480;
constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
float g_time_accumulator = 0.0f;
float g_previous_ticks;
constexpr float WINDOW_LEFT = -5.0f;
constexpr float WINDOW_RIGHT = 5.0f;
constexpr float WINDOW_TOP = 3.75f;
constexpr float WINDOW_BOTTOM = -3.75f;

// Background colour components
constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

// Camera Params
constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Font Texture Id
GLuint g_font_texture_id;

// Paddle Params
GLuint g_paddle_texture_id;
constexpr float PADDLE_X_1 = -4.8f;
constexpr float PADDLE_X_2 = 4.8f;

constexpr float PADDLE_WIDTH = 0.4f,
PADDLE_HEIGHT = 3.0f;
float paddle_y_1 = 0.0f;
float paddle_y_2 = 0.0f;

int winner = 0;

constexpr float PADDLE_SPEED = 0.035f;
int paddle_direct = -1;
bool g_single_mod = false;

// Ball Texture Id
constexpr float BALL_WIDTH = 0.5f,
BALL_HEIGHT = BALL_WIDTH;
GLuint g_ball_texture_id;
float BALL_SPEED = 0.04f;

// balls
int ball_num = 1;
float balls[3][5] = {
    {-4.35f, 0.0f, 0.3f, 1.0f, 1.0f},
    {-4.35f, 0.1f, 0.4f, 1.0f, 1.0f},
    {-4.35f, 0.2f, 0.5f, 1.0f, 1.0f},
};


constexpr char V_SHADER_TEXTURE_PATH[] = "SDLProject/shaders/vertex_textured.glsl",
F_SHADER_TEXTURE_PATH[] = "SDLProject/shaders/fragment_textured.glsl";

constexpr char PADDLE_TEXTURE_FILEPATH[] = "assets/chopstick.png",
BALL_TEXTURE_FILEPATH[] = "assets/sushi.png",
FONT_SHEET_FILEPATH[] = "assets/font.png";

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix;
glm::mat4 g_model_matrix;
glm::mat4 g_projection_matrix;


// Our object's fill colour
constexpr float TRIANGLE_RED = 1.0,
TRIANGLE_BLUE = 0.4,
TRIANGLE_GREEN = 0.4,
TRIANGLE_OPACITY = 1.0;


SDL_Window* g_display_window = nullptr;

int g_frame_counter = 0;
bool g_is_growing = true;

bool g_game_is_running = true;
bool g_game_over = false;

float paddle_vert[] = {
    -0.5f, -0.5f,
     0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f,  0.5f
};
float paddle_texture_coordinates[] =
{
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f
};

float ball_vert[] = {
    -0.5f, -0.5f,
     0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f,  0.5f
};
float ball_texture_coordinates[] =
{
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f
};


constexpr int G_MAX_FRAME = 60;

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;
constexpr int FONTBANK_SIZE = 16;

void initialise();
void process_input();
void update();
void render();
void shutdown();


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


// Draw Text
void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    
    for (int i = 0; i < text.size(); i++) {
        int spritesheet_index = (int)text[i];
        float offset = (font_size + spacing) * i;
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
        texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

// Draw Paddle
void draw_paddle(ShaderProgram& program, float x, float y) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(PADDLE_WIDTH, PADDLE_HEIGHT, 1.0f));



    glVertexAttribPointer(program.get_position_attribute(), 2, GL_FLOAT, GL_FALSE, 0, paddle_vert);
    glEnableVertexAttribArray(program.get_position_attribute());
    glVertexAttribPointer(program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, paddle_texture_coordinates);
    program.set_model_matrix(modelMatrix);
    glEnableVertexAttribArray(program.get_tex_coordinate_attribute());
    glBindTexture(GL_TEXTURE_2D, g_paddle_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // We disable two attribute arrays now
    glDisableVertexAttribArray(program.get_position_attribute());
    glDisableVertexAttribArray(program.get_tex_coordinate_attribute());
}

// Draw Ball
void draw_ball(ShaderProgram& program, float x, float y) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(BALL_WIDTH, BALL_HEIGHT, 1.0f));



    glVertexAttribPointer(program.get_position_attribute(), 2, GL_FLOAT, GL_FALSE, 0, ball_vert);
    glEnableVertexAttribArray(program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, ball_texture_coordinates);
    glEnableVertexAttribArray(program.get_tex_coordinate_attribute());
    program.set_model_matrix(modelMatrix);
    glBindTexture(GL_TEXTURE_2D, g_ball_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // We disable two attribute arrays now
    glDisableVertexAttribArray(program.get_position_attribute());
    glDisableVertexAttribArray(program.get_tex_coordinate_attribute());
}

// Check Collision
bool is_colliding(const float ball[], const float x, const float y, const float width, const float height) {
    float x_distance = fabs(ball[0] - x) - ((BALL_WIDTH + width) / 2.0f);
    float y_distance = fabs(ball[1] - y) - ((BALL_HEIGHT + height) / 2.0f);

    if (x_distance <= 0 && y_distance < 0){
        // Collision!
        return true;
    }
    return false;
}

// Balls Moving
void move_balls() {

    // According to ball_num
    for (int i = 0; i < ball_num; i++) {

        // check paddle collision
        if (is_colliding(balls[i], PADDLE_X_1, paddle_y_1, PADDLE_WIDTH, PADDLE_HEIGHT)) {
            balls[i][3] *= -1;
        }
        else if (is_colliding(balls[i], PADDLE_X_2, paddle_y_2, PADDLE_WIDTH, PADDLE_HEIGHT)) {
            balls[i][3] *= -1;
        }
        else if (is_colliding(balls[i], 0.0f, WINDOW_TOP, WINDOW_WIDTH, 0.0f)) {
            balls[i][4] *= -1;
        }
        else if (is_colliding(balls[i], 0.0f, WINDOW_BOTTOM, WINDOW_WIDTH, 0.0f)) {
            balls[i][4] *= -1;
        }

        balls[i][0] += cosf(balls[i][2]) * BALL_SPEED * balls[i][3];
        balls[i][1] += sinf(balls[i][2]) * BALL_SPEED * balls[i][4];

        // Judge Game Over
        if (balls[i][0] > 5.0) {
            g_game_over = true;
            winner = 0;
        }
        else if (balls[i][0] < -5.0) {
            g_game_over = true;
            winner = 1;
        }
    }
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    
    if (g_display_window == nullptr) {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_game_is_running = false;

        SDL_Quit();
        exit(1);
    }
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_TEXTURE_PATH, F_SHADER_TEXTURE_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_model_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(WINDOW_LEFT, WINDOW_RIGHT, WINDOW_BOTTOM, WINDOW_TOP, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);


    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load Texture
    g_font_texture_id = load_texture(FONT_SHEET_FILEPATH);
    g_paddle_texture_id = load_texture(PADDLE_TEXTURE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_TEXTURE_FILEPATH);
}

void process_input() {

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:g_game_is_running = false;break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym){
                // Change Single-Double Mod
                case SDLK_t:g_single_mod = !g_single_mod; break;
            }
        default: break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (!g_game_over) {

        if (key_state[SDL_SCANCODE_W]) {
            //LOG("Player 1 Up");
            paddle_y_1 += PADDLE_SPEED;
        }
        if (key_state[SDL_SCANCODE_S]) {
            //LOG("Player 1 Down");
            paddle_y_1 -= PADDLE_SPEED;
        }

        // Change Number of Balls
        if (key_state[SDL_SCANCODE_1]) {
            ball_num = 1;
        }
        if (key_state[SDL_SCANCODE_2]) {
            ball_num = 2;
        }
        if (key_state[SDL_SCANCODE_3]) {
            ball_num = 3;
        }

        if (!g_single_mod) {

            if (key_state[SDL_SCANCODE_UP]) {
                //LOG("Player 2 Up");
                paddle_y_2 += PADDLE_SPEED;
            };
            if (key_state[SDL_SCANCODE_DOWN]) {
                //LOG("Player 2 Down");
                paddle_y_2 -= PADDLE_SPEED;
            };
        }
    }

    // Limit Paddle 's Top and Bottom
    paddle_y_1 = glm::clamp(paddle_y_1, -3.75f + PADDLE_HEIGHT / 2, 3.75f - PADDLE_HEIGHT / 2);
    paddle_y_2 = glm::clamp(paddle_y_2, -3.75f + PADDLE_HEIGHT / 2, 3.75f - PADDLE_HEIGHT / 2);
}

void update() {
    // move balls
    move_balls();

    // This is for auto move paddle
    if (!g_game_over && g_single_mod) {
        if (paddle_y_2 <= (-3.75f + PADDLE_HEIGHT / 2) || paddle_y_2 >= (3.75f - PADDLE_HEIGHT / 2)) {
            paddle_direct *= -1;
        }
        paddle_y_2 += paddle_direct * PADDLE_SPEED;
    }

    // Using Delta Time to Keep 60fps
    float ticks = (float)SDL_GetTicks();
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_time_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP) {
        delta_time -= FIXED_TIMESTEP;
    }

    g_time_accumulator = delta_time;
}


void render() {
    // Step 1
    glClear(GL_COLOR_BUFFER_BIT);

    // Step 2
    g_shader_program.set_model_matrix(g_model_matrix);

    // Step 3
    // Draw Paddles
    draw_paddle(g_shader_program, PADDLE_X_1, paddle_y_1);
    draw_paddle(g_shader_program, PADDLE_X_2, paddle_y_2);

    // Draw Ball
    for (int i = 0; i < ball_num; i++) {
        draw_ball(g_shader_program, balls[i][0], balls[i][1]);
    }
//3
    // Draw Text
    if (g_game_over) {
        draw_text(&g_shader_program, g_font_texture_id, "Game Over!", 0.5f, 0.05f,
            glm::vec3(-2.5f, 2.0f, 0.0f));
        if (winner == 0) {
            draw_text(&g_shader_program, g_font_texture_id, "Winner is Player1", 0.5f, 0.05f,
                glm::vec3(-4.5f, 1.0f, 0.0f));
        }
        else {
            draw_text(&g_shader_program, g_font_texture_id, "Winner is Player2", 0.5f, 0.05f,
                glm::vec3(-4.5f, 1.0f, 0.0f));

        }
    }

    // Step 4
    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    initialise();

    float lastTicks = 0.0f;

    while (g_game_is_running) {

        float ticks = (float)SDL_GetTicks() / 1000.0f;
        float elapsed = ticks - lastTicks;
        lastTicks = ticks;

        process_input();
        if(!g_game_over) {
            update();
        }
        render();
    }

    shutdown();
    return 0;
}
