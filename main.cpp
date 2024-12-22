#include "chip8.h"
int main(i32 argc, i8** argv){
	if(argc>1){
		Chip8* chip8=new Chip8(argv[1]);
		delete chip8;
	}
	else
		printf("No rom specified.\n");
	return 0;
}
