//#define NDEBUG

#include <libdragon.h>

static resolution_t LO_RES_MODE;
static resolution_t HI_RES_MODE;

typedef struct rotating_point_s
{
    float x;
    float y;
    float angle;
} rotating_point_t;

#define INTERNAL_WIDTH (320.0f)
#define INTERNAL_HEIGHT (240.0f)
static float screen_scale_x = 1.0f;
static float screen_scale_y = 1.0f;

#define SCR_SPACE_X(x) ((x) * screen_scale_x)
#define SCR_SPACE_Y(y) ((y) * screen_scale_y)

static bool interlace_mode = false;
static bool radar_mode = false;
static bool stop_movement = false;

#define TRACE_POS_X (160)
#define TRACE_POS_Y (120)
#define TRACE_RADIUS (80)
#define TRACE_ANGULAR_SPEED (0.01f)
static float trace_angular_speed = TRACE_ANGULAR_SPEED;

static rotating_point_t circle_pos = {0, 0, 0};
#define CIRCLE_RADIUS (30)

static rotating_point_t radar_pos = {0, 0, 0.5f};

static color_t color_bright;
static color_t color_dark;

void fill_vertex_array(float* array, float x, float y, color_t color)
{
    array[0] = x;
    array[1] = y;
    array[2] = color.r / 255.0f;
    array[3] = color.g / 255.0f;
    array[4] = color.b / 255.0f;
    array[5] = color.a / 255.0f;    
}

void init_display_mode()
{
    if (get_tv_type() == TV_PAL)
    {
        debugf("TV Type is PAL\n");
        LO_RES_MODE = (resolution_t) {320, 288, INTERLACE_OFF};
        HI_RES_MODE = (resolution_t) {640, 576, INTERLACE_HALF};
    }
    else
    {
        debugf("TV Type is NTSC\n");
        LO_RES_MODE = (resolution_t) {320, 240, INTERLACE_OFF};
        HI_RES_MODE = (resolution_t) {640, 480, INTERLACE_HALF};
    }
}

void update_display_mode(bool init)
{
    if (!init)
    {
        rspq_wait();
        display_close();
        rspq_wait();
    }

    if (interlace_mode)
    {
        display_init(HI_RES_MODE, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);
        color_bright = RGBA32(0,255,0,255);
        color_dark = RGBA32(0,60,0,255);
    }
    else
    {
        display_init(LO_RES_MODE, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);
        color_bright = RGBA32(255,0,0,255);
        color_dark = RGBA32(60,0,0,255);
    }

    screen_scale_x = display_get_width() / INTERNAL_WIDTH;
    screen_scale_y = display_get_height() / INTERNAL_HEIGHT;
    
    debugf("Resolution: %ldx%ld (%f:%f)\n", display_get_width(), display_get_height(), screen_scale_x, screen_scale_y);
}

float update_angle(float angle)
{
    // Increment the angle based on the speed
    angle += trace_angular_speed;
    
    // Keep the angle within the range [0, 2*PI]
    if (angle >= 2 * M_PI)
    {
        angle -= 2 * M_PI;
    }

    return angle;
}

void update_rotating_point(rotating_point_t* point)
{
    point->angle = update_angle(point->angle);
    
    // Compute the new position
    point->x = TRACE_POS_X + TRACE_RADIUS * fm_cosf(point->angle);
    point->y = TRACE_POS_Y + TRACE_RADIUS * fm_sinf(point->angle);
}

void render_radar()
{
    rdpq_set_mode_standard();
    // Set vertex array format to {X, Y, R, G, B, A}
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);

    // Alloc arrays for point coordinates
    float central_point[6];
    float radar_point1[6];
    float radar_point2[6];
    // Fill the arrays with relevant data; X and Y are converted to screen space
    fill_vertex_array(central_point, SCR_SPACE_X(TRACE_POS_X), SCR_SPACE_Y(TRACE_POS_Y), color_dark);
    fill_vertex_array(radar_point1, SCR_SPACE_X(circle_pos.x), SCR_SPACE_Y(circle_pos.y), color_bright);
    fill_vertex_array(radar_point2, SCR_SPACE_X(radar_pos.x), SCR_SPACE_Y(radar_pos.y), color_bright);

    // Draw rotating triangle
    rdpq_triangle(&TRIFMT_SHADE, central_point, radar_point1, radar_point2);
}

void render_circle(int number_of_points)
{
    rdpq_set_mode_fill(color_bright);

    // Draw dot in the middle of the tracing circle to visualize better
    rdpq_fill_rectangle(
        SCR_SPACE_X(TRACE_POS_X - 1),
        SCR_SPACE_Y(TRACE_POS_Y - 1),
        SCR_SPACE_X(TRACE_POS_X + 1),
        SCR_SPACE_Y(TRACE_POS_Y + 1)
    );
    // Draw a 20px bar in the top-left half of the screen
    rdpq_fill_rectangle(
        SCR_SPACE_X(0),
        SCR_SPACE_Y(0),
        SCR_SPACE_X(INTERNAL_WIDTH / 2),
        SCR_SPACE_Y(20)
    );
    // Draw a 20px bar in the bottom-left half of the screen
    rdpq_fill_rectangle(
        SCR_SPACE_X(INTERNAL_WIDTH / 2),
        SCR_SPACE_Y(INTERNAL_HEIGHT - 20),
        SCR_SPACE_X(INTERNAL_WIDTH),
        SCR_SPACE_Y(INTERNAL_HEIGHT)
    );

    rdpq_set_mode_standard();
    // Set vertex array format to {X, Y} with a flat color (color set on next line)
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(color_bright);

    float circle_center[2] = {SCR_SPACE_X(circle_pos.x), SCR_SPACE_Y(circle_pos.y)};

    float circle_vertexes[number_of_points][2];
    float angle_step = 2 * M_PI / (float) number_of_points;

    for (int i = 0; i < number_of_points; i++)
    {
        float angle = angle_step * i;
        circle_vertexes[i][0] = SCR_SPACE_X(circle_pos.x + CIRCLE_RADIUS * fm_cosf(angle));
        circle_vertexes[i][1] = SCR_SPACE_Y(circle_pos.y + CIRCLE_RADIUS * fm_sinf(angle));
    }

    // Draw pseudo circle
    for (int i = 0; i < number_of_points; i++)
    {
        int next = (i + 1) % number_of_points;
        rdpq_triangle(&TRIFMT_FILL, circle_center, circle_vertexes[i], circle_vertexes[next]);
    }
}

int main()
{
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();
    rdpq_init();
    //rdpq_debug_start();
    debug_init_isviewer();

    init_display_mode();

    update_display_mode(true);

    for (;;)
    {
        joypad_poll();
        joypad_buttons_t buttons = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (buttons.a)
        {
            interlace_mode = !interlace_mode;
            update_display_mode(false);
        }
        if (buttons.b)
        {
            radar_mode = !radar_mode;
            trace_angular_speed = (radar_mode) ? 8 * TRACE_ANGULAR_SPEED : TRACE_ANGULAR_SPEED;
        }
        if (buttons.z)
        {
            stop_movement = !stop_movement;
            trace_angular_speed = (stop_movement) ? 0 : TRACE_ANGULAR_SPEED;
        }

        update_rotating_point(&circle_pos);
        update_rotating_point(&radar_pos);

        rdpq_attach_clear(display_get(), NULL);
        rdpq_clear(RGBA32(0,0,100,255));

        if (radar_mode)
        {
            render_radar();
        }
        else
        {
            render_circle(32);
        }

        rdpq_detach_show();
    }
}
