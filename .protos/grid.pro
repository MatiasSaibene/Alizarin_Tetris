
void
cleanup_grid(Grid *g);
Grid
generate_board(int w, int h, int level);
void
add_garbage(Grid *g);
void
draw_grid(SDL_Surface *screen, color_style *cs, Grid *g, int draw);
void
draw_falling(SDL_Surface *screen, int blockWidth, Grid *g, int offset);
void
fall_down(Grid *g);
int
determine_falling(Grid *g);
int
run_gravity(Grid *g);
int
check_tetris(Grid *g);
