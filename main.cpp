#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <emscripten.h>


SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* txt;
TTF_Font* arial;
SDL_Texture* safetxt;
SDL_Rect rect;
bool running = true;
int cur=0;
int last=0;
int strt=0;
int alpha=0;
std::vector<SDL_Rect> walls={SDL_Rect{600,500,150,150}};

void move(SDL_Rect* rect, int targetX, int targetY, float speed, float delta) {
    float dx = targetX - rect->x;
    float dy = targetY - rect->y;
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    if (dist < 1.0f) return; 

    dx /= dist;
    dy /= dist;

    rect->x += dx * speed * delta;
    rect->y += dy * speed * delta;
}


extern "C" void load(){
    int x=0,y=0;
    std::ifstream file("/save/load.bin",std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Save not found";
        rect.x = 100;
        rect.y = 100;
        return;
    }
    file.read(reinterpret_cast<char*>(&x),sizeof(int));
    file.read(reinterpret_cast<char*>(&y),sizeof(int));
    EM_ASM(
        {console.log($0,$1);},x,y
    );
    file.close();
    rect.x=x;
    rect.y=y;
}

extern "C" void save(){
    std::ofstream file("/save/load.bin",std::ios::binary);
    file.write(reinterpret_cast<char*>(&rect.x),sizeof(int));
    file.write(reinterpret_cast<char*>(&rect.y),sizeof(int));
    file.close();
    EM_ASM(
        FS.syncfs(false,function (err){});
        console.log("saved");
    );
    alpha=255;
}

void loop() {
    cur=SDL_GetTicks();
    float delta=(cur-last)/1000.f;
    last=cur;
    SDL_Event e;
    int speed=400;
    const Uint8* k=SDL_GetKeyboardState(NULL);
    if (alpha>0)
    alpha-=100*delta;
    if (alpha<0)
    alpha=0;
    SDL_SetTextureAlphaMod(safetxt,alpha);
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            running = false;
        if (e.type == SDL_KEYDOWN)
        if (e.key.keysym.sym == SDLK_s)
        if (e.key.keysym.mod & KMOD_SHIFT)
        save();
    }
    if (!running)
        emscripten_cancel_main_loop();
    if (k[SDL_SCANCODE_W]){
        rect.y-=speed*delta;
        for (auto& i:walls)
            if (SDL_HasIntersection(&i,&rect))
                rect.y=i.y+i.h;
    }
    if (k[SDL_SCANCODE_S]){
        rect.y+=speed*delta;
        for (auto& i:walls)
            if (SDL_HasIntersection(&i,&rect))
                rect.y=i.y-rect.h;
    }
    if (k[SDL_SCANCODE_A]){
        rect.x-=speed*delta;
        for (auto& i:walls)
            if (SDL_HasIntersection(&i,&rect))
                rect.x=i.x+i.w;
    }
    if (k[SDL_SCANCODE_D]){
        rect.x+=speed*delta;
        for (auto& i:walls)
            if (SDL_HasIntersection(&i,&rect))
                rect.x=i.x-rect.w;
    }
    if ((cur-strt>30000)){
        save();
        strt=cur;
    }
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer,&rect);
    for (auto& i:walls){
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer,&i);
    }
    {
        SDL_Rect recto{0,0,100,50};
        SDL_RenderCopy(renderer,safetxt,nullptr,&recto);
    }
    SDL_RenderPresent(renderer);
}
int main() {
    rect.w=50;
    rect.h=50;
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();


    window = SDL_CreateWindow("SDL + Emscripten", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);


    arial=TTF_OpenFont("assets/arialmt.ttf",20);
    if (!arial){
        std::cerr<<"FAILED TO LOAD ARIAL";
        return 1;
    }
    SDL_Surface* surff=TTF_RenderText_Solid(arial,"saved",SDL_Color{255,255,255,255});
    safetxt=SDL_CreateTextureFromSurface(renderer,surff);


    EM_ASM(
    window.addEventListener("beforeunload", () => {
        ccall("_save", null, [], []);
    });
);
    EM_ASM(
            FS.mkdir("/save");
            FS.mount(IDBFS,{},"/save");
            FS.syncfs(true,function (err){
                if (err){
                console.error("error",err)
                }
                else{
                    console.log("loading");
                    _load();
                }
            });
    );
    emscripten_set_main_loop(loop, 0, 1);
    return 0;
}