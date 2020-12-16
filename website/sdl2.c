//
// proof of concept implementation for BeepBox synthesizer in SDL games
//
// This is a minimum-effort demo, the code isn't well written.
//
// I've cleaned it up since my first try, and it seems to work better, but
// there's still a lack of error checking and it's based on guessing how
// things work in QuickJS, not being sure.
//
// BeepBox is a great little music synthesizer, there are lots of things you
// can do in a game by having access to the synthesizer, and not just mp3s
// of songs you made with it. I did this mainly so I'd have confidence that I
// could port web games using BeepBox to native games without a crazy amount
// of work or overhead.
//
// built with: g++ -g -o sdl2 sdl2.c -lSDL2 -L/usr/local/lib/quickjs -lquickjs 
// tested (built and run) on Ubuntu 18.04
//
// you need to run it in the same directory with "beepbox_synthbase.js"
// which is a modified "beepbox_synth.js" that strips out browser-specific stuff
//
// press "Escape" to quit
// (there's unnecessary graphics/controls from a test program I used as a base)
//
// needs SDL https://libsdl.org/
// and QuickJS from https://bellard.org/quickjs/
// (installed unmodified with "make install")
//
//
// by Darrell Johnson 
//
// @darrellprograms on Twitter
// https://github.com/funchucks/
//
// my work on this file is released in the public domain
// or MIT license where that is not permitted
//
// I did not write the song, it's borrowed from the BeepBox demo.

#include <SDL2/SDL.h>
#include <stdio.h>
#include <quickjs/quickjs.h>

int quit=0;

/*
struct guy{
	int x
};
guy the_guy;
*/

struct painter{
	SDL_Texture *this_texture;
	Uint32 p[320*240];
	Uint32 rr, gg, bb, aa;
	Uint32 *rgb[3];
	int i;
	int x, y;
	int centermode;
	void init(SDL_Renderer *r){
		centermode=0;
		rr=255; gg=255; bb=255; aa=255;
		rgb[0]=&rr; rgb[1]=&gg; rgb[2]=&bb;
		i=0;
		x=20; y=20;
		this_texture=SDL_CreateTexture(r,
		 SDL_PIXELFORMAT_ARGB8888,
		 SDL_TEXTUREACCESS_STREAMING,
		 320,240);
		for(int j=0; j<320*240; j++){
			p[j]=0;
		}
		/*
		this_surface=
		SDL_CreateRGBSurface(
		 0, //flags
		 320, 240, 32,
		 0x000000FF, //little-endian byte order
		 0x0000FF00,
		 0x00FF0000,
		 0xFF000000
		 );
		*/
	}
	void paint_pixels(){
		//paint background
		if(i>=3)i=0;
		if(i<0)i=2;
		rr=255; gg=255; bb=255; aa=255;
		*rgb[i]=0;
		Uint32 backfill=(aa<<24)|(rr<<16)|(gg<<8)|bb;
		for(int j=0; j<320*240; j++){
			p[j]=backfill;
		}
		Uint32 rectfill=255<<24;
		for(int yy=0; yy<32; yy++){
			if(y+yy>=0 && y+yy<240){
				for(int xx=0; xx<96; xx++){
					if(x+xx>=0 && x+xx<320) {
						p[(y+yy)*320+x+xx]=rectfill;
					}
				}
			}
		}
	}
	void paint(SDL_Window *w, SDL_Renderer *r){
		/*
		SDL_SetRenderDrawColor(r,rr,gg,bb,aa); //renderer brush color
		rr=255; gg=255; bb=255; aa=255;
		*rgb[i]=0;
		*/
		SDL_SetRenderDrawColor(r,0,0,0,255); //renderer brush color
		SDL_RenderClear(r);
		paint_pixels();
		SDL_UpdateTexture(this_texture, 0, p,
		 320*sizeof (Uint32)); //last argument is pitch, not buffer size
		int width, height, sw,sh, xoff=0, yoff=0;
		SDL_GetWindowSize(w, &width, &height);
		sw=width; sh=height;
		if(width*240>height*320){
			sw=height*320/240;
			if(centermode>0){
				xoff=width-sw;
			}else if(centermode<0){
				xoff=0;
			}else{
				xoff=(width-sw)/2;
			}
		}else if(width*240<height*320){
			sh=width*240/320;
			yoff=(height-sh)/2;
		}
		SDL_Rect rect={xoff,yoff,sw,sh};
		SDL_RenderCopy(r, this_texture, 0, &rect);
		/*
		SDL_SetRenderDrawColor(r,0,0,0,aa);
		SDL_Rect rect={x,y,32,96}; //x,y,w,h
		SDL_RenderFillRect(r, &rect);
		*/
		SDL_RenderPresent(r); //"present" (paint) the rendered frame
	}
};
painter the_painter;

struct controller{
	SDL_GameController *controllers[16];
	int count;
	void init(){
		int i;
		for(i=0; i<16; i++){
			controllers[i]=0;
		}
		int num=SDL_NumJoysticks();
		printf("%d\n", num);
		count=0;
		for(i=0; i<num; i++){
			controllers[count]= SDL_GameControllerOpen(i);
			if(controllers[count]){
				count+=1;
			}
		}
	}
	int getbutton(SDL_GameControllerButton b){
		for(int i=0; i<count; i++){
			if(SDL_GameControllerGetButton(controllers[i], b)){
				return 1;
			}
		}
		return 0;
	}
	int xaxis(){
		int ret=0;
		ret-=getbutton(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		ret+=getbutton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		return  ret;
	}
	int yaxis(){
		int ret=0;
		ret-=getbutton(SDL_CONTROLLER_BUTTON_DPAD_UP);
		ret+=getbutton(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		return  ret;
	}
	int x(){
		return getbutton(SDL_CONTROLLER_BUTTON_X);
	}
	int y(){
		return getbutton(SDL_CONTROLLER_BUTTON_Y);
	}
	int a(){
		return getbutton(SDL_CONTROLLER_BUTTON_A);
	}
	int b(){
		return getbutton(SDL_CONTROLLER_BUTTON_B);
	}
};
controller the_controller;

struct ticker{
	int t;
	Uint32 last_tick;
	Uint32 time_error;
	int tries, total_ticks;
	int tick_set;
	void init(){
		t=0;
		last_tick=SDL_GetTicks();
		time_error=0;
		tries=0; total_ticks=0;
		tick_set=0;
	}
	void tick(){
		/*
		t+=1;
		if(t>=30){
			the_painter.i+=1;
			t=0;
		}*/
		int guy_speed=5;
		the_painter.x+=the_controller.xaxis()*guy_speed;
		the_painter.y+=the_controller.yaxis()*guy_speed;
		if(the_painter.x<0) the_painter.x=0;
		if(the_painter.y<0) the_painter.y=0;
		if(the_painter.x>=320-96) the_painter.x=320-96;
		if(the_painter.y>=240-32) the_painter.y=240-32;
		int a=the_controller.a(), b=the_controller.b();
		t+=1;
		if(t > 10  && (a || b)){
			the_painter.i+=a;
			the_painter.i-=b;
			t=0;
		}
	}
	int try_tick(){
		Uint32 cur_tick=SDL_GetTicks();
		int n=0;
		tries+=1;
		/*
		if(tries>1){
			tick();
			tries=0;
		}
		*/
		if(!tick_set){
			last_tick=cur_tick;
			time_error=0;
			tick_set=1;
			tick();
			return 1;
		}
		if(cur_tick>last_tick && cur_tick-last_tick >= (1000/60)+5){
			do{
				tick();
				total_ticks+=1;
				//printf("%d ", tries);
				if(total_ticks%30==0){
					//printf("\n");
				}
				tries=0;
				last_tick+=1000/30;
				time_error+=1000000000/30-(1000/30)*1000000;
				while(time_error>1000000){
					last_tick+=1;
					time_error-=1000000;
				}
				n+=1;
				if(n>2){
					last_tick=cur_tick;
					time_error=0;
				}
			}while(cur_tick > last_tick && cur_tick-last_tick >= (1000/30));
			return 1;
		}
		return 0;
	}
};

ticker the_ticker;

void handle_keypress(SDL_Keycode k){
	switch(k){
		case SDLK_ESCAPE:
			quit=1;
			break;
		case SDLK_SPACE:
			the_painter.i+=1;
			break;
		default:
			return;
	}
}

void handle_event(SDL_Event &e){
	if(e.type == SDL_QUIT){
		quit=1;
	}
	if(e.type == SDL_KEYDOWN){
		handle_keypress(e.key.keysym.sym);
	}
}

void load_js_file(JSContext *jsc, const char *fname){
	FILE *fin=fopen(fname,"r");
	fseek(fin,0,SEEK_END);
	int flen=ftell(fin);
	rewind(fin);
	char *buf=(char *)malloc(flen);
	fread(buf,1,flen,fin);
	fclose(fin);
	JSValue jsval;
	JS_Eval(jsc, buf, flen, fname, 0);
	JS_FreeValue(jsc,jsval);
	free(buf);
}

void do_js(JSContext *jsc, const char *code){
	JSValue jsval=JS_Eval(jsc, code, strlen(code), "<inline>", 0);
	JS_FreeValue(jsc,jsval);
}

void print_js(JSContext *jsc, const char *code){
	JSValue jsval=JS_Eval(jsc, code, strlen(code), "<inline>", 0);
	const char *cstr=JS_ToCString(jsc,jsval);
	printf("%s\n",cstr);
	JS_FreeCString(jsc, cstr);
	JS_FreeValue(jsc,jsval);
}

float float_js(JSContext *jsc, const char *code){
	JSValue jsval=JS_Eval(jsc, code, strlen(code), "<inline>", 0);
	double dret=0;
	JS_ToFloat64(jsc, &dret, jsval);
	JS_FreeValue(jsc,jsval);
	return (float)dret;
}

struct beepbox_data{
	JSRuntime *jsr;
	JSContext *jsc;
	int initialized;
	const char *song;
	int buflen;
};

//this is a lot less dumb
//still lacks error checking, but it seems to be a lot better

void beepbox_callback(void *userdata, Uint8 *stream, int len){
	char buffer[4096];
	beepbox_data *data=(beepbox_data *)userdata;
	int sample_count=len/(2*sizeof(float));
	float *output=(float *)stream;
	JSContext *jsc=data->jsc;
	if(!data->initialized){
		data->jsr=JS_NewRuntime();
		jsc=data->jsc=JS_NewContext(data->jsr);
		load_js_file(jsc, "beepbox_synthbase.js");
		sprintf(buffer,"var synth=new beepbox.SynthBase(\"%s\");",data->song);
		do_js(jsc, buffer);
		do_js(jsc,"var outL=null, outR=null;");
		do_js(jsc,"var outlen=0;");
		do_js(jsc,"synth.samplesPerSecond=44100;");
		do_js(jsc,"synth.play();");
		data->buflen=0;
		data->initialized=1;
	}
	if(data->buflen<sample_count){
		sprintf(buffer, "outL=new Float32Array(%d); outR=new Float32Array(%d); outlen=%d;", sample_count, sample_count, sample_count);
		do_js(jsc,buffer);
	}
	do_js(jsc,"synth.synthesize(outL, outR, outlen, true);");
	size_t byte_offsetL, byte_offsetR, byte_length, bytes_per_element;
	JSValue tempval, outL, outR;
	const char *tempcode="outL;";
	tempval=JS_Eval(jsc, tempcode, strlen(tempcode), "<inline>", 0);
	outL=JS_GetTypedArrayBuffer(jsc,tempval,
	 &byte_offsetL, &byte_length, &bytes_per_element);
	JS_FreeValue(jsc, tempval);
	tempcode="outR;";
	tempval=JS_Eval(jsc, tempcode, strlen(tempcode), "<inline>", 0);
	outR=JS_GetTypedArrayBuffer(jsc,tempval,
	 &byte_offsetR, &byte_length, &bytes_per_element);
	JS_FreeValue(jsc, tempval);
	unsigned char *boutL=(unsigned char*)JS_GetArrayBuffer(jsc, &byte_length, outL);
	unsigned char *boutR=(unsigned char*)JS_GetArrayBuffer(jsc, &byte_length, outR);
	JS_FreeValue(jsc, outL);
	JS_FreeValue(jsc, outR);
	//
	// I still don't understand QuickJS.
	// It's not well documented, and the code's hard to read.
	// Should I be adding these byte offsets in this next step or not?
	// Since they keep turning out to be zero, I can't test.
	//
	// Maybe they're always zero except when it's a dataview.
	//
	float *foutL=(float*)(boutL+byte_offsetL);
	float *foutR=(float*)(boutR+byte_offsetR);
	for(int i=0; i<sample_count; i++){
		output[i*2+0]=foutL[i];
		output[i*2+1]=foutR[i];
	}
}

int main(int argc, char **argv){
	//js_State *mujs=js_newstate(NULL, NULL, JS_STRICT);
	//js_dostring(mujs, "var x=Math.PI;");
	//js_dofile(mujs,"beepbox_synthbase.es5.js");
	beepbox_data the_beepbox_data;
	the_beepbox_data.song="5sbk4l00e0ftaa7g0fj7i0r1w1100f0000d1110c0000h0000v2200o3320b4z8Ql6hkpUsiczhkp5hDxN8Od5hAl6u74z8Ql6hkpUsp24ZFzzQ1E39kxIceEtoV8s66138l1S0L1u2139l1H39McyaeOgKA0TxAU213jj0NM4x8i0o0c86ywz7keUtVxQk1E3hi6OEcB8Atl0q0Qmm6eCexg6wd50oczkhO8VcsEeAc26gG3E1q2U406hG3i6jw94ksf8i5Uo0dZY26kHHzxp2gAgM0o4d516ej7uegceGwd0q84czm6yj8Xa0Q1EIIctcvq0Q1EE3ihE8W1OgV8s46Icxk7o24110w0OdgqMOk392OEWhS1ANQQ4toUctBpzRxx1M0WNSk1I3ANMEXwS3I79xSzJ7q6QtEXgw0";
	the_beepbox_data.initialized=0;
	/*
	beepbox_synthesize(jsc, 4096);
	char buffer[256];
	int sample_count=4096;
	for(int i=0; i<sample_count; i++){
		sprintf(buffer,"outL[%d];",i);
		audiobuf[i*2+0]=float_js(jsc,buffer);
		sprintf(buffer,"outR[%d];",i);
		audiobuf[i*2+1]=float_js(jsc,buffer);
	}
	//print_js(jsc, "synth.isPlayingSong ? 'playing' : 'not playing';");
	//print_js(jsc, "beepbox ? 'synth' : 'no synth';");
	//do_js(jsc, "outL[5200]=0.02;");
	printf("%f\n",float_js(jsc, "outR[200];"));
	print_js(jsc, "outR[0];");
	print_js(jsc, "outR[200];");
	print_js(jsc, "outR[201];");
	print_js(jsc, "outR[202];");
	do_js(jsc,"var a=10+10;");
	print_js(jsc, "a;");
	print_js(jsc, "outR[0];");
	*/
	int errcode=SDL_Init(
	 SDL_INIT_VIDEO
	 | SDL_INIT_GAMECONTROLLER
	 | SDL_INIT_AUDIO
	 );
	if(errcode!=0){
		return errcode;
	}
	int isfull=0;
	SDL_Window *w=SDL_CreateWindow(
	 "lrnsdl", //window caption
	 SDL_WINDOWPOS_UNDEFINED, //xpos
	 SDL_WINDOWPOS_UNDEFINED, //ypos
	 640, //width
	 480, //height
	 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE //flags
	 );
	SDL_Renderer *r=0;
	if(!w){
		//skipping: handle the error
		goto error;
	}
	SDL_SetWindowMinimumSize(w,320,240);
	r=SDL_CreateRenderer(
	 w, //window
	 -1, //index of rendering driver (-1 means "just pick one that works")
	 //flags
	 SDL_RENDERER_ACCELERATED | //use graphics accelerator hardware
	 SDL_RENDERER_PRESENTVSYNC //"syncronize present with vsync"
	 //"present" means SDL_RenderPresent, a function to paint to the screen
	 );
	if(!r){
		//skipping: handle the error
		goto error;
	}
	SDL_ShowCursor(SDL_DISABLE);
	the_controller.init();
	the_ticker.init();
	the_painter.init(r);
	the_painter.paint(w,r);
	//
	//audio
	//
	SDL_AudioSpec want,have;
	SDL_memset(&want, 0, sizeof(want));
	want.userdata=(void *)&the_beepbox_data;
	want.callback=&beepbox_callback;
	want.format=AUDIO_F32;
	want.channels=2;
	want.samples=4096; // glitches on 4096, because the callback is slow
	want.freq=44100;
	SDL_AudioDeviceID dev;
	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	SDL_PauseAudioDevice(dev, 0);
	//
	//end audio
	//
	SDL_Event e;
	while(!quit){
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_KEYDOWN && e.key.keysym.mod&KMOD_ALT){
				switch(e.key.keysym.sym){
				case SDLK_RETURN:
					SDL_SetWindowFullscreen(w,
					 isfull ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
					isfull=!isfull;
				break;
				case SDLK_RIGHT:
					the_painter.centermode=1;
				break;
				case SDLK_LEFT:
					the_painter.centermode=-1;
				break;
				case SDLK_DOWN:
					the_painter.centermode=0;
				break;
				}
			}else{
				handle_event(e);
			}
		}
		if(the_ticker.try_tick()){
			the_painter.paint(w,r);
		}else{
			SDL_RenderPresent(r);
		}
	}
	//standard exit
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(w);
	SDL_Quit();
	//JS_FreeContext(jsc);
	//JS_FreeRuntime(jsr);
	return 0;
	error:
	SDL_Quit();
	//JS_FreeContext(jsc);
	//JS_FreeRuntime(jsr);
	return 1;
}
