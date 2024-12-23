#include "chip8.h"
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <thread>
#define hrcn() std::chrono::high_resolution_clock::now()
#define countms(end,start) std::chrono::duration_cast<std::chrono::microseconds>((end)-(start)).count()
#define sleep(t) std::this_thread::sleep_for(std::chrono::microseconds(t))
Chip8::Chip8(const i8* rom){
	PC = PCInit;
	I = 0;
	SP = 0;
	delayTimer = 0;
	soundTimer = 0;
	wait = false;
	draw = false;
	memset(keypad,0,sizeof(keypad));
	memset(displayData,0,displayWidth*displayHeight);
	memset(ram,0,ramSize);
	memcpy(ram+font,fontData,sizeof(fontData));
	std::ifstream file(std::string("./roms/")+rom,std::ios::binary);
	ui16 index=0;
	ui8 ch;
	while(file.read(reinterpret_cast<i8*>(&ch),sizeof(ch)))
		ram[PCInit+(index++)]=static_cast<ui8>(ch);
	printf("%x\n",PCInit+index);
	display = new Display();
	//ram[0x1ff]=1;
	loop();
}

void Chip8::loop(){
	bool pressed = false;
	ui8 key;
	while(true){
		auto frame = hrcn();
		if(delayTimer>0)
			delayTimer--;
		if(soundTimer>0)
			soundTimer--;
		if(draw){
			display->Draw(displayData);
			draw = false;
		}
		SDL_Event e;
		while(SDL_PollEvent(&e)){
			switch(e.type){
				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP:
					checkKey();
					break;
				case SDL_EVENT_QUIT:
					return;
				default:
					break;
			}
		}
		ui16 opcode;
		for(ui8 i=0;i<10&&countms(hrcn(),frame)<16666;i++){
			if(!wait&&!draw){
				opcode = fetch();
				decode(opcode);
			}
			else if(wait && !pressed){
				key = getKey();
				if(key!=0xff){
					//V(opcode&0xf00>>8)=key;
					//wait = false;
					pressed = true;
				}
			}
			else if(wait && pressed){
				if(!keypad[key]){
					V(opcode&0xf00>>8)=key;
					wait = false;
					pressed = false;
				}		
			}
		}
		while(countms(hrcn(),frame)<16666){}
	}
}

ui16 Chip8::fetch(){
	ui16 instruction=(ram[PC]<<8)+ram[PC+1];
	PC+=2;
	return instruction;
}

void Chip8::decode(ui16 instr){
	switch(instr>>12){
		case 0x0:
			switch(instr&0x0fff){
				case 0x00e0:
					// 00E0 Clear screen
					std::memset(displayData,0,displayWidth*displayHeight); 
					draw = true;
					break;
				case 0x00ee:
					// 00EE Return
					PC = S(--SP);
					PC += S(--SP)<<8;
					break;
				default:
					// 0NNN Machine code call
					// Wont implement for the time being
					printf("Unimplemented %x\n",instr);
					break;
			}
			break;
		case 0x1:
			// 1NNN Jump
			PC = instr&0x0fff;
			break;
		case 0x2:
			// 2NNN Call
			S(SP++) = PC>>8;
			S(SP++) = PC&0xff;
			PC = instr&0x0fff;
			break;
		case 0x3:
			// 3XNN Skip one instruction if VX is equal to NN
			if(V(instr>>8&0xf)==(instr&0xff))
				PC+=2;
			break;
		case 0x4:
			// 4XNN Skip one instruction if VX is not equal to NN
			if(V(instr>>8&0xf)!=(instr&0xff))
				PC+=2;
			break;
		case 0x5:
			if((instr&0xf)==0){
				// 5XY0 Skip one instruction if VX and VY are equal
				if(V(instr>>8&0xf)==V(instr>>4&0xf))
					PC+=2;
			}
			else
				printf("Unimplemented %x, PC:%x\n",instr,PC);
			break;
		case 0x6:
			// 6XNN Set register VX
			V(instr>>8&0xf)=instr&0xff;
			break;
		case 0x7:
			// 7XNN Add value to register VX
			V(instr>>8&0xf)+=instr&0xff;
			break;
		case 0x8:
			switch(instr&0xf){
				case 0x0:
					// 8XY0 Set VX = VY
					V(instr>>8&0xf)=V(instr>>4&0xf);
					break;
				case 0x1:
					// 8XY1 Binary OR VX |= VY
					V(instr>>8&0xf)|=V(instr>>4&0xf);
					V(0xF)=0;
					break;
				case 0x2:
					// 8XY2 Binary AND VX &= VY
					V(instr>>8&0xf)&=V(instr>>4&0xf);
					V(0xF)=0;
					break;
				case 0x3:
					// 8XY3 Logical XOR VX ^= VY
					V(instr>>8&0xf)^=V(instr>>4&0xf);
					V(0xF)=0;
					break;
				case 0x4:{
					// 8XY4 Add VX += VY
					ui8 vX = V(instr>>8&0xf), vY = V(instr>>4&0xf);
					V(instr>>8&0xf)+=V(instr>>4&0xf);
					if(vX+vY>0xff)
						V(0xF)=1;
					else
						V(0xF)=0;
					break;
				}
				case 0x5:{
					// 8XY5 Subtract VX -= VY
					ui8 vX = V(instr>>8&0xf), vY = V(instr>>4&0xf);
					V(instr>>8&0xf)-=V(instr>>4&0xf);
					if(vX>=vY)
						V(0xF)=1;
					else
						V(0xF)=0;
					break;
				}
				case 0x6:{
					// 8XY6 Shift to the right by 1
					ui8 vY = V(instr>>4&0xf);
					V(instr>>8&0xf)=V(instr>>4&0xf)>>1;
					V(0xF)=vY%2;
					//V(0xF)=V(instr>>4&0xf)&0xf;
					break;
				}
				case 0x7:{
					// 8XY7 Subtract VX = VY - VX
					ui8 vX = V(instr>>8&0xf), vY = V(instr>>4&0xf);
					V(instr>>8&0xf)=V(instr>>4&0xf)-V(instr>>8&0xf);
					if(vX>vY)
						V(0xF)=0;
					else
						V(0xF)=1;
					break;
				}
				case 0xe:{
					// 8XYE Shift to the left by 1
					ui8 vY = V(instr>>4&0xf);
					V(instr>>8&0xf)=V(instr>>4&0xf)<<1;
					V(0xF)=vY>>7;
					break;
				}
				default:
					 printf("Unimplemented %x\n",instr);
			}
			break;
		case 0x9:
			if((instr&0xf)==0){
				// 9XY0 Skip one instruction if VX and VY are not equal
				if(V(instr>>8&0xf)!=V(instr>>4&0xf))
					PC+=2;
			}
			else
				printf("Unimplemented %x\n",instr);
			break;
		case 0xa:
			// ANNN Set index register I
			I = instr&0xfff; 
			break;
		case 0xb:
			// BNNN Jump with offset
			//PC = V(instr>>8&0xf)+(instr&0xfff);
			PC = V(0)+(instr&0xfff);
			break;
		case 0xc:
			// CXNN Random
			V(instr>>8&0xf)=(rand()%256)&0xff;
			break;
		case 0xd:{
			// DXYN Display
			ui8 x = V((instr&0xf00)>>8)%displayWidth, y = V((instr&0xf0)>>4)%displayHeight, n = instr&0xf;
			//if((x!=V((instr&0xf00)>>8))||(y!=V((instr&0xf0)>>4))){
			//	printf("x: %d, wrapped x: %d, y: %d, wrapped y: %d\n",V((instr&0xf00)>>8),x,V((instr&0xf0)>>4),y);
			//}
			if(!draw){
				V(0xF)=0;
				for(ui8 i=0;i<n;i++){
					//if(!ram[I+i])
					//	continue;
					for(ui8 tX=0;tX<8;tX++){
						if(x+tX<displayWidth){
							bool pixel=(ram[I+i]>>(7-tX))%2;
							if(!pixel)
								continue;
							if(displayData[y*displayWidth+x+tX])
							//if(displayData[y*displayWidth+x+tX]^pixel)
								V(0xF)=1;
							displayData[y*displayWidth+x+tX]^=pixel;
						}
						else
							break;
					}
					y++;
					if(y>=displayHeight)
						break;	
				}
				draw = true;
			}
			else
				PC--;
			break;
		}
		case 0xe:
			switch(instr&0xff){
				case 0x9e:
					// EX9E Skip if key pressed
					if(keypad[(V(instr>>8&0xf)&0xf)])
						PC+=2;
					break;
				case 0xa1:
					// EXA1 Skip if key not pressed
					if(!keypad[(V(instr>>8&0xf)&0xf)])
						PC+=2;
					break;
				default:
					printf("Unimplemented %x\n",instr);	
			}
			break;
		case 0xf:
			switch(instr&0xff){
				case 0x07:
					// FX07 Set VX equal to delay timer
					V(instr>>8&0xf) = delayTimer;
					break;
				case 0x0a:
					// FX0A Get Key
					wait=true;
					break;
				case 0x15:
					// FX15 Set delay timer equal to VX
					delayTimer = V(instr>>8&0xf);
					break;
				case 0x18:
					// FX18 Set sound timer equal to VX
					soundTimer = V(instr>>8&0xf);
					break;
				case 0x1e:
					// FX1E Add to index I+=VX
					I+=V(instr>>8&0xf);
					if(I&0xf000){
						V(0xF)=1;
						I&=0xfff;
					}
					break;
				case 0x29:
					// FX29 Set I to font character
					I=font+(V(instr>>8&0xf)&0xf)*5;
					//I=(V(instr>>8&0xf))*5;
					break;
				case 0x33:
					// FX33 Binary-coded decimal conversion
					ram[I]=V(instr>>8&0xf)/100;
					ram[I+1]=V(instr>>8&0xf)/10%10;
					ram[I+2]=V(instr>>8&0xf)%10;
					break;
				case 0x55:
					// FX55 Store memory
					for(ui8 i=0;i<=(instr>>8&0xf);i++)
						ram[I+i]=V(i);
					I+=(instr>>8&0xf)+1;
					break;
				case 0x65:
					// FX65 Load memory
					for(ui8 i=0;i<=(instr>>8&0xf);i++)
						V(i)=ram[I+i];
					I+=(instr>>8&0xf)+1;
					break;
				default:
					printf("Unimplemented %x\n",instr);
			}
			break;
	}
}

ui8 Chip8::getKey(){
	const bool* state = SDL_GetKeyboardState(nullptr);
	if(state[SDL_SCANCODE_1])
		return 1;	
	if(state[SDL_SCANCODE_2])
		return 2;
	if(state[SDL_SCANCODE_3])
		return 3;
	if(state[SDL_SCANCODE_4])
		return 0xc;
	if(state[SDL_SCANCODE_Q])
		return 4;
	if(state[SDL_SCANCODE_W])
		return 5;
	if(state[SDL_SCANCODE_E])
		return 6;
	if(state[SDL_SCANCODE_R])
		return 0xd;
	if(state[SDL_SCANCODE_A])
		return 7;
	if(state[SDL_SCANCODE_S])
		return 8;
	if(state[SDL_SCANCODE_D])
		return 9;
	if(state[SDL_SCANCODE_F])
		return 0xe;
	if(state[SDL_SCANCODE_Z])
		return 0xa;
	if(state[SDL_SCANCODE_X])
		return 0;
	if(state[SDL_SCANCODE_C])
		return 0xb;
	if(state[SDL_SCANCODE_V])
		return 0xf;
	return 0xff;
}

void Chip8::checkKey(){
	const bool* state = SDL_GetKeyboardState(nullptr);
	keypad[1]=state[SDL_SCANCODE_1];
	keypad[2]=state[SDL_SCANCODE_2];
	keypad[3]=state[SDL_SCANCODE_3];
	keypad[0xc]=state[SDL_SCANCODE_4];
	keypad[4]=state[SDL_SCANCODE_Q];
	keypad[5]=state[SDL_SCANCODE_W];
	keypad[6]=state[SDL_SCANCODE_E];
	keypad[0xd]=state[SDL_SCANCODE_R];
	keypad[7]=state[SDL_SCANCODE_A];
	keypad[8]=state[SDL_SCANCODE_S];
	keypad[9]=state[SDL_SCANCODE_D];
	keypad[0xe]=state[SDL_SCANCODE_F];
	keypad[0xa]=state[SDL_SCANCODE_Z];
	keypad[0]=state[SDL_SCANCODE_X];
	keypad[0xb]=state[SDL_SCANCODE_C];
	keypad[0xf]=state[SDL_SCANCODE_V];
}
