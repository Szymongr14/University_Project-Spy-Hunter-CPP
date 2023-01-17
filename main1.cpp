#include<math.h>
#include<stdio.h>
#include<string.h>
#include <time.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#define NORMAL_CAR_SPEED 200
#define MAX_CAR_SPEED 500
#define MIN_CAR_SPEED 150
#define BOARD_TILES 10
#define FILE_NAME_LENGTH 23
#define FILENAMES_OF_STATS "filenames_of_saved_stats.txt"
#define NUMBER_OF_STATS "number_of_saved_stats.txt"
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

//ignore fsnanf and fprintf warnings
#define _CRT_SECURE_NO_WARNINGS
#pragma warning( disable : 6031 )

typedef struct {
	const int r;
	const int g;
	const int b;
} RGBOfPixel;
RGBOfPixel GrassRGB = {25,64,17};

struct GameState {
	double worldTime;
	int score;
	double distance;
	double CarSpeed;
	double CarX;
};

struct Timing {
	int t1;
	int t2;
	double delta;
	double fpsTimer;
	double fps;
	int frames;
};

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text,SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	}
}

void FreeSurfaces(SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer * renderer) {
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
}

void RestartGame(struct GameState* gameState) {
	gameState->worldTime = 0;
	gameState->score = 0;
	gameState->distance = (-BOARD_TILES) * SCREEN_HEIGHT;
	gameState->CarSpeed = NORMAL_CAR_SPEED;
	gameState->CarX = SCREEN_WIDTH / 2;
}

void PauseGame(SDL_Event event,int *quit, int * wasPause) {
	bool pause = true;
	while (pause) {
		while (SDL_WaitEvent(&event)) {
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				*quit = 1;
				pause = false;
				break;
			}
			else if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_p || event.key.keysym.sym == SDLK_n)) {
				pause = false;
				break;
			}
		}
	}
	*wasPause = 1;
}

int ChoosingfFilename(SDL_Event event, int* quit, int* wasPause, int *cancel_load) {
	bool pause = true;
	while (pause) {
		while (SDL_WaitEvent(&event)) {
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				*quit = 1;
				pause = false;
				break;
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) {
				*wasPause = 1;
				return event.key.keysym.sym - SDLK_0;
				break;
			}
			else if (event.key.keysym.sym == SDLK_c) {
				*cancel_load = 1;
				pause = false;
				break;
			}
		}
	}
	*wasPause = 1;
}

char* ReadFilename(const char* filename, int number_of_filename) {
	FILE* fp;
	char* napis;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("Nie uda�o si� otworzy� pliku");
		return NULL;
	}
	
	if (number_of_filename != 0) {
		fseek(fp, number_of_filename * (FILE_NAME_LENGTH + 2), SEEK_CUR);
	}
	else {
		fseek(fp, 0, SEEK_CUR);
	}

	napis = (char*)malloc(FILE_NAME_LENGTH + 1);
	if (napis == NULL) {
		fclose(fp);
		fputs("Nie uda�o si� zaalokowa� pami�ci", stderr);
		return NULL;
	}

	fread(napis, FILE_NAME_LENGTH, 1, fp);
	fclose(fp);

	napis[FILE_NAME_LENGTH] = '\0';
	return napis;
}

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt �rodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
}
// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt �rodka obrazka sprite na ekranie
void DrawBoard(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x =x;
	dest.y =y;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
}

Uint32 GetPixel(SDL_Surface* screen, int x, int y) {
	int bytes_per_pixel = screen->format->BytesPerPixel;
	Uint8* pixel = (Uint8*)screen->pixels + y * screen->pitch + x * bytes_per_pixel;
	switch (bytes_per_pixel) {
		case 1:
			return *pixel;
			break;
		case 2:
			return *(Uint16*)pixel;
			break;
		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN) return pixel[0] << 16 | pixel[1] << 8 | pixel[2];
			else return pixel[0] | pixel[1] << 8 | pixel[2] << 16;
			break;

		case 4:
			return *(Uint32*)pixel;
			break;
	}
}

// rysowanie linii o d�ugo�ci l w pionie (gdy dx = 0, dy = 1) 
// b�d� poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	}
}

// rysowanie prostok�ta o d�ugo�ci bok�w l i k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,Uint32 outlineColor, Uint32 fillColor, bool filling) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	if (filling) {
		for (i = y + 1; i < y + k - 1; i++)
			DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	}
}

void PrintListOfFilenames(SDL_Surface* screen, SDL_Surface* charset, int number_of_filenames, int borderColor, int fillingColor, bool printControllsInfo) {
	char text[90];
	DrawRectangle(screen, 4, 50, SCREEN_WIDTH - 8, 350, borderColor, fillingColor, 1);
	sprintf(text, "To choose which file you want to load press wanted number:");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 60, text, charset);
	sprintf(text, "Press c to cancel ");
	DrawString(screen, 40, 370, text, charset);

	for (int i = 1; i <= number_of_filenames; i++) {
		sprintf(text, "%d. %s",i,ReadFilename(FILENAMES_OF_STATS, i - 1));
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 80+(i*15), text, charset);
	}
}

void PrintTimeAndScore(SDL_Surface* screen, SDL_Surface* charset, const float WorldTime, const float score, int borderColor, int fillingColor, bool printControllsInfo) {
	char text[60];
	//time and score
	DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 50, borderColor, fillingColor, 1);
	sprintf(text, "Duration Time = %.1lf s    Score: %.0lf", WorldTime,score);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 30, text, charset);
	
	//controls info
	if (printControllsInfo) {
		sprintf(text, "Szymon Groszkowski nr 193141");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
		DrawRectangle(screen, 470, 400, 150, 75, borderColor, fillingColor, 1);
		sprintf(text, "Esc - exit");
		DrawString(screen, 480, 405, text, charset);
		sprintf(text, "\030 - acceleration");
		DrawString(screen, 480, 415, text, charset);
		sprintf(text, "\031 - deceleration ");
		DrawString(screen, 480, 425, text, charset);
		sprintf(text, "n - restart game ");
		DrawString(screen, 480, 435, text, charset);
		sprintf(text, "p - pause game ");
		DrawString(screen, 480, 445, text, charset);
		sprintf(text, "s - save game  ");
		DrawString(screen, 480, 455, text, charset);
		sprintf(text, "l - load game  ");
		DrawString(screen, 480, 465, text, charset);
	}
}

int CheckCollision1(SDL_Surface* screen,SDL_Rect* CarHitbox) {
	Uint32 left_top_pixel = GetPixel(screen, CarHitbox->x, CarHitbox->y);
	Uint32 right_top_pixel = GetPixel(screen, CarHitbox->x+ CarHitbox->w, CarHitbox->y);

	// Get the red, green, blue, and alpha values of the pixel
	Uint8 r, g, b;
	Uint8 r1, g1, b1;

	SDL_GetRGB(left_top_pixel, screen->format, &r, &g, &b);
	SDL_GetRGB(right_top_pixel, screen->format, &r1, &g1, &b1);

	// Print the color values
	printf("has color (R: %d, G: %d, B: %d)\n",r1, g1, b1);
	if ((r == GrassRGB.r && g== GrassRGB.g && GrassRGB.b==b)|| (r1 == GrassRGB.r && g1 == GrassRGB.g && b1==GrassRGB.b)) {
		return 1;
	}
	return 0;
}

void UpdateScreen(SDL_Surface* screen, SDL_Renderer* renderer, SDL_Texture* scrtex) {
	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	//SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void get_current_time(char* buffer, size_t size) {
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, size, "%Y_%m_%d_%H_%M_%S.txt", timeinfo);
}

SDL_Surface* LoadSprite(const char* filepath,SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer) {
	SDL_Surface* sprite = SDL_LoadBMP(filepath);
	if (!sprite) {
		printf("Error: Failed to load sprite. %s\n", SDL_GetError());
		FreeSurfaces(screen, scrtex, window, renderer);
		return NULL;
	}
	return sprite;
}

int number_of_saved_stats() {
	FILE* file;
	int number;
	file = fopen(NUMBER_OF_STATS, "r");
	fscanf(file, "%d", &number);
	return number;
}

void load(const char* filename, struct GameState* gameState) {
	FILE* file;
	file = fopen(filename, "r");
	fscanf(file, "%lf", &gameState->worldTime);
	fscanf(file, "%d", &gameState->score);
	fscanf(file, "%lf", &gameState->distance);
	fscanf(file, "%lf", &gameState->CarSpeed);
	fscanf(file, "%lf", &gameState->CarX);
	fclose(file);
}

void save(struct GameState* gameState) {
	char buffer[25];
	get_current_time(buffer, sizeof(buffer));

	FILE* file;
	file = fopen(buffer, "w");
	fprintf(file, "%lf\n", gameState->worldTime);
	fprintf(file, "%d\n", gameState->score);
	fprintf(file, "%lf\n", gameState->distance);
	fprintf(file, "%lf\n", gameState->CarSpeed);
	fprintf(file, "%lf\n", gameState->CarX);
	fclose(file);

	int number = number_of_saved_stats();
	file = fopen(NUMBER_OF_STATS, "w");
	fprintf(file, "%d", number + 1);
	fclose(file);

	file = fopen(FILENAMES_OF_STATS, "a");
	fwrite(buffer, strlen(buffer), 1, file);
	fwrite("\n", 1, 1, file);
	fclose(file);
}


void load_filenames(char filename[20],const int number_of_filenames) {
	FILE* file;
	int number;
	file = fopen(filename, "r");
}

void update_per_frame(struct Timing* timing, int* wasPause, int* cancel_load, struct GameState* gameState) {
	timing->t2 = SDL_GetTicks();
	// w tym momencie t2-t1 to czas w milisekundach,jaki uplyna� od ostatniego narysowania ekranu delta to ten sam czas w sekundach
	if (!(*wasPause)) timing->delta = (timing->t2 - timing->t1) * 0.001;
	*wasPause = 0;
	timing->t1 = timing->t2;
	gameState->worldTime += timing->delta;
	gameState->score += (gameState->CarSpeed / 480);
	gameState->distance += gameState->CarSpeed * (timing->delta);
	*cancel_load = 0;

	timing->fpsTimer += timing->delta;
	if (timing->fpsTimer > 0.5) {
		timing->fps = timing->frames * 2;
		timing->frames = 0;
		timing->fpsTimer -= 0.5;
	}
}


#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char **argv) {
	int quit, frames, rc, pause, wasPause, cancel_load;
	char text[128], buffer[15];
	struct GameState gameState = { 0, 0, 0, 0, 0 };
	struct Timing timing = { SDL_GetTicks(), 0, 0, 0, 0, 0 };
	SDL_Event event{};
	SDL_Surface* screen;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Rect CarHitbox;
	
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) printf("SDL_Init error: %s\n", SDL_GetError());
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "Spy Hunter Game");
	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_ShowCursor(SDL_DISABLE);//wy��czenie widoczno�ci kursora myszy

	SDL_Surface* charset = LoadSprite("./Surfaces/cs8x8.bmp",screen, scrtex, window, renderer);
	SDL_SetColorKey(charset, true, 0x000000);
	SDL_Surface* board = LoadSprite("./Surfaces/plansza13.bmp", screen, scrtex, window, renderer);
	SDL_Surface* gameOverScreen = LoadSprite("./Surfaces/gameover.bmp", screen, scrtex, window, renderer);
	SDL_Surface* pauseScreen = LoadSprite("./Surfaces/pausescreen.bmp", screen, scrtex, window, renderer);
	SDL_Surface* darkScreen = LoadSprite("./Surfaces/dark_theme.bmp", screen, scrtex, window, renderer);
	SDL_Surface* car = LoadSprite("./Surfaces/car.bmp", screen, scrtex, window, renderer);
	CarHitbox = { 600,700,car->w - 25 ,car->h - 10 };

	int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int yellow = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0x00);

	wasPause = quit = pause = cancel_load = 0;
	RestartGame(&gameState);
	CarHitbox.y = 300 - CarHitbox.h / 2;

	while(!quit) {
		update_per_frame(&timing,&wasPause,&cancel_load,&gameState);
		DrawBoard(screen, board, 0, gameState.distance);//scrolling board
		if (gameState.distance > 0) gameState.distance = (-BOARD_TILES) * SCREEN_HEIGHT;
		CarHitbox.x = gameState.CarX - CarHitbox.w / 2;
		DrawSurface(screen, car, gameState.CarX, 300);
		
		if (CheckCollision1(screen, &CarHitbox)){//collisions check
			DrawBoard(screen, gameOverScreen,0, 0);
			PrintTimeAndScore(screen, charset, gameState.worldTime, gameState.score, yellow, blue, 0);
			UpdateScreen(screen, renderer, scrtex);
			PauseGame(event, &quit,&wasPause);
			RestartGame(&gameState);
		}
		
		PrintTimeAndScore(screen, charset, gameState.worldTime, gameState.score, yellow, blue,1);
		while(SDL_PollEvent(&event)){ //handling of events (if there were any)
			switch(event.type){
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					else if (event.key.keysym.sym == SDLK_UP && gameState.CarSpeed < MAX_CAR_SPEED) gameState.CarSpeed *= 1.2;
					else if(event.key.keysym.sym == SDLK_DOWN && gameState.CarSpeed > MIN_CAR_SPEED) gameState.CarSpeed/=1.2;
					else if (event.key.keysym.sym == SDLK_LEFT) gameState.CarX -=20;
					else if (event.key.keysym.sym == SDLK_RIGHT) gameState.CarX +=20;
					else if (event.key.keysym.sym == SDLK_n) RestartGame(&gameState);
					else if (event.key.keysym.sym == SDLK_p){	
						DrawBoard(screen, pauseScreen, 0, 0);
						UpdateScreen(screen, renderer, scrtex);
						PauseGame(event, &quit,&wasPause);
					}
					else if (event.key.keysym.sym == SDLK_l) {
						DrawBoard(screen, darkScreen, 0, 0);
						PrintListOfFilenames(screen, charset,number_of_saved_stats(), yellow, blue, 0);
						UpdateScreen(screen, renderer, scrtex);
						int filename_number = ChoosingfFilename(event, &quit, &wasPause, &cancel_load);
						if (!quit && !cancel_load) {
							char* filename = ReadFilename(FILENAMES_OF_STATS, filename_number - 1);
							load(filename, &gameState);
							free(filename);
						}
					}
					else if (event.key.keysym.sym == SDLK_s) {
						save(&gameState);
					}
					break;
			}
		}
		UpdateScreen(screen, renderer, scrtex);
		timing.frames++;
	}
	FreeSurfaces(screen, scrtex, window, renderer);
	return 0;
}
