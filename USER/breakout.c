#include "breakout.h"
#include "stm32f10x.h"
#include "oled.h"
#include "key.h"
#include "timer.h"
#include "bsp_iwdg.h"

#define BROW            4
#define BCOL            8
#define BRICK_WIDTH     14
#define BRICK_HEIGHT    5
#define BRICK_GAP_X     1
#define BRICK_GAP_Y     1
#define BRICK_X         4
#define BRICK_Y         4
#define PADDLE_WIDTH    16
#define PADDLE_HEIGHT   4
#define PADDLE_Y        58
#define BALL_SIZE       3
#define FRAME_INTERVAL  20
#define MAX_ANGLE       2.0f

typedef struct {
    float x, y;
    float vx, vy;
} Ball_t;

typedef struct {
    u8 x;
} Paddle_t;

typedef enum {
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_WIN
} GameState_t;

static u8 bricks[BROW][BCOL];
static Ball_t ball;
static Paddle_t paddle;
static GameState_t game_state;

static float Breakout_Abs(float value)
{
    return value < 0.0f ? -value : value;
}

static void Breakout_DrawRect(u8 x, u8 y, u8 width, u8 height)
{
    u8 dx;
    u8 dy;

    for (dx = 0; dx < width; dx++)
    {
        for (dy = 0; dy < height; dy++)
        {
            OLED_DrawPoint(x + dx, y + dy, 1);
        }
    }
}

static void Breakout_Draw(void)
{
    u8 row;
    u8 col;
    u8 x;
    u8 y;

    OLED_Clear_GRAM();
    for (row = 0; row < BROW; row++)
    {
        for (col = 0; col < BCOL; col++)
        {
            if (bricks[row][col])
            {
                x = BRICK_X + col * (BRICK_WIDTH + BRICK_GAP_X);
                y = BRICK_Y + row * (BRICK_HEIGHT + BRICK_GAP_Y);
                Breakout_DrawRect(x, y, BRICK_WIDTH, BRICK_HEIGHT);
            }
        }
    }

    Breakout_DrawRect(paddle.x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT);
    Breakout_DrawRect((u8)(ball.x + 0.5f), (u8)(ball.y + 0.5f),
                      BALL_SIZE, BALL_SIZE);
    OLED_Refresh();
}

static void Breakout_DrawEnd(void)
{
    OLED_Clear_GRAM();
    if (game_state == STATE_WIN)
    {
        OLED_ShowString(32, 24, (u8 *)"YOU WIN!", 16, 1);
    }
    else
    {
        OLED_ShowString(16, 24, (u8 *)"GAME OVER", 16, 1);
    }
    OLED_Refresh();
}

static void Breakout_Init(void)
{
    u8 row;
    u8 col;

    for (row = 0; row < BROW; row++)
    {
        for (col = 0; col < BCOL; col++)
        {
            bricks[row][col] = 1;
        }
    }

    paddle.x = 56;
    ball.x = 62.0f;
    ball.y = 48.0f;
    ball.vx = 1.1f;
    ball.vy = -1.4f;
    game_state = STATE_PLAYING;
}

static void Breakout_Update(void)
{
    float old_x;
    float old_y;
    float brick_x;
    float brick_y;
    float hit_pos;
    u8 row;
    u8 col;

    old_x = ball.x;
    old_y = ball.y;
    ball.x += ball.vx;
    ball.y += ball.vy;

    if (ball.x < 0.0f)
    {
        ball.x = 0.0f;
        ball.vx = Breakout_Abs(ball.vx);
    }
    else if (ball.x > 128 - BALL_SIZE)
    {
        ball.x = 128 - BALL_SIZE;
        ball.vx = -Breakout_Abs(ball.vx);
    }

    if (ball.y < 0.0f)
    {
        ball.y = 0.0f;
        ball.vy = Breakout_Abs(ball.vy);
    }
    if (ball.y > 60.0f)
    {
        game_state = STATE_GAMEOVER;
        return;
    }

    if (ball.vy > 0.0f &&
        ball.x + BALL_SIZE > paddle.x &&
        ball.x < paddle.x + PADDLE_WIDTH &&
        ball.y + BALL_SIZE > PADDLE_Y &&
        ball.y < PADDLE_Y + PADDLE_HEIGHT)
    {
        hit_pos = (ball.x + BALL_SIZE / 2.0f -
                   (paddle.x + PADDLE_WIDTH / 2.0f)) /
                  (PADDLE_WIDTH / 2.0f);
        if (hit_pos < -1.0f) hit_pos = -1.0f;
        if (hit_pos > 1.0f) hit_pos = 1.0f;
        ball.vx = hit_pos * MAX_ANGLE;
        ball.vy = -Breakout_Abs(ball.vy);
        ball.y = PADDLE_Y - BALL_SIZE;
    }

    for (row = 0; row < BROW; row++)
    {
        for (col = 0; col < BCOL; col++)
        {
            if (!bricks[row][col]) continue;

            brick_x = BRICK_X + col * (BRICK_WIDTH + BRICK_GAP_X);
            brick_y = BRICK_Y + row * (BRICK_HEIGHT + BRICK_GAP_Y);
            if (ball.x + BALL_SIZE > brick_x &&
                ball.x < brick_x + BRICK_WIDTH &&
                ball.y + BALL_SIZE > brick_y &&
                ball.y < brick_y + BRICK_HEIGHT)
            {
                bricks[row][col] = 0;
                if (old_x + BALL_SIZE <= brick_x ||
                    old_x >= brick_x + BRICK_WIDTH)
                {
                    ball.vx = -ball.vx;
                    ball.x = old_x;
                }
                else
                {
                    ball.vy = -ball.vy;
                    ball.y = old_y;
                }
                break;
            }
        }
        if (col < BCOL) break;
    }

    for (row = 0; row < BROW; row++)
    {
        for (col = 0; col < BCOL; col++)
        {
            if (bricks[row][col]) return;
        }
    }
    game_state = STATE_WIN;
}

void Breakout_Run(void)
{
    u32 last_frame;
    u32 now;

    Breakout_Init();
    last_frame = Timer_GetMs();
    Breakout_Draw();

    while (1)
    {
        IWDG_Feed();
        Key_Scan(&key_left);
        Key_Scan(&key_right);
        Key_Scan(&key_enter);

        if (game_state != STATE_PLAYING)
        {
            if (Key_GetEvent(&key_enter) == KEY_EVENT_DOWN) break;
        }
        else
        {
            if (Key_GetEvent(&key_left) == KEY_EVENT_DOWN)
            {
                if (paddle.x >= 2) paddle.x -= 2;
                else paddle.x = 0;
            }
            if (Key_GetEvent(&key_right) == KEY_EVENT_DOWN)
            {
                if (paddle.x <= 110) paddle.x += 2;
                else paddle.x = 112;
            }
            Key_GetEvent(&key_enter);

            now = Timer_GetMs();
            if (now - last_frame >= FRAME_INTERVAL)
            {
                last_frame = now;
                Breakout_Update();
                if (game_state == STATE_PLAYING) Breakout_Draw();
                else Breakout_DrawEnd();
            }
        }
    }

    OLED_Clear();
}
