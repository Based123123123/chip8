#include "defines.h"
#include "display.h"
#include <iostream>
Display::Display(){
	if(!SDL_Init(SDL_INIT_VIDEO))
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
	window = SDL_CreateWindow("CHIP-8",640,320,0);
	surface = SDL_GetWindowSurface(window);
}
Display::~Display(){
	SDL_DestroySurface(surface);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Display::Draw(bool displayData[]){
	SDL_LockSurface(surface);
	ui32* pixelPtr = (ui32*)surface->pixels;
	for(ui32 i=0;i<displayHeight;i++)
		for(ui32 j=0;j<displayWidth;j++){
			ui32 pixelX=j*10, pixelY=i*10;
			for(ui32 dy=0;dy<10;dy++)
				for(ui32 dx=0;dx<10;dx++){
					pixelPtr[(pixelY+dy)*displayWidth*10+pixelX+dx]=displayData[i*displayWidth+j]?0xFFFFFF:0;
				}
		}
				
	SDL_UnlockSurface(surface);
	SDL_UpdateWindowSurface(window);
}
