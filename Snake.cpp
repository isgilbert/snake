#include <stdlib.h>
#include <time.h>
#include "Console.h"

#define FPS 10
#define MAX_SEGMENTS 100
#define UP    0
#define DOWN  1
#define RIGHT 2
#define LEFT  3

struct Point {
	int x, y;
};
struct Snake {
	struct Point segments[MAX_SEGMENTS];
    int num_segments;  // how many segments the snake has
    int dir;           // which direction the snake is moving in
};

struct Scene {
	struct Snake snake;
	struct Point fruit_location;
	int fruit_color;
	int score;
}; 

void point_init(struct Point *p, int x, int y);
void snake_init(struct Snake *snake);
void snake_append_head(struct Snake *snake, int x, int y);
void snake_remove_tail(struct Snake *snake);
struct Point snake_get_head(const struct Snake *snake);
struct Point snake_get_segment(const struct Snake *snake, int index);

void scene_init(struct Scene *s);
void scene_render(const struct Scene *s);
int scene_update(struct Scene *s);
void scene_delay(struct Scene *s);

int game_over(void);

int main(void) {
	struct Scene scene;

	scene_init(&scene);

	int scene_active = 1;
	while (scene_active == 1) {
		// Game is in progress. 
		scene_render(&scene);
		cons_update();
		scene_delay(&scene);
		scene_active = scene_update(&scene);

		// Illegal move
		if (scene_active == 2) {
			scene_active = game_over();
			// Reset the scene for if the player opts to play again.
			scene_init(&scene);
		}
	}

	return 0;
}

void point_init(struct Point *p, int x, int y) {
	p->x = x;
	p->y = y;
}
void snake_init(struct Snake *snake) {
	// Initialize snake's config.
	snake->num_segments = 8;
	snake->dir = 2;
	for (int i = 0; i < 8; ++i) {
		(*snake).segments[i].x = i + 5;
		(*snake).segments[i].y = 12;
	}
}
void snake_append_head(struct Snake *snake, int x, int y) {
	 (*snake).segments[(*snake).num_segments].x = x;
	 (*snake).segments[(*snake).num_segments].y = y;
	 ++(snake->num_segments);
}
void snake_remove_tail(struct Snake *snake) {
	for (int i = 1; i <= ((*snake).num_segments); ++i) {
		(*snake).segments[i - 1].x = (*snake).segments[i].x;
		(*snake).segments[i - 1].y = (*snake).segments[i].y;
	}
	--(snake->num_segments);
}
struct Point snake_get_head(const struct Snake *snake) {
	// Depending on direction, add/subtract x/y value accordingly from current
	// head (num_segments - 1) to get the new head.
	struct Point new_point;
	new_point.x = (*snake).segments[((*snake).num_segments) - 1].x;
	new_point.y = (*snake).segments[((*snake).num_segments) - 1].y;
	if ((snake->dir) == 0) {
		--(new_point.y);
	}
	if ((snake->dir) == 1) {
		++(new_point.y);
	}
	if ((snake->dir) == 2) {
		++(new_point.x);
	}
	if ((snake->dir) == 3) {
		--(new_point.x);
	}
	
	return (new_point);
}

void scene_init(struct Scene *s) {
	srand(time(0));
	
	// Initialize snake's config.
	snake_init(&s->snake);
	
	// Initializes position and color of the fruit.
	s->fruit_color = 1;
	s->fruit_location.x = rand() % 80;
	s->fruit_location.y = rand() % 23;
	
	// Initializes score.
	s->score = 0;
}

void scene_render(const struct Scene *s) {
	cons_clear_screen();
	
	// Render snake by looping through array of snake segments.
	cons_change_color(GREEN, BLACK);
	for (int i = 0; i < ((*s).snake.num_segments); ++i) {
		cons_move_cursor((*s).snake.segments[i].y, (*s).snake.segments[i].x);
		cons_printw("*");
	}
	
	// Render fruit according to color value and location.
	if ((s->fruit_color) == 1) {
		cons_change_color(RED, BLACK);
	}
	if ((s->fruit_color) == 2) {
		cons_change_color(RED+INTENSE, BLACK);
	}
	if ((s->fruit_color) == 3) {
		cons_change_color(GREEN, BLACK);
	}
	if ((s->fruit_color) == 4) {
		cons_change_color(GREEN+INTENSE, BLACK);
	}
	if ((s->fruit_color) == 5) {
		cons_change_color(YELLOW, BLACK);
	}
	if ((s->fruit_color) == 6) {
		cons_change_color(YELLOW+INTENSE, BLACK);
	}
	if ((s->fruit_color) == 7) {
		cons_change_color(BLUE, BLACK);
	}
	if ((s->fruit_color) == 8) {
		cons_change_color(BLUE+INTENSE, BLACK);
	}
	if ((s->fruit_color) == 9) {
		cons_change_color(MAGENTA, BLACK);
	}
	if ((s->fruit_color) == 10) {
		cons_change_color(MAGENTA+INTENSE, BLACK);
	}
	if ((s->fruit_color) == 11) {
		cons_change_color(CYAN, BLACK);
	}
	if ((s->fruit_color) == 12) {
		cons_change_color(CYAN+INTENSE, BLACK);
	}
	if ((s->fruit_color) == 13) {
		cons_change_color(GRAY, BLACK);
	}
	if ((s->fruit_color) == 14) {
		cons_change_color(GRAY+INTENSE, BLACK);
	}
	if ((s->fruit_color) == 15) {
		cons_change_color(MAGENTA+INTENSE, BLACK);
	}
	cons_move_cursor((*s).fruit_location.y, (*s).fruit_location.x);
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
	cons_printw("/100");
	cons_change_color(GRAY+INTENSE, BLACK);
	cons_move_cursor(23, 64);
	cons_printw("%03i", (*s).snake.num_segments);

	// Move cursor to bottom-right to make scene look tidier.
	cons_move_cursor(23, 79);
}

int scene_update(struct Scene *s) {
	srand(time(0));
	
	int key = cons_get_keypress();
	
	// Quit the scene at any time if the player presses "Q".
	if (key == 'q') {
		return 0;
	}
	else {
		// Check for direction change. Snake can't go in reverse. 
		if ((key == UP_ARROW) && ((*s).snake.dir != 1)) {
			(*s).snake.dir = 0;
		}
		else if ((key == DOWN_ARROW) && ((*s).snake.dir != 0)) {
			(*s).snake.dir = 1;
		}
		else if ((key == RIGHT_ARROW) && ((*s).snake.dir != 3)) {
			(*s).snake.dir = 2;
		}
		else if ((key == LEFT_ARROW) && ((*s).snake.dir != 2)) {
			(*s).snake.dir = 3;
		}
		
		// Get the snake's new head for its next move, then append it.
		struct Point new_head;
		new_head = snake_get_head(&s->snake);
		snake_append_head(&s->snake, new_head.x, new_head.y);
		
		// Check to see if this new head collides with a wall or with the snake's body.
		// If head does collide, set flag.
		int head_collides = 0;
		
		if ((new_head.x == -1 || new_head.x == 80) || (new_head.y == -1 || new_head.y == 23)) {
			head_collides = 1;
		}
		for (int i = 0; i < ((*s).snake.num_segments - 1); ++i) {
			if ( (new_head.x == (*s).snake.segments[i].x) && (new_head.y == (*s).snake.segments[i].y) ) {
				head_collides = 1;
			}
		}
		
		// Check to see if new head eats a piece of fruit, calculate new score, and generate a new fruit.
		if ((new_head.x == (s->fruit_location.x)) && (new_head.y == (s->fruit_location.y))) {
			s->fruit_color = (rand() % 15) + 1;
			s->fruit_location.x = rand() % 80;
			s->fruit_location.y = rand() % 23;
			s->score += ((((*s).snake.num_segments) - 1) * 10);
			if ((*s).snake.num_segments == MAX_SEGMENTS) {
				snake_remove_tail(&s->snake);
			}
		}
		// Remove tail only if snake doesn't eat fruit.
		else {
			snake_remove_tail(&s->snake);
		}
		
		// If the head collides, game_over() will be called in main. Otherwise, keep going.
		if (head_collides == 1) {
			return 2;
		}
		else {
			return 1;
		}
	}
}

void scene_delay(struct Scene *s) {
	// This function determines how many milliseconds the game is
	// paused between each frame of animation.
	// You can modify this if you want to vary the game speed.
	cons_sleep_ms(1000/FPS);
}

int game_over(void) {
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
	
	int game_over_screen = 1;
	while (game_over_screen == 1) {
		// Wait for player to exit the program or play again.
		int key = cons_get_keypress();
		if (key == 'q') {
			return 0;
		}
		else if (key == 'p') {
			return 1;
		}
	}
	return 0;
}