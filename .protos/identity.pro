
char *
input_string(SDL_Surface *screen, int x, int y, int opaque);
identity *
load_identity_file();
void 
save_identity_file(identity *id, char *new_name, int new_level);
char *
network_choice(SDL_Surface *screen);
int 
who_are_you(SDL_Surface *screen, identity **id, int taken, int p);
