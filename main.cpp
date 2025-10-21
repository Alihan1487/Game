#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <emscripten.h>


SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* txt;
SDL_Rect rect={100,100,100,100};
bool running = true;
int cur=0;
int last=0;
std::vector<SDL_Rect> walls={SDL_Rect{600,500,100,100}};

extern "C" void load(){
    int a;
    std::ifstream file("/save/data.bin",std::ios::binary);
    file.read(reinterpret_cast<char*>(&a),sizeof(int));
    file.close();
    EM_ASM(
        {console.log($0);},a
    );
    a+=1;
    std::ofstream afile("/save/data.bin",std::ios::binary);
    afile.write(reinterpret_cast<char*>(&a),sizeof(int));
    EM_ASM(
            FS.syncfs(false,function (err){});
    );
    afile.close();
}

void loop() {
    cur=SDL_GetTicks();
    float delta=(cur-last)/1000.f;
    last=cur;
    SDL_Event e;
    int speed=400;
    const Uint8* k=SDL_GetKeyboardState(NULL);
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            running = false;
    }
    if (!running)
        emscripten_cancel_main_loop();
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer,&rect);
    for (auto& i:walls){
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer,&i);
    }
    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("SDL + Emscripten", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    // Важно! Вместо while используем Emscripten loop:
    EM_ASM(
            FS.mkdir("/save");
            FS.mount(IDBFS,{},"/save");
            FS.syncfs(true,function (err){_load()});
    );
    emscripten_set_main_loop(loop, 0, 1);
    return 0;
}