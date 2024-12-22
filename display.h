#include <SDL3/SDL.h>
#ifndef display_h
	#define display_h
	#define scale 10
	struct Display{
		SDL_Window* window;
		SDL_Surface* surface;
		Display();
		~Display();
		void Draw(bool displayData[]);
	};
#endif
