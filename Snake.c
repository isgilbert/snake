#include <stdlib.h>
#include <time.h>
#include "Console.h"

#define FPS 10

#define FIELD_WIDTH 80
#define FIELD_HEIGHT 23

#define INIT_SEGMENTS 8

#define MAX_SEGMENTS 100

enum Direction { UP = 0, DOWN = 1, RIGHT = 2, LEFT = 3 };

enum SceneResult { SCENE_QUIT = 0, SCENE_CONTINUE = 1, SCENE_GAME_OVER = 2 };

struct Point {
	int x, y;
};

struct Snake {
	struct Point segments[MAX_SEGMENTS + 1]; // 1 extra slot for the transient head
	int num_segments;	// how many segments the snake has
	int dir;			// which direction the snake is moving in
};

struct Scene {
	struct Snake snake;
	struct Point fruit_location;
	int fruit_color;
	int score;
}; 

static void point_init(struct Point *p, int x, int y);
static void snake_init(struct Snake *snake);
static void snake_append_head(struct Snake *snake, int x, int y);
static void snake_remove_tail(struct Snake *snake);
static struct Point snake_get_next_head(const struct Snake *snake);

static void scene_init(struct Scene *s);
static void scene_render(const struct Scene *s);
static enum SceneResult scene_update(struct Scene *s);
static void scene_delay(void);

static enum SceneResult game_over(void);

int main(void) {

	srand((unsigned)time(0));

	struct Scene scene;

	scene_init(&scene);

	enum SceneResult state = SCENE_CONTINUE;
	while (state == SCENE_CONTINUE) {
		// Game is in progress. 
		scene_render(&scene);
		cons_update();
		scene_delay();
		state = scene_update(&scene);

		// Illegal move
		if (state == SCENE_GAME_OVER) {
			state = game_over();
			// Reset the scene for if the player opts to play again.
			scene_init(&scene);
		}
	}

	return 0;
}

static void point_init(struct Point *p, int x, int y) {
	p->x = x;
	p->y = y;
}

static void snake_init(struct Snake *snake) {
	// Initialize snake's config.
	snake->num_segments = INIT_SEGMENTS;
	snake->dir = RIGHT;
	for (int i = 0; i < INIT_SEGMENTS; ++i) {
		point_init(&snake->segments[i], i + 5, 12);
	}
}

static void snake_append_head(struct Snake *snake, int x, int y) {
	if (snake->num_segments > MAX_SEGMENTS) return;
	point_init(&snake->segments[snake->num_segments], x, y);
	++(snake->num_segments);
}

static void snake_remove_tail(struct Snake *snake) {
	for (int i = 1; i < snake->num_segments; ++i) {
		snake->segments[i - 1] = snake->segments[i];
	}
	--(snake->num_segments);
}

static struct Point snake_get_next_head(const struct Snake *snake) {
	// Depending on direction, add/subtract x/y value accordingly from current
	// head (num_segments - 1) to get the new head.
	struct Point new_point;
	point_init(&new_point, 
		snake->segments[snake->num_segments - 1].x,
		snake->segments[snake->num_segments - 1].y);
	switch (snake->dir) {
		case UP:	--(new_point.y); break;
		case DOWN:	++(new_point.y); break;
		case RIGHT:	++(new_point.x); break;
		case LEFT:	--(new_point.x); break;
	}
	
	return (new_point);
}

// check whether the snake intersects a provided point
static int point_on_snake(const struct Snake *snake, int x, int y) {
	for (int i = 0; i < snake->num_segments; ++i) {
		if (snake->segments[i].x == x && snake->segments[i].y == y) {
			return 1;   // this point is on the snake
		}
	}
	return 0;
}

static void scene_init(struct Scene *s) {
	
	// Initialize snake's config.
	snake_init(&s->snake);
	
	// Initializes position and color of the fruit.
	s->fruit_color = 1;
	point_init(&s->fruit_location, rand() % FIELD_WIDTH, rand() % FIELD_HEIGHT);
	
	// Initializes score.
	s->score = 0;
}

static const int FRUIT_COLORS[] = {
	RED, RED+INTENSE, GREEN, GREEN+INTENSE, YELLOW, YELLOW+INTENSE,
	BLUE, BLUE+INTENSE, MAGENTA, MAGENTA+INTENSE, CYAN, CYAN+INTENSE,
	GRAY, GRAY+INTENSE
};

#define NUM_FRUIT_COLORS ((int)(sizeof FRUIT_COLORS / sizeof *FRUIT_COLORS))

static void scene_render(const struct Scene *s) {
	cons_clear_screen();
	
	// Render snake by looping through array of snake segments.
	cons_change_color(GREEN, BLACK);
	for (int i = 0; i < (s->snake.num_segments); ++i) {
		cons_move_cursor(s->snake.segments[i].y, s->snake.segments[i].x);
		cons_printw("*");
	}
	
	// Render fruit according to color value and location.
	int idx = s->fruit_color - 1;
	if (idx >= 0 && idx < (int)(sizeof FRUIT_COLORS/sizeof *FRUIT_COLORS)) {
		cons_change_color(FRUIT_COLORS[idx], BLACK);
	}
	
	cons_move_cursor(s->fruit_location.y, s->fruit_location.x);
	cons_printw("O");
	
	// Render player's score and number of segments.
	cons_change_color(GRAY, BLACK);
	cons_move_cursor(23, 10);
	cons_printw("Score:");
	cons_change_color(GRAY+INTENSE, BLACK);
	cons_move_cursor(23, 17);
	cons_printw("%06i", s->score);
	
	cons_change_color(GRAY, BLACK);
	cons_move_cursor(23, 54);
	cons_printw("Segments:");
	cons_move_cursor(23, 67);
	cons_printw("/%d", MAX_SEGMENTS);
	cons_change_color(GRAY+INTENSE, BLACK);
	cons_move_cursor(23, 64);
	cons_printw("%03i", s->snake.num_segments);

	// Move cursor to bottom-right to make scene look tidier.
	cons_move_cursor(23, 79);
}

static enum SceneResult scene_update(struct Scene *s) {

	int key = cons_get_keypress();
	
	// Quit the scene at any time if the player presses "Q".
	if (key == 'q') {
		return SCENE_QUIT;
	}
	else {
		// Check for direction change. Snake can't go in reverse. 
		if ((key == UP_ARROW) && (s->snake.dir != DOWN)) {
			s->snake.dir = UP;
		}
		else if ((key == DOWN_ARROW) && (s->snake.dir != UP)) {
			s->snake.dir = DOWN;
		}
		else if ((key == RIGHT_ARROW) && (s->snake.dir != LEFT)) {
			s->snake.dir = RIGHT;
		}
		else if ((key == LEFT_ARROW) && (s->snake.dir != RIGHT)) {
			s->snake.dir = LEFT;
		}
		
		// Get the snake's new head for its next move, then append it.
		struct Point new_head;
		new_head = snake_get_next_head(&s->snake);
		snake_append_head(&s->snake, new_head.x, new_head.y);
		
		// Check to see if this new head collides with a wall or with the snake's body.
		// If head does collide, set flag.
		int head_collides = 0;
		
		if ((new_head.x == -1 || new_head.x == FIELD_WIDTH) || (new_head.y == -1 || new_head.y == FIELD_HEIGHT)) {
			head_collides = 1;
		}
		for (int i = 1; i < (s->snake.num_segments - 1); ++i) {
			if ( (new_head.x == s->snake.segments[i].x) && (new_head.y == s->snake.segments[i].y) ) {
				head_collides = 1;
			}
		}
		
		// Check to see if new head eats a piece of fruit, calculate new score, and generate a new fruit.
		if ((new_head.x == (s->fruit_location.x)) && (new_head.y == (s->fruit_location.y))) {
			s->fruit_color = (rand() % NUM_FRUIT_COLORS) + 1;
			do {
				point_init(&s->fruit_location, rand() % FIELD_WIDTH, rand() % FIELD_HEIGHT);
			// prevent fruit spawning on snake
			} while (point_on_snake(&s->snake, s->fruit_location.x, s->fruit_location.y)); 

			s->score += (((s->snake.num_segments) - 1) * 10);
			if (s->snake.num_segments > MAX_SEGMENTS) {
				snake_remove_tail(&s->snake);
			}
		}
		// Remove tail only if snake doesn't eat fruit.
		else {
			snake_remove_tail(&s->snake);
		}
		
		// If the head collides, game_over() will be called in main. Otherwise, keep going.
		return head_collides ? SCENE_GAME_OVER : SCENE_CONTINUE;
	}
}

static void scene_delay(void) {
	// This function determines how many milliseconds the game is
	// paused between each frame of animation.
	// You can modify this if you want to vary the game speed.
	cons_sleep_ms(1000/FPS);
}

static enum SceneResult game_over(void) {
	// Print game-over screen. 
	cons_change_color(GRAY+INTENSE, BLACK);
	cons_move_cursor(10, 28);
	cons_printw("Game Over!");
	cons_move_cursor(12, 28);
	cons_printw("Press P to play again.");
	cons_move_cursor(13, 28);
	cons_printw("Press Q to quit.");
	
	// Move cursor to bottom-right to make scene look tidier.
	cons_move_cursor(23, 79);
	
	cons_update();
	
	for (;;) {
		int key = cons_wait_for_keypress();
		if (key == 'q') return SCENE_QUIT;
		if (key == 'p') return SCENE_CONTINUE;
	}
}