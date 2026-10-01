// Original Metro-style title for Emulos360. No retail dashboard code/assets.
// XDK 21256 title APIs only; system actions are brokered by the PC host.
#include <xtl.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "vs.h"
#include "solid.h"
#include "textured.h"
#include "font_widths.h"

namespace {
struct Game { std::string name, id; };
struct Room { std::string name; int game; };
struct Snapshot {
    std::string nonce, profile, status;
    std::vector<Game> games;
    std::vector<Room> rooms;
};
Snapshot data;
IDirect3DDevice9* device = NULL;
IDirect3DVertexShader9* vertexShader = NULL;
IDirect3DPixelShader9 *solidShader = NULL, *textureShader = NULL;
IDirect3DVertexDeclaration9* declaration = NULL;
IDirect3DTexture9* font = NULL;
IDirect3DTexture9* covers[6] = {NULL};
int coverPage = -1, page = 0, selected = 0, gamePage = 0, roomPage = 0;
DWORD requestSerial = 0, requestTime = 0;
bool awaiting = false, guideOpen = false, shutdownConfirm = false;
DWORD guideSerial = 0;
const DWORD green = 0xff168500, white = 0xffffffff, ink = 0xff404040;
struct Vertex { float x,y; DWORD color; float u,v; };

bool Line(FILE* f, std::string& out, size_t maxLength) {
    out.clear(); int c;
    while ((c = fgetc(f)) != EOF && c != '\n') {
        if (c != '\r') out += static_cast<char>(c);
        if (out.size() > maxLength) return false;
    }
    return c != EOF;
}
bool Number(FILE* f, int& result, int maximum) {
    std::string line;
    if (!Line(f,line,10) || line.empty()) return false;
    char* end = NULL; long n = strtol(line.c_str(), &end, 10);
    if (*end || n < 0 || n > maximum) return false;
    result = static_cast<int>(n); return true;
}
void ReadSnapshot() {
    FILE* f = fopen("game:\\snapshot.txt", "rb");
    if (!f) return;
    Snapshot next; std::string magic; int count = 0;
    bool ok = Line(f,magic,16) && magic == "EMETRO1" &&
        Line(f,next.nonce,32) && next.nonce.size() == 32 &&
        Line(f,next.profile,64) && Line(f,next.status,160) && Number(f,count,1024);
    for (int i=0; ok && i<count; ++i) {
        Game g; ok = Line(f,g.name,100) && Line(f,g.id,8); next.games.push_back(g);
    }
    if (ok) ok = Number(f,count,256);
    for (int i=0; ok && i<count; ++i) {
        Room r; int index=0; ok=Line(f,r.name,100) && Number(f,index,1024);
        r.game=index-1; next.rooms.push_back(r);
    }
    fclose(f);
    if (ok) data = next;
}
void Command(int action, int argument = 0) {
    if (awaiting || data.nonce.empty()) return;
    FILE* f = fopen("game:\\request.txt", "wb");
    if (!f) return;
    ++requestSerial;
    fprintf(f,"EMETRO1\n%s\n%lu\n%d\n%d\nEND\n",data.nonce.c_str(),requestSerial,action,argument);
    if (fclose(f) == 0) { awaiting=true; requestTime=GetTickCount(); }
}
void Quad(float x,float y,float w,float h,DWORD color,IDirect3DTexture9* texture=NULL,
          float u0=0,float v0=0,float u1=1,float v1=1) {
    const float l=x/640.f-1.f, r=(x+w)/640.f-1.f, t=1.f-y/360.f,b=1.f-(y+h)/360.f;
    Vertex vertices[6]={{l,t,color,u0,v0},{r,t,color,u1,v0},{l,b,color,u0,v1},
                        {l,b,color,u0,v1},{r,t,color,u1,v0},{r,b,color,u1,v1}};
    device->SetPixelShader(texture ? textureShader : solidShader);
    device->SetTexture(0,texture);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,vertices,sizeof(Vertex));
}
void Text(float x,float y,const std::string& value,float size,DWORD color,float limit=1100) {
    const float scale=size/36.f, origin=x;
    for (size_t i=0;i<value.size();++i) {
        const unsigned char c=static_cast<unsigned char>(value[i]);
        if (c<32) continue;
        const unsigned int glyph=c-32;
        if (x-origin+kWidths[glyph]*scale>limit) break;
        // Keep linear sampling inside this glyph's padded atlas cell.
        const float u=(static_cast<float>(glyph%16)*64.f+2.f)/1024.f;
        const float v=(static_cast<float>(glyph/16)*64.f+2.f)/1024.f;
        Quad(x,y,64*scale,64*scale,color,font,u,v,u+60.f/1024.f,v+60.f/1024.f);
        x+=kWidths[glyph]*scale;
    }
}
void Gradient(float y,float h,DWORD top,DWORD bottom) {
    const float t=1.f-y/360.f,b=1.f-(y+h)/360.f;
    Vertex v[6]={{-1.f,t,top,0,0},{1.f,t,top,1,0},{-1.f,b,bottom,0,1},
                 {-1.f,b,bottom,0,1},{1.f,t,top,1,0},{1.f,b,bottom,1,1}};
    device->SetPixelShader(solidShader); device->SetTexture(0,NULL);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(Vertex));
}
void Tile(float x,float y,float w,float h,const char* label,const char* caption,int index,DWORD color) {
    if (selected==index) {
        Quad(x-5,y-5,w+10,h+10,white);
        Quad(x-2,y-2,w+4,h+4,0xff555555);
    }
    Quad(x,y,w,h,color);
    if(h<100) {
        Text(x+16,y+5,label,19,white,w-30);
        Text(x+16,y+36,caption,15,white,w-30);
    } else {
        Text(x+18,y+16,label,24,white,w-30);
        Text(x+22,y+h-48,caption,19,white,w-40);
    }
}
void LoadCovers(int targetPage) {
    if (coverPage==targetPage) return;
    coverPage=targetPage;
    for (int i=0;i<6;++i) {
        if (covers[i]) { covers[i]->Release(); covers[i]=NULL; }
        char path[80]; sprintf_s(path,"game:\\covers\\%d.png",targetPage*6+i);
        D3DXCreateTextureFromFileA(device,path,&covers[i]);
    }
}
int ItemCount() {
    if (guideOpen) return 4;
    if (page==0) return 9;
    if (page==1) { int n=static_cast<int>(data.games.size())-gamePage*6; return n>6?6:(n>0?n:1); }
    if (page==2) { int n=static_cast<int>(data.rooms.size())-roomPage*4; return 2+(n>4?4:(n>0?n:0)); }
    if (page==3) return 2;
    if (page==4) return 4;
    return 3;
}
void Activate() {
    if (guideOpen) {
        if (selected==0) { guideOpen=false; shutdownConfirm=false; }
        else if (selected==1) Command(5);
        else if (selected==2) Command(4);
        else if (shutdownConfirm) Command(6);
        else shutdownConfirm=true;
        return;
    }
    if (page==0) {
        if (selected==1 || selected==2) { page=1; selected=0; }
        else if(selected==8) { page=2; selected=0; Command(2); }
        else {
            const int game = selected==0 || selected==3 ? 0 : selected==4 ? 1 : selected==5 ? 2 : selected==6 ? 3 : 4;
            if(game<static_cast<int>(data.games.size())) Command(1,game);
            else { page=1; selected=0; }
        }
    } else if (page==1) {
        int index=gamePage*6+selected;
        if (index<static_cast<int>(data.games.size())) Command(1,index);
    } else if (page==2) {
        if (selected==0) Command(2);
        else if (selected==1) Command(3);
        else {
            int i=roomPage*4+selected-2;
            if (i<static_cast<int>(data.rooms.size()) && data.rooms[i].game>=0) Command(1,data.rooms[i].game);
        }
    } else if (page==3) {
        if(selected==0) page=1; else page=2; selected=0;
    }
    else if (page==4) {
        if (selected==0) Command(9);
        else if (selected==1) Command(3);
        else if (selected==2) { page=1; selected=0; }
        else { page=2; selected=0; Command(2); }
    } else if(selected==0) { page=1; selected=0; }
    else if(selected==1) Command(3);
    else { page=4; selected=0; }
}
int SelectedGameIndex() {
    if(guideOpen) return -1;
    int index=-1;
    if(page==0) {
        if(selected==0 || selected==3) index=0;
        else if(selected>=4 && selected<=7) index=selected-3;
    } else if(page==1) index=gamePage*6+selected;
    else if(page==2 && selected>=2) {
        const int room=roomPage*4+selected-2;
        if(room<static_cast<int>(data.rooms.size())) index=data.rooms[room].game;
    }
    return index>=0 && index<static_cast<int>(data.games.size())?index:-1;
}
void DrawCover(int n,float x,float y,float w,float h,const char* fallback) {
    Quad(x,y,w,h,0xff3f3f3f);
    if(n>=0 && n<static_cast<int>(data.games.size()) && covers[n]) {
        // Source covers are 256x360; crop their centre instead of stretching
        // characters across the wide Metro promotional tiles.
        const float sourceAspect=256.f/360.f, targetAspect=w/h;
        if(targetAspect>sourceAspect) {
            const float visible=sourceAspect/targetAspect;
            Quad(x,y,w,h,white,covers[n],0,(1.f-visible)*0.5f,1,(1.f+visible)*0.5f);
        } else {
            const float visible=targetAspect/sourceAspect;
            Quad(x,y,w,h,white,covers[n],(1.f-visible)*0.5f,0,(1.f+visible)*0.5f,1);
        }
    }
    else Text(x+18,y+18,fallback,30,white,w-32);
}
void GameTile(int n,float x,float y,float w,float h,int focus) {
    if(selected==focus) { Quad(x-5,y-5,w+10,h+10,white); Quad(x-2,y-2,w+4,h+4,0xff363636); }
    DrawCover(n,x,y,w,h,"Xbox 360");
    Quad(x,y+h-35,w,35,0xd6202020);
    Text(x+10,y+h-31,n<static_cast<int>(data.games.size())?data.games[n].name:"Mis juegos",18,white,w-18);
}
void ReadGuide() {
    FILE* f=fopen("game:\\guide.txt","rb");
    if(!f) return;
    char token[40]={}; unsigned long serial=0;
    bool valid=fscanf(f,"%39s %lu",token,&serial)==2 && data.nonce==token && serial>guideSerial;
    fclose(f);
    if(valid) { guideSerial=serial; guideOpen=true; shutdownConfirm=false; selected=0; }
}
void Render() {
    device->Clear(0,NULL,D3DCLEAR_TARGET,0xff8d8d8d,1,0);
    device->SetVertexShader(vertexShader); device->SetVertexDeclaration(declaration);
    device->SetRenderState(D3DRS_ZENABLE,FALSE);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
    device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
    device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    Gradient(0,360,0xff3c3c3c,0xff777777);
    Gradient(360,360,0xff777777,0xffb8b8b8);
    // The original Metro layout places section names on a single thin row.
    const char* tabs[]={"netplay","home","social","games","apps","settings"};
    const float tabX[]={148,290,420,555,690,835};
    const int tabPage[]={2,0,3,1,5,4};
    for(int t=0;t<6;++t) Text(tabX[t],120,tabs[t],30,page==tabPage[t]?white:0xffbcbcbc);
    Text(1150,38,data.profile.empty()?"Perfil":data.profile,21,white,120);
    if(page==0) {
        LoadCovers(0);
        // 360 Metro home: three narrow tiles, one wide feature, secondary
        // content on the right. Netplay takes the old advertisement space.
        GameTile(0,229,181,176,132,0);
        Tile(229,316,176,130,"MIS PINS","Juegos favoritos",1,0xff079107);
        Tile(229,449,176,132,"RECIENTES","Ultimos juegos",2,0xff079107);
        GameTile(0,408,181,466,265,3);
        GameTile(1,408,449,232,132,4);
        GameTile(2,643,449,231,132,5);
        GameTile(3,877,181,176,132,6);
        GameTile(4,877,316,176,130,7);
        Tile(877,449,176,132,"NETPLAY","Jugar juntos",8,0xff007d9a);
        Quad(1210,181,70,132,0xff168700);
        Text(1218,276,"amigos",16,white,60);
    } else if(page==1) {
        LoadCovers(gamePage);
        Text(229,168,"MIS JUEGOS",39,white);
        for(int i=0;i<6;++i) {
            const int index=gamePage*6+i;
            if(index>=static_cast<int>(data.games.size())) break;
            const float x=229+static_cast<float>(i%3)*276.f;
            const float y=236+static_cast<float>(i/3)*166.f;
            if(selected==i) { Quad(x-5,y-5,270,154,white); Quad(x-2,y-2,264,148,green); }
            DrawCover(i,x,y,260,144,"Xbox 360");
            Quad(x,y+111,260,33,0xd6202020);
            Text(x+10,y+113,data.games[index].name,19,white,240);
        }
        if(data.games.empty()) Text(229,270,"Anade juegos desde Emulos360.",25,white);
        const int focused=gamePage*6+selected;
        if(focused<static_cast<int>(data.games.size()))
            Text(229,574,data.games[focused].name,22,white,810);
        Text(229,611,"LT / RT   PAGINAS",17,white);
    } else if(page==2) {
        Text(229,168,"NETPLAY",39,white);
        Tile(229,236,320,147,"SALAS","Actualizar sesiones publicas",0,0xff087f95);
        Tile(229,396,320,147,"GESTOR","Configurar Xenia Canary",1,green);
        for(int i=0;i<4;++i) {
            int index=roomPage*4+i;
            if(index>=static_cast<int>(data.rooms.size())) break;
            const Room& r=data.rooms[index];
            Tile(568,236+static_cast<float>(i)*80,485,70,r.name.c_str(),r.game<0?"Juego no instalado":"Abrir juego",i+2,0xff4b4b4b);
        }
        Text(229,572,data.status,19,white,800);
        Text(229,605,"Para entrar, abre el juego y usa su menu multijugador.",18,white);
    } else if(page==3) {
        Text(229,168,"SOCIAL",39,white);
        Tile(229,236,400,278,"MI PERFIL",data.profile.c_str(),0,green);
        Tile(642,236,411,278,"NETPLAY","Amigos y sesiones publicas",1,0xff267ca7);
        Text(229,535,"Perfiles y logros permanecen disponibles en Emulos360.",20,white);
    } else if(page==4) {
        Text(229,168,"CONFIGURACION",39,white);
        Tile(229,236,399,147,"SISTEMA","Guia y alimentacion",0,green);
        Tile(642,236,411,147,"NETPLAY","Ajustes de Xenia Canary",1,0xff267ca7);
        Tile(229,396,399,147,"MIS JUEGOS","Biblioteca",2,0xff168700);
        Tile(642,396,411,147,"SALAS","Actualizar la lista",3,0xff555555);
    } else {
        Text(229,168,"APLICACIONES",39,white);
        Tile(229,236,262,278,"JUEGOS","Toda la biblioteca",0,green);
        Tile(505,236,262,278,"NETPLAY","Gestor de Xenia Canary",1,0xff267ca7);
        Tile(781,236,272,278,"AJUSTES","Sistema y red",2,0xff555555);
    }
    Quad(0,641,1280,79,0xb0323232);
    Text(94,657,"A  Seleccionar / jugar     B  Atras     Y  Caratula",18,white);
    Text(916,657,"Guia 2,5 s  Sistema",18,white,300);
    if(data.nonce.empty()) {
        Quad(130,268,1020,160,0xef252525);
        Text(163,290,"Abre Dashboard Metro desde Emulos360",32,white);
        Text(163,347,"El XEX necesita el anfitrion para juegos y Netplay.",21,white);
    }
    if(guideOpen) {
        Quad(0,0,1280,720,0xc0000000);
        Text(229,133,shutdownConfirm?"APAGAR EL PC":"GUIA EMULOS360",41,white);
        if(shutdownConfirm) Text(229,200,"Guarda la partida y los documentos antes de confirmar.",24,white);
        const char* labels[]={"Continuar","Reiniciar dashboard","Cerrar dashboard","Apagar Windows"};
        for(int i=0;i<4;++i) {
            const float x=229+static_cast<float>(i%2)*415.f,y=253+static_cast<float>(i/2)*125.f;
            Tile(x,y,398,108,labels[i],i==3&&shutdownConfirm?"A  CONFIRMAR":"",i,i==3?0xff9b3800:green);
        }
    }
    if(awaiting && GetTickCount()-requestTime>8000) {
        Quad(130,268,1020,100,0xef252525);
        Text(163,288,"El anfitrion no responde. B para reintentar.",27,white);
    }
    device->Present(NULL,NULL,NULL,NULL);
}
bool Initialize() {
    IDirect3D9* d3d=Direct3DCreate9(D3D_SDK_VERSION);
    if(!d3d) return false;
    D3DPRESENT_PARAMETERS pp={};
    pp.BackBufferWidth=1280; pp.BackBufferHeight=720;
    pp.BackBufferFormat=D3DFMT_A8R8G8B8; pp.FrontBufferFormat=D3DFMT_LE_X8R8G8B8;
    pp.BackBufferCount=1; pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
    HRESULT hr=d3d->CreateDevice(0,D3DDEVTYPE_HAL,NULL,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&device);
    d3d->Release(); if(FAILED(hr)) return false;
    const D3DVERTEXELEMENT9 elements[]={
        {0,0,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
        {0,8,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},
        {0,12,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
    return SUCCEEDED(device->CreateVertexDeclaration(elements,&declaration)) &&
        SUCCEEDED(device->CreateVertexShader(reinterpret_cast<const DWORD*>(g_vs),&vertexShader)) &&
        SUCCEEDED(device->CreatePixelShader(reinterpret_cast<const DWORD*>(g_solid),&solidShader)) &&
        SUCCEEDED(device->CreatePixelShader(reinterpret_cast<const DWORD*>(g_textured),&textureShader)) &&
        SUCCEEDED(D3DXCreateTextureFromFileA(device,"game:\\font.png",&font));
}
}
VOID __cdecl main() {
    if(!Initialize()) { OutputDebugStringA("Metro: renderer initialization failed\n"); return; }
    DWORD lastRead=0,repeatAt=0; WORD previous=0,previousTriggers=0;
    for(;;) {
        DWORD now=GetTickCount();
        if(now-lastRead>500) { ReadSnapshot(); ReadGuide(); lastRead=now; }
        if(awaiting) {
            FILE* ack=fopen("game:\\ack.txt","rb");
            if(ack) { unsigned long serial=0; if(fscanf(ack,"%lu",&serial)==1 && serial==requestSerial) awaiting=false; fclose(ack); }
        }
        XINPUT_STATE state={}; WORD buttons=0;
        for(DWORD user=0;user<4;++user) {
            if(XInputGetState(user,&state)==ERROR_SUCCESS) { buttons=state.Gamepad.wButtons; break; }
        }
        if(state.Gamepad.sThumbLX>18000) buttons|=XINPUT_GAMEPAD_DPAD_RIGHT;
        if(state.Gamepad.sThumbLX<-18000) buttons|=XINPUT_GAMEPAD_DPAD_LEFT;
        if(state.Gamepad.sThumbLY>18000) buttons|=XINPUT_GAMEPAD_DPAD_UP;
        if(state.Gamepad.sThumbLY<-18000) buttons|=XINPUT_GAMEPAD_DPAD_DOWN;
        const WORD triggers=static_cast<WORD>((state.Gamepad.bLeftTrigger>100?1:0)|
                                              (state.Gamepad.bRightTrigger>100?2:0));
        const WORD triggerPressed=static_cast<WORD>(triggers & ~previousTriggers);
        previousTriggers=triggers;
        WORD pressed=buttons & ~previous;
        if(buttons!=previous) repeatAt=now+380;
        else if(static_cast<LONG>(now-repeatAt)>=0) { pressed|=buttons & 15; repeatAt=now+135; }
        previous=buttons;
        if(pressed & XINPUT_GAMEPAD_B) { awaiting=false; if(guideOpen) { guideOpen=false; shutdownConfirm=false; } else { page=0; selected=0; } }
        if(!awaiting) {
            if(pressed & XINPUT_GAMEPAD_LEFT_SHOULDER) { page=(page+5)%6; selected=0; }
            if(pressed & XINPUT_GAMEPAD_RIGHT_SHOULDER) { page=(page+1)%6; selected=0; if(page==2) Command(2); }
            if(pressed & (XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_UP)) selected=(selected+ItemCount()-1)%ItemCount();
            if(pressed & (XINPUT_GAMEPAD_DPAD_RIGHT|XINPUT_GAMEPAD_DPAD_DOWN)) selected=(selected+1)%ItemCount();
            if(triggerPressed & 1) { if(page==1 && gamePage>0) --gamePage; if(page==2 && roomPage>0) --roomPage; selected=0; }
            if(triggerPressed & 2) { if(page==1 && (gamePage+1)*6<static_cast<int>(data.games.size())) ++gamePage; if(page==2 && (roomPage+1)*4<static_cast<int>(data.rooms.size())) ++roomPage; selected=0; }
            if(pressed & XINPUT_GAMEPAD_A) Activate();
            if(pressed & XINPUT_GAMEPAD_Y) {
                const int game=SelectedGameIndex();
                if(game>=0) Command(11,game);
            }
            if(pressed & XINPUT_GAMEPAD_START) Command(9);
        }
        if(selected>=ItemCount()) selected=0;
        Render();
    }
}
