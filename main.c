#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>

#define WINDOW_X (3840)
#define WINDOW_Y (2160)
#define SPRITE_ORIENTATIONS (72)
#define MAX_FILE_PATH (1024)

#define MAX_PARTICLES (128)
#define PARTICLE_SPRITE_SIZE (5)

#define PARTICLE_RENDER_SIZE_MIN (20)
#define PARTICLE_RENDER_SIZE_MAX (30)

#define MASS_MIN (1)
#define MASS_MAX (20)

#define GRAVITATIONAL_CONSTANT (7.0)

float degsin(float deg) {return 57.2957795*sin(deg*0.0174532925);}
float degcos(float deg) {return 57.2957795*cos(deg*0.0174532925);}
float degtan(float deg) {return 57.2957795*tan(deg*0.0174532925);}
float degasin(float deg) {return 57.2957795*asin(deg*0.0174532925);}
float degacos(float deg) {return 57.2957795*acos(deg*0.0174532925);}
float degatan(float deg) {return 57.2957795*atan(deg*0.0174532925);}


void getPathToExecutable(char* buf, int bufLen)
{
	readlink("/proc/self/exe", buf, bufLen); //Linux.
	//GetModuleFileName(NULL, buf, bufLen) //Windows?

	for(int i = bufLen - 1; i >= 0; i--)
	{
		if(buf[i] == '/')
		{
			break;
		}
		buf[i] = 0;
	}
}

float randFloat(float min, float max) { return min + ((float)rand()/RAND_MAX)*(max-min); };

struct Vector2
{
	float x;
	float y;
};

float v2Distance(struct Vector2 a, struct Vector2 b)
{
	return sqrt(pow(abs(a.x - b.x), 2) + pow(abs(a.y - b.y), 2));
}

struct Vector2 v2Normalized(struct Vector2 v)
{
	struct Vector2 result;
	result.x = 0;
	result.y = 0;
	float d = v2Distance(result, v);
	if(d != 0)
	{
		result.x = v.x / d;
		result.y = v.y / d;
	}
	return result;
}

struct Vector2 v2subv2(struct Vector2 a, struct Vector2 b)
{
	struct Vector2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

struct Vector2 v2addv2(struct Vector2 a, struct Vector2 b)
{
	struct Vector2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

struct Vector2 v2byf(struct Vector2 v, float f)
{
	struct Vector2 result;
	result.x = v.x * f;
	result.y = v.y * f;
	return result;
}

struct Particle
{
	struct Vector2 pos;
	struct Vector2 vel;
	
	float mass;
	float heat;
};

int renderSize(float mass)
{
	return PARTICLE_RENDER_SIZE_MIN + (PARTICLE_RENDER_SIZE_MAX - PARTICLE_RENDER_SIZE_MIN) * ((mass - MASS_MIN) / (MASS_MAX - MASS_MIN));
}

int main()
{
	srand(time(NULL));

	char pathToExecutable[MAX_FILE_PATH];
	memset(pathToExecutable, 0, MAX_FILE_PATH);
	getPathToExecutable(pathToExecutable, MAX_FILE_PATH-1);
	char path[MAX_FILE_PATH];
	memset(path, 0, MAX_FILE_PATH);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("My SDL2 Window",
		                                  SDL_WINDOWPOS_UNDEFINED,
		                                  SDL_WINDOWPOS_UNDEFINED,
		                                  WINDOW_X, WINDOW_Y,
		                                  SDL_WINDOW_SHOWN);
		                                  
	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		                                
	if (!window) {
		printf("Failed to create SDL window: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED || SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		printf("Failed to create SDL renderer: %s\n", SDL_GetError());
		return 1;
	}

	int FrameRate = 60;
	int ClocksPerFrame = CLOCKS_PER_SEC / FrameRate;
	int frame = 0;
	
	//load texture
	memset(path, 0, MAX_FILE_PATH);
	strcat(path, pathToExecutable);
	strcat(path, "particle.png");
	SDL_Surface *particleImage = IMG_Load(path);
	SDL_Texture *particleTexture = SDL_CreateTextureFromSurface(renderer, particleImage);

	struct Particle particles[MAX_PARTICLES];

	for(int i = 0; i < MAX_PARTICLES; i++)
	{
		particles[i].pos.x = rand() % (WINDOW_X - PARTICLE_RENDER_SIZE_MAX);
		particles[i].pos.y = rand() % (WINDOW_X - PARTICLE_RENDER_SIZE_MAX);

		particles[i].vel.x = 0; //randFloat(-1, 1);
		particles[i].vel.y = 0; //randFloat(-1, 1);

		particles[i].heat = 1;
		particles[i].mass = randFloat(MASS_MIN, MASS_MAX);
	}
	
	while(1)
	{
		clock_t FrameStartClock = clock();
		SDL_SetRenderDrawColor(renderer, 153, 138, 78, 255);
		SDL_RenderClear(renderer); //erase

		SDL_Texture *texture;

		//Particle Physics
		for(int i = 0; i < MAX_PARTICLES; i++)
		{
			//Since each particle affects each others, we can start at i+1 and do all interactions both ways.
			for(int i2 = i+1; i2 < MAX_PARTICLES; i2++)
			{
				//Gravity
				//F = (G * m1 * m2) / d^2
				float gravitationalForce = (GRAVITATIONAL_CONSTANT * particles[i].mass * particles[i2].mass) / pow(v2Distance(particles[i].pos, particles[i2].pos), 2);
				if(gravitationalForce != INFINITY) //if the particles are on top of each other this will be infinite. That is no gud :(
				{
					particles[i].vel = v2addv2(
						particles[i].vel,
						v2byf(v2Normalized(v2subv2(particles[i2].pos, particles[i].pos)), gravitationalForce/particles[i].mass)
					);
					particles[i2].vel = v2addv2(
						particles[i2].vel,
						v2byf(v2Normalized(v2subv2(particles[i].pos, particles[i2].pos)), gravitationalForce/particles[i2].mass)
					);
				}

				//Repulsion when close
				//float repulsiveForce = -pow(1.1, -v2Distance(particles[i].pos, particles[i2].pos)) * 10 * MASS_MAX;
				float repulsiveForce = -20 / pow(-v2Distance(particles[i].pos, particles[i2].pos), 3) * MASS_MAX;
				if(repulsiveForce != INFINITY)
				{
					particles[i].vel = v2addv2(
						particles[i].vel,
						v2byf(v2Normalized(v2subv2(particles[i2].pos, particles[i].pos)), repulsiveForce/particles[i].mass)
					);
					particles[i2].vel = v2addv2(
						particles[i2].vel,
						v2byf(v2Normalized(v2subv2(particles[i].pos, particles[i2].pos)), repulsiveForce/particles[i2].mass)
					);
				}

			}

			//Movement
			particles[i].pos = v2addv2(particles[i].pos, particles[i].vel);

			//Bounds
			if(particles[i].pos.x < 0)
			{
				particles[i].pos.x = 0;
				particles[i].vel.x = 0;
			}
			if(particles[i].pos.x > WINDOW_X - renderSize(particles[i].mass))
			{
				particles[i].pos.x = WINDOW_X - renderSize(particles[i].mass);
				particles[i].vel.x = 0;
			}
			if(particles[i].pos.y < 0)
			{
				particles[i].pos.y = 0;
				particles[i].vel.y = 0;
			}
			if(particles[i].pos.y > WINDOW_Y - renderSize(particles[i].mass))
			{
				particles[i].pos.y = WINDOW_Y - renderSize(particles[i].mass);
				particles[i].vel.y = 0;
			}
		}

		//Render particles
		for(int i = 0; i < MAX_PARTICLES; i++)
		{
			SDL_Surface *img = SDL_CreateRGBSurface(0, PARTICLE_SPRITE_SIZE, PARTICLE_SPRITE_SIZE, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
			SDL_Rect srcrect = { 0, 0, PARTICLE_SPRITE_SIZE, PARTICLE_SPRITE_SIZE };
			SDL_Rect dstrect = { 0, 0, 0, 0 };
			SDL_BlitSurface(particleImage, &srcrect, img, &dstrect);
			texture = SDL_CreateTextureFromSurface(renderer, img);
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
			dstrect.x = particles[i].pos.x;
			dstrect.y = particles[i].pos.y;
			dstrect.w = renderSize(particles[i].mass);
			dstrect.h = renderSize(particles[i].mass);
			SDL_RenderCopy(renderer, texture, NULL, &dstrect);
		}

		SDL_RenderPresent(renderer);

		SDL_Event event;
		int quit = 0;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
				    quit = 1;
				    break;
				//Keeping these for future use :)
				/*
				case SDL_KEYDOWN:
					if(!strcmp(SDL_GetKeyName(event.key.keysym.sym), "A"))
					{
						//pressed A
					}
					else if(!strcmp(SDL_GetKeyName(event.key.keysym.sym), "D"))
					{
						//pressed D
					}
					break;
				case SDL_KEYUP:
					if(!strcmp(SDL_GetKeyName(event.key.keysym.sym), "A"))
					{
						//released A
					}
					if(!strcmp(SDL_GetKeyName(event.key.keysym.sym), "D"))
					{
						//released D
					}
				    break;
					*/
			}
			
			if(quit)
			{
				break;
			}
		}
		
		if(quit)
		{
			break;
		}
		
		//printf("frame: %i\n", frame);
		
		//Sleep until we have taken up enough time.
		int ClocksThisFrame = clock()-FrameStartClock;
		int ClocksToSleep = ClocksPerFrame - ClocksThisFrame;
		if(ClocksToSleep > 0)
		{
			usleep((int)(((double)ClocksToSleep) / ClocksPerFrame / FrameRate * 1000000));
		}
		else
		{
			printf("LAG FRAME!\n");
		}
		
		frame++;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
