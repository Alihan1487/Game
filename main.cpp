#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <tuple>
#include <emscripten.h>

class Weapon;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* txt;
TTF_Font* arial;
SDL_Texture* safetxt;
SDL_Texture* playertxt;
SDL_Texture* walltxt;
SDL_Texture* bgtxt;
Mix_Chunk* judgment;
Mix_Chunk* reloadsound;
Mix_Music* bgmus;
float delta;
bool running = true;
int cur=0;
int last=0;
int strt=0;
int alpha=0;

void loop();
extern "C" void save();

template <typename T>
std::vector<T> vremove(std::vector<T> vector,int index){
    std::vector<T> real;
    for (int i=0;i<vector.size();i++)
    if (i!=index)
    real.push_back(vector[i]);
    return real;
}
template <typename T>
std::vector<T> vremove(std::vector<T> vector,T element){
    std::vector<T> real;
    for (int i=0;i<vector.size();i++)
    if (vector[i]!=element)
    real.push_back(vector[i]);
    return real;
}

void TakeAgreement(){
    SDL_Event ev;
    while (SDL_PollEvent(&ev)){
        if (ev.type==SDL_MOUSEBUTTONDOWN)
        emscripten_cancel_main_loop();
        emscripten_set_main_loop(loop,0,1);
    }
}

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

class Wall;

class Weapon{
    public:
    int inmag;
    int speed;
    float current_cooldown;
    int cooldown;
    size_t mag_size;
    int ammos;
    inline static std::vector<std::tuple<SDL_Rect,std::tuple<int,int>,int>> bullets;
    static void update_all(SDL_Renderer* rend,std::vector<Wall*> walls);
    virtual void reload(){};
    virtual void shoot(SDL_Rect who,int x,int y){};
};



class Pistol : public Weapon{
    public:
    Pistol(int ammos){
        current_cooldown=0;
        cooldown=500;
        mag_size=10;
        this->ammos=ammos;
        speed=1000;
        inmag=mag_size;
    }
    void reload() override{
        current_cooldown=2000;
        Mix_PlayChannel(-1,reloadsound,1);
        std::cout<<"COOLDOWN\n";
        inmag=mag_size;
    }
    void shoot(SDL_Rect who,int x,int y) override{
        SDL_Rect shrect{who.x,who.y,10,10};
        if (ammos<=0 || current_cooldown>0)
        return;
        if (inmag==0){
            reload();
        }
        ammos-=1;
        inmag-=1;
        std::cout<<ammos<<std::endl;
        auto i=std::make_tuple(shrect,std::make_tuple(x,y),speed);
        bullets.push_back(i);
        Mix_PlayChannel(-1,judgment,0);
        current_cooldown+=cooldown;
    }
};

class Sprite{
    public:
    SDL_Rect rect;
    virtual void update(){};
    virtual void evupdate(SDL_Event ev){};
};

namespace MainS{
    std::vector<Sprite*> sprites;
}

class Wall : public Sprite{
    public:
    Wall(SDL_Rect r){
        rect=r;
        MainS::sprites.push_back(this);
    }
    void update() override{
        SDL_RenderCopy(renderer,walltxt,nullptr,&rect);
    }
};

namespace MainS{
    std::vector<Wall*> walls;
}

void Weapon::update_all(SDL_Renderer* rend,std::vector<Wall*> walls){
    std::vector<std::tuple<SDL_Rect,std::tuple<int,int>,int>> real;
    for (auto i:bullets){
        SDL_Rect r=std::get<0>(i);
        std::tuple<int,int> coords=std::get<1>(i);
        int speed=std::get<2>(i);
        SDL_Rect coordsrect{std::get<0>(coords)-5,std::get<1>(coords)-5,15,15};
        bool push=true;
        for (auto j:walls)
        if (SDL_HasIntersection(&j->rect,&r))
        push=false;
        if (SDL_HasIntersection(&r,&coordsrect))
        push=false;
        if (push)
        real.push_back(i);
    }
    bullets=real;
    for (int i=0;i<bullets.size();i++){
        move(&std::get<0>(bullets[i]),std::get<0>(std::get<1>(bullets[i])),std::get<1>(std::get<1>(bullets[i])),std::get<2>(bullets[i]),delta);
        SDL_SetRenderDrawColor(rend,255,0,0,255);
        SDL_RenderFillRect(rend,&std::get<0>(bullets[i]));
    }
}

class Enemy;
namespace MainS{
    std::vector<Enemy*> enemies;
}

class Player : public Sprite{
    public:
    int rx=0,ry=0;
    std::vector<Wall*> *walls;
    Weapon* player;
    Player(SDL_Rect r,std::vector<Wall*> *walls){
        rect=r;
        this->walls=walls;
    }
    Player(){
        rect={0,0,0,0};
    }
    void move(int dx,int dy){
        for (auto i:MainS::sprites){
            i->rect.x+=dx;
            i->rect.y+=dy;
        }
        rx+=dx;
        ry+=dy;
    }
    void setX(int x){
        if (x>rx)
            move(x-rx,0);
        else if (x<rx)
            move(x-rx,0);
        rx=x;
    }
    void setY(int y){
        if (y>ry)
            move(y-ry,0);
        else if (y<ry)
            move(y-ry,0);
        ry=y;
    }
    void update() override{
        int speed=400;
        const Uint8* k=SDL_GetKeyboardState(NULL);
        int mx,my;
        Uint32 mstate=SDL_GetMouseState(&mx,&my);
        if (k[SDL_SCANCODE_W]){
        rect.y-=speed*delta;
        for (auto i:*walls)
            if (SDL_HasIntersection(&i->rect,&rect))
                rect.y=i->rect.y+i->rect.h;
        }
        if (k[SDL_SCANCODE_S]){
            rect.y+=speed*delta;
            for (auto i:*walls)
                if (SDL_HasIntersection(&i->rect,&rect))
                    rect.y=i->rect.y-rect.h;
        }
        if (k[SDL_SCANCODE_A]){
            rect.x-=speed*delta;
            for (auto i:*walls)
                if (SDL_HasIntersection(&i->rect,&rect))
                    rect.x=i->rect.x+i->rect.w;
        }
        if (k[SDL_SCANCODE_D]){
            rect.x+=speed*delta;
            for (auto i:*walls)
                if (SDL_HasIntersection(&i->rect,&rect))
                    rect.x=i->rect.x-rect.w;
        }
        if (mstate & SDL_BUTTON_LMASK){
            player->shoot(rect,mx,my);
        }
        if (player->current_cooldown>0)
        player->current_cooldown-=delta*1000;
        if (player->current_cooldown<0)
        player->current_cooldown=0;
        SDL_RenderCopy(renderer,playertxt,nullptr,&rect);
    }
    void evupdate(SDL_Event e) override{
        if (e.type == SDL_KEYDOWN){
        if (e.key.keysym.sym == SDLK_e){
            if (e.key.keysym.mod & KMOD_SHIFT)
                save();
        }
        else if(e.key.keysym.sym==SDLK_r){
            player->reload();
        }
        else if(e.key.keysym.sym==SDLK_p){
            player->ammos=99;
        }
        }
    }
};

namespace MainS{
    Player me(SDL_Rect{0,0,0,0},&walls);
}

class Enemy : public Sprite{
    public:
    Player* p;
    Enemy(SDL_Rect r,Player* plr){
        MainS::enemies.push_back(this);
        MainS::sprites.push_back(this);
        rect=r;
        p=plr;
    }
    ~Enemy(){
        MainS::enemies=vremove<Enemy*>(MainS::enemies,this);
    }
    void update() override{
        move(&rect,p->rect.x,p->rect.y,300,delta);
        SDL_SetRenderDrawColor(renderer,0,0,255,255);
        SDL_RenderFillRect(renderer,&rect);
    }
};

namespace MainS{
    Enemy m(SDL_Rect{100,150,100,100},&me);
}

struct WeaponSave{
    int ammos;
    int inmag;
    void LoadToWeapon(Weapon* wep){
        wep->ammos=ammos;
        wep->inmag=inmag;
    }
};


extern "C"{ 
EMSCRIPTEN_KEEPALIVE
void load(){
    int x=0,y=0;
    std::ifstream file("/save/load.bin",std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Save not found\n";
        MainS::me.rect.x = 100;
        MainS::me.rect.y = 100;
        return;
    }
    WeaponSave sv{99,10};
    file.read(reinterpret_cast<char*>(&x),sizeof(int));
    file.read(reinterpret_cast<char*>(&y),sizeof(int));
    file.read(reinterpret_cast<char*>(&sv),sizeof(WeaponSave));
    std::cout<<sv.ammos<<" "<<sv.inmag<<std::endl;
    sv.LoadToWeapon(MainS::me.player);
    EM_ASM(
        {console.log($0,$1);},x,y
    );
    file.close();
    MainS::me.rect.x=x;
    MainS::me.rect.y=y;
}
}
extern "C" {
EMSCRIPTEN_KEEPALIVE
void save(){
    std::ofstream file("/save/load.bin",std::ios::binary);
    WeaponSave sv{MainS::me.player->ammos,MainS::me.player->inmag};
    file.write(reinterpret_cast<char*>(&MainS::me.rect.x),sizeof(int));
    file.write(reinterpret_cast<char*>(&MainS::me.rect.y),sizeof(int));
    file.write(reinterpret_cast<char*>(&sv),sizeof(WeaponSave));
    file.close();
    EM_ASM(
        FS.syncfs(false,function (err){});
        console.log("saved");
    );
    alpha=255;
}
}
void loop() {
    cur=SDL_GetTicks();
    delta=(cur-last)/1000.f;
    last=cur;
    SDL_Event e;
    if (alpha>0)
    alpha-=100*delta;
    if (alpha<0)
    alpha=0;
    SDL_SetTextureAlphaMod(safetxt,alpha);
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            running = false;
        MainS::me.evupdate(e);
    }
    if (!running)
        emscripten_cancel_main_loop();
    if ((cur-strt>30000)){
        save();
        strt=cur;
    }
    {
        int w,h;
        SDL_GetWindowSize(window,&w,&h);
        SDL_Rect win{0,0,w,h};
        SDL_RenderCopy(renderer,bgtxt,nullptr,&win);
    }
    for (auto i:MainS::walls)
        i->update();
    Weapon::update_all(renderer,MainS::walls);
    {
        SDL_Rect recto{0,0,100,50};
        SDL_RenderCopy(renderer,safetxt,nullptr,&recto);
    }
    MainS::me.update();
    MainS::m.update();
    SDL_RenderPresent(renderer);
}
int main() {

    MainS::me.player=new Pistol(99);

    Wall w(SDL_Rect{300,300,100,100});
    MainS::walls.push_back(&w);

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048);

    window = SDL_CreateWindow("SDL + Emscripten", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    MainS::me.rect.w=100;
    MainS::me.rect.h=100;

    SDL_Surface* s=IMG_Load("assets/plr.png");
    if (!s){
        std::cerr<<"DIDNT LOAD IMAGE PLAYER BECAUSE:\n"<<IMG_GetError()<<"\n";
        return 1;
    }
    playertxt=SDL_CreateTextureFromSurface(renderer,s);
    SDL_FreeSurface(s);

    SDL_Surface* su=IMG_Load("assets/wall.png");
    if (!su){
        std::cerr<<"DIDNT LOAD IMAGE WALL BECAUSE:\n"<<IMG_GetError()<<"\n";
        return 1;
    }
    walltxt=SDL_CreateTextureFromSurface(renderer,su);
    SDL_FreeSurface(su);

    SDL_Surface* sur=IMG_Load("assets/bg.png");
    if (!sur){
        std::cerr<<"DIDNT LOAD IMAGE WALL BECAUSE:\n"<<IMG_GetError()<<"\n";
        return 1;
    }
    bgtxt=SDL_CreateTextureFromSurface(renderer,sur);
    SDL_FreeSurface(sur);

    arial=TTF_OpenFont("assets/Arialmt.ttf",20);
    if (!arial){
        std::cerr<<"FAILED TO LOAD ARIAL BECAUSE:\n"<<TTF_GetError()<<std::endl;
        return 1;
    }
    SDL_Surface* surff=TTF_RenderText_Solid(arial,"saved",SDL_Color{255,255,255,255});
    safetxt=SDL_CreateTextureFromSurface(renderer,surff);
    SDL_FreeSurface(surff);

    judgment=Mix_LoadWAV("assets/shot.wav");
    if (!judgment){
        std::cerr<<"FAILED TO LOAD SHOT CAUSE:\n"<<Mix_GetError()<<std::endl;
        return 1;
    }

    reloadsound=Mix_LoadWAV("assets/reload.wav");
    if (!reloadsound){
        std::cerr<<"FAILED TO LOAD RELOAD CAUSE:\n"<<Mix_GetError()<<std::endl;
        return 1;
    }

    bgmus=Mix_LoadMUS("assets/versus.wav");
    if (!bgmus){
        std::cerr<<"FAILED TO LOAD VERSUS CAUSE:\n"<<Mix_GetError()<<std::endl;
        return 1;
    }

    EM_ASM(
    window.addEventListener("beforeunload", () => {
        ccall("save", null, [], []);
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
    Mix_PlayMusic(bgmus,-1);
    emscripten_set_main_loop(loop, 0, 1);
    return 0;
}
