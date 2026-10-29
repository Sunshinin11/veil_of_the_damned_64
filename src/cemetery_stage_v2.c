#include "cemetery_stage.h"
#include "mapgen.h"
#include "gfx.h"
#include <t3d/t3dmodel.h>
#include <assert.h>

enum { WALL_FACE, ROCK_A, ROCK_B, ROCK_C, ROOT_A, GRAVE_STONE, GRAVE_CROSS,
       FENCE_BASE, FENCE_POST, CRYPT_STONE, CRYPT_ROOF, CRYPT_DOOR, FENCE_IRON,
       CRYPT_WALL, CRYPT_PILLAR, CRYPT_NICHE, CRYPT_SARCOPHAGUS, CRYPT_ARCH, CRYPT_ALTAR,
       CRYPT_WALL_DAMAGED, CRYPT_RUBBLE_A, CRYPT_RUBBLE_B, CRYPT_FALLEN_COLUMN,
       ALT_EDGE,ALT_RAIL,ALT_POST,ALT_PYLON,ALT_CHAIN,ALT_CAGE,ALT_CAGE_BARS,ALT_THRONE,
       FOREST_TREE_A,FOREST_TREE_B,FOREST_TREE_C,FOREST_ROOT_MAT,FOREST_LOG,FOREST_STUMP,FOREST_STONE,
       FOREST_BROKEN_FENCE,SWAMP_TREE_A,SWAMP_TREE_B,SWAMP_TREE_C,SWAMP_ROOTS,SWAMP_LOG,SWAMP_WALKWAY,SWAMP_SHELTER,SWAMP_FENCE,
       HEART_WALL,HEART_CAP,HEART_ROOT,HEART_ROCK_B,HEART_ROCK_C,HEART_BRIDGE,HEART_BANK,HEART_FENCE,
       SNOW_FACE,SNOW_ROCK_A,SNOW_ROCK_B,SNOW_ROCK_C,SNOW_CAP_A,SNOW_CAP_B,SNOW_CAP_C,
       SNOW_RUIN,SNOW_BANK,ICE_FACE,ICE_MASS_A,ICE_MASS_B,ICE_MASS_C,ICE_FILL,
       JARL_WALL,JARL_WALL_L,JARL_WALL_R,JARL_WALL_LR,JARL_PILLAR,JARL_TABLE,JARL_ICE_FOOT,
       EX_CLIFF,EX_ROCK_A,EX_ROCK_B,EX_ROCK_C,EX_TOMB_WALL,EX_CITY_WALL,EX_CATHEDRAL_WALL,
       EX_COLUMN,EX_PIER,EX_OBELISK,EX_TOMB,EX_PEW,EX_ROOF,EX_WINDOW,EX_DOOR,EX_TOMBDOOR,EX_GLASS,EX_PINNACLE,KIT_COUNT };
enum { SOIL, PAVING, MOSS, CLIFF, ROCK, BARK, MASONRY, SLATE, DOOR, IRON, HEADSTONE_PAINT, CROSS_PAINT, GRAVE_EARTH,
       OPEN_GRAVE_PAINT, BONES_PAINT, BLOOD_PAINT, CRACKS_PAINT, MATERIAL_COUNT };
typedef struct {
    float x,y,z,yaw,scale,height_scale;
    T3DVec3 min,max;
    int next;
    uint8_t object;
} Occurrence;
typedef struct { int16_t min_x,min_z,max_x,max_z,y; uint16_t material; int next; } FloorPatch;
static T3DModel *kit;
static T3DObject *objects[KIT_COUNT];
static sprite_t *textures[MATERIAL_COUNT];
static Occurrence *instances;
static T3DMat4FP *matrices;
static T3DVertPacked *floor_vertices;
static FloorPatch *patches;
static int instance_count,patch_count,instance_capacity,patch_capacity;
static int object_head[KIT_COUNT],object_tail[KIT_COUNT];
static int floor_head[MATERIAL_COUNT],floor_tail[MATERIAL_COUNT];
static CemeteryPerf perf;
static uint32_t shared_fence_posts;
static bool crypt_active;
static bool altar_active;
static bool forest_active;
static bool swamp_active;
static bool heart_active;
static bool snow_active;
static bool ice_active;
static bool jarl_active;
static bool expansion_active;
static bool polished_cemetery;
static uint8_t *swamp_dry_field;
static uint8_t *path_union(const FloorQuad *floors,int nf,int rows,int cols,int *quads,bool marsh);
typedef struct {int16_t x[12],z[12];} IcePatch;
static IcePatch ice_patches[MG_CW*MG_CH];
static int ice_patch_count;
typedef struct {int16_t x[12],z[12];float speed;} WetPatch;
static WetPatch wet_patches[64];static int wet_count;
static const char *const *swamp_grid;
static int swamp_rows,swamp_cols;
static const FloorQuad *swamp_routes;static int swamp_route_count;
const CemeteryPerf *cemetery_stage_perf(void) {return &perf;}

bool cemetery_stage_ready(void) { return kit && floor_vertices; }
void cemetery_stage_release(void) {
    if (kit || floor_vertices) rspq_wait();
    if (kit) t3d_model_free(kit);
    for (int i=0;i<MATERIAL_COUNT;i++) { if (textures[i] && i!=GRAVE_EARTH) sprite_free(textures[i]); textures[i]=NULL; }
    if (floor_vertices) free_uncached(floor_vertices);
    if (matrices) free_uncached(matrices);
    free(instances);free(patches);
    kit=NULL;floor_vertices=NULL;matrices=NULL;instances=NULL;patches=NULL;
    crypt_active=false;expansion_active=false;polished_cemetery=false;
    free(swamp_dry_field);swamp_dry_field=NULL;
    altar_active=false;
    forest_active=false;
    swamp_active=false;heart_active=false;snow_active=false;ice_active=false;jarl_active=false;ice_patch_count=0;wet_count=0;swamp_routes=NULL;swamp_grid=NULL;swamp_route_count=0;
    instance_count=patch_count=0;
    for(int i=0;i<KIT_COUNT;i++)object_head[i]=object_tail[i]=-1;
    for(int i=0;i<MATERIAL_COUNT;i++)floor_head[i]=floor_tail[i]=-1;
    memset(&perf,0,sizeof(perf));
    shared_fence_posts=0;
}
static void vertex(int index,float x,float y,float z,float s,float t) {
    int16_t *p=t3d_vertbuffer_get_pos(floor_vertices,index),*uv=t3d_vertbuffer_get_uv(floor_vertices,index);
    p[0]=lroundf(x);p[1]=lroundf(y);p[2]=lroundf(z);
    uv[0]=lroundf(s*32);uv[1]=lroundf(t*32);
    *t3d_vertbuffer_get_color(floor_vertices,index)=0xFFFFFFFF;
    *t3d_vertbuffer_get_norm(floor_vertices,index)=t3d_vert_pack_normal(&(T3DVec3){{0,1,0}});
}
static void floor_patch(const int16_t *xx,const int16_t *zz,int material,int y) {
    assertf(patch_count<patch_capacity,"Cemetery floor capacity");
    int x0=xx[0],x1=xx[0],z0=zz[0],z1=zz[0];
    for (int i=1;i<6;i++) {
        if (xx[i]<x0)x0=xx[i];
        if (xx[i]>x1)x1=xx[i];
        if (zz[i]<z0)z0=zz[i];
        if (zz[i]>z1)z1=zz[i];
    }
    // Common local origin preserves continuity and keeps 10.5 UVs in range.
    float period=((swamp_active || heart_active) && (material==SOIL || material==PAVING || material==MOSS))?144:72;
    float origin=period*2,bx=floorf(x0/origin)*origin,bz=floorf(z0/origin)*origin;
    for (int i=0;i<6;i++) vertex(patch_count*6+i,xx[i],y,zz[i],(xx[i]-bx)*(64.0f/period),(zz[i]-bz)*(64.0f/period));
    int index=patch_count++;
    patches[index]=(FloorPatch){x0,z0,x1,z1,y,material,-1};
    if(floor_tail[material]>=0)patches[floor_tail[material]].next=index;
    else floor_head[material]=index;
    floor_tail[material]=index;
}
static void floor_rect(int x0,int z0,int x1,int z1,int material,int y) {
    if (x1<=x0 || z1<=z0)return;
    int16_t xx[6]={x0,x1,x1,x0,x1,x0},zz[6]={z0,z0,z1,z0,z1,z1};
    floor_patch(xx,zz,material,y);
}
static int ground_height(float x0,float z0,float x1,float z1) {
    int y=0;
    for(int i=0;i<patch_count;i++) {
        const FloorPatch *p=&patches[i];
        if((p->material>MOSS && !((swamp_active || heart_active) && (p->material==CLIFF || p->material==ROCK))) || p->max_x<x0 || p->min_x>x1 || p->max_z<z0 || p->min_z>z1)continue;
        if(p->y>y)y=p->y;
    }
    return y;
}
float cemetery_stage_ground_y(float x,float z,float width) {
    if(!cemetery_stage_ready())return 4; // highest legacy walkable floor layer
    float half=width*.5f;
    return ground_height(x-half,z-half,x+half,z+half);
}
bool cemetery_stage_decal(CemeteryDecal type,float x,float z,float width) {
    if(!cemetery_stage_ready() || patch_count>=patch_capacity || width<=0 || (unsigned)type>=CEM_DECAL_COUNT)return false;
    static const int decal_material[CEM_DECAL_COUNT]={OPEN_GRAVE_PAINT,BONES_PAINT,BLOOD_PAINT,CRACKS_PAINT};
    int mat=decal_material[type];
    float hx=width*0.5f,hz=width*0.27f;
    int y=ground_height(x-hx,z-hz,x+hx,z+hz);
    int index=patch_count;
    floor_rect(lroundf(x-hx),lroundf(z-hz),lroundf(x+hx),lroundf(z+hz),mat,y+1);
    // Preserve artwork orientation in this camera (+X points left, +Z points up).
    int w=textures[mat]->width,h=textures[mat]->height;
    const int st[6][2]={{w,h},{0,h},{0,0},{w,h},{0,0},{w,0}};
    for(int i=0;i<6;i++) {
        int16_t *uv=t3d_vertbuffer_get_uv(floor_vertices,index*6+i);
        uv[0]=st[i][0]*32;uv[1]=st[i][1]*32;
    }
    return true;
}
static bool wall(const char *const *grid,int rows,int cols,int c,int r) {
    return c<0||r<0||c>=cols||r>=rows||grid[r][c]=='#';
}
static void occurrence_scaled(int object,float x,float y,float z,float yaw,float scale,float height_scale) {
    assertf(instance_count<instance_capacity,"Cemetery kit capacity");
    int i=instance_count++;
    instances[i]=(Occurrence){.x=x,.y=y,.z=z,.yaw=yaw,.scale=scale,.height_scale=height_scale,.object=object,.next=-1};
    if(object_tail[object]>=0)instances[object_tail[object]].next=i;
    else object_head[object]=i;
    object_tail[object]=i;
    // Transform the exported local AABB once at placement, not every frame.
    const T3DObject *model=objects[object];
    float cx=(model->aabbMin[0]+model->aabbMax[0])*0.5f*scale;
    float cz=(model->aabbMin[2]+model->aabbMax[2])*0.5f*scale;
    float hx=(model->aabbMax[0]-model->aabbMin[0])*0.5f*scale;
    float hz=(model->aabbMax[2]-model->aabbMin[2])*0.5f*scale;
    float ca=cosf(yaw),sa=sinf(yaw);
    // Match Tiny3D's actual Euler matrix convention (local +Z rotates toward -X).
    float px=x+ca*cx-sa*cz,pz=z+sa*cx+ca*cz;
    float ex=fabsf(ca)*hx+fabsf(sa)*hz,ez=fabsf(sa)*hx+fabsf(ca)*hz;
    instances[i].min=(T3DVec3){{px-ex-2,y+model->aabbMin[1]*height_scale-2,pz-ez-2}};
    instances[i].max=(T3DVec3){{px+ex+2,y+model->aabbMax[1]*height_scale+2,pz+ez+2}};
    if (matrices) t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){scale,height_scale,scale},
        (float[]){0,yaw,0},(float[]){x,y,z});
}
static void occurrence(int object,float x,float y,float z,float yaw) {
    occurrence_scaled(object,x,y,z,yaw,1,1);
}
static void fence_post(float x,float z) {
    for(int i=object_head[FENCE_POST];i>=0;i=instances[i].next) {
        float dx=instances[i].x-x,dz=instances[i].z-z;
        if(dx*dx+dz*dz<0.25f){shared_fence_posts++;return;}
    }
    occurrence(FENCE_POST,x,0,z,0);
}
static void grave_earth(float x,float z,float yaw) {
    const int corners[6][2]={{-10,-30},{10,-30},{10,3},{-10,-30},{10,3},{-10,3}};
    const int uv[6][2]={{64,64},{0,64},{0,0},{64,64},{0,0},{64,0}};
    float ca=cosf(yaw),sa=sinf(yaw);
    int16_t xx[6],zz[6];int x0=32767,x1=-32768,z0=32767,z1=-32768;
    for(int i=0;i<6;i++) {
        xx[i]=lroundf(x+ca*corners[i][0]-sa*corners[i][1]);
        zz[i]=lroundf(z+sa*corners[i][0]+ca*corners[i][1]);
        if(xx[i]<x0)x0=xx[i];
        if(xx[i]>x1)x1=xx[i];
        if(zz[i]<z0)z0=zz[i];
        if(zz[i]>z1)z1=zz[i];
    }
    int index=patch_count;
    floor_patch(xx,zz,GRAVE_EARTH,ground_height(x0,z0,x1,z1)+1);
    for(int i=0;i<6;i++) {
        int16_t *st=t3d_vertbuffer_get_uv(floor_vertices,index*6+i);
        st[0]=uv[i][0]*32;st[1]=uv[i][1]*32;
        *t3d_vertbuffer_get_color(floor_vertices,index*6+i)=0xB0A895FF;
    }
}
bool cemetery_stage_place(CemeteryProp type,float x,float z,float yaw) {
    if (!cemetery_stage_ready() || instance_count+4>instance_capacity) return false;
    switch(type) {
        case CEM_GRAVE: case CEM_CROSS:
            if(patch_count>=patch_capacity)return false;
            occurrence(type==CEM_GRAVE?GRAVE_STONE:GRAVE_CROSS,x,ground_height(x-14,z-5,x+14,z+5)+1,z,yaw);
            grave_earth(x,z,yaw);break;
        case CEM_FENCE:
            occurrence(FENCE_BASE,x,0,z,yaw);occurrence(FENCE_IRON,x,0,z,yaw);
            fence_post(x-cosf(yaw)*36,z-sinf(yaw)*36);
            fence_post(x+cosf(yaw)*36,z+sinf(yaw)*36);break;
        case CEM_CRYPT:
            occurrence(CRYPT_STONE,x,0,z,yaw);occurrence(CRYPT_ROOF,x,0,z,yaw);
            occurrence(CRYPT_DOOR,x,0,z,yaw);break;
        default:return false;
    }
    return true;
}
void cemetery_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();polished_cemetery=true;
    uint64_t started=get_ticks();
    static const char *const paths[MATERIAL_COUNT]={
        "rom:/cem_v6_soil.sprite","rom:/cem_v3_paving.sprite","rom:/cem_v3_moss.sprite",
        "rom:/cem_v3_cliff.sprite","rom:/cem_v3_rock.sprite","rom:/cem_v3_bark.sprite",
        "rom:/cem_v3_masonry.sprite","rom:/cem_v3_slate.sprite","rom:/cem_v3_door.sprite","rom:/cem_v3_iron.sprite",
        "rom:/cem_v5_headstone.sprite","rom:/cem_v5_cross.sprite","rom:/cem_v3_soil.sprite",
        "rom:/scene_decal_open_grave.rgba16.sprite","rom:/scene_decal_bones.rgba16.sprite",
        "rom:/scene_decal_blood.rgba16.sprite","rom:/scene_decal_cracks.rgba16.sprite"
    };
    for (int i=0;i<MATERIAL_COUNT;i++) { textures[i]=i==GRAVE_EARTH?textures[SOIL]:sprite_load(paths[i]); assertf(textures[i],"Missing cemetery material %d",i); }
    kit=t3d_model_load("rom:/cem_v4_kit.t3dm");assertf(kit,"Missing cemetery kit");
    static const char *const names[CRYPT_WALL]={"wall_face","rock_a","rock_b","rock_c","root_a",
        "grave_stone","grave_cross","fence_base","fence_post","crypt_stone","crypt_roof","crypt_door","fence_iron"};
    for (int i=0;i<CRYPT_WALL;i++) {objects[i]=t3d_model_get_object(kit,names[i]);assertf(objects[i],"Missing cemetery object %s",names[i]);}
    const FloorQuad *floors;int nf=mapgen_floor(&floors);
    const int apron=7;
    patch_capacity=((cols+apron*2+1)/2)*((rows+apron*2+1)/2)+(rows+apron*2)*(cols+apron*2)+nf+144;
    instance_capacity=(rows+apron*2)*(cols+apron*2)*6+768;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);
    floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);
    instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate cemetery scene");
    int hw=cols*36,hh=rows*36;
    for (int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2) {
        int index=patch_count;floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SOIL,0);
        for(int v=0;v<6;v++) {
            int16_t *p=t3d_vertbuffer_get_pos(floor_vertices,index*6+v);
            float shade=.91f+.065f*sinf(p[0]*.006f+p[2]*.002f)+.025f*cosf(p[2]*.009f-p[0]*.003f);
            int red=225*shade,green=231*shade,blue=232*shade;
            *t3d_vertbuffer_get_color(floor_vertices,index*6+v)=((uint32_t)red<<24)|((uint32_t)green<<16)|((uint32_t)blue<<8)|255;
        }
    }
    static const int floor_mat[4]={PAVING,SOIL,PAVING,MOSS},floor_y[4]={3,1,3,1};
    for (int i=0;i<nf;i++)floor_patch(floors[i].x,floors[i].z,floor_mat[floors[i].kind&3],floor_y[floors[i].kind&3]);
    static const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for (int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        if (!wall(grid,rows,cols,c,r))continue;
        int x=(c*72+36)-hw,z=hh-(r*72+36),mask=0;
        for (int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d]))mask|=1<<d;
        for(int d=0;d<4;d++)if(mask&(1<<d))occurrence(WALL_FACE,x,0,z,d*1.5707963268f);
        // Cap only the inner remainder. Sloped faces reach the collision boundary.
        int x0=x-36,x1=x+36,z0=z-36,z1=z+36;
        if(mask&1)z1=z;
        if(mask&2)x1=x;
        if(mask&4)z0=z;
        if(mask&8)x0=x;
        // All blocked ground, including unused rooms and the outside apron, is rock.
        // The plateau closes the sculpted faces; boulders break up its silhouette.
        floor_rect(x0,z0,x1,z1,ROCK,46);
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        // Ornament remains within the blocked cell, away from traversable paths.
        if (mask || (h%3)!=0) {
            float scale=mask?0.88f+(h%9)*0.012f:1.12f+(h%7)*0.045f;
            float angle=mask?(h%4)*1.5707963268f:(h%61)*0.103f;
            occurrence_scaled(ROCK_A+((h>>4)%3),x,mask?24:43,z,angle,scale,0.72f+((h>>8)%8)*0.07f);
        }
        if (mask && (h%7)==0) {
            int d=0;while(!(mask&(1<<d)))d++;
            occurrence(ROOT_A,x+dc[d]*18,0,z-dr[d]*18,d*1.5707963268f);
        }
    }
    // Resident transforms outlive all asynchronous frames; no per-frame matrix-ring pressure.
    instance_capacity=instance_count+768;
    Occurrence *trimmed=realloc(instances,sizeof(Occurrence)*instance_capacity);
    assertf(trimmed,"Cannot retain cemetery instances");instances=trimmed;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);
    assertf(matrices || !instance_count,"Cannot allocate cemetery transforms");
    for(int i=0;i<instance_count;i++) {
        Occurrence *o=&instances[i];
        t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});
    }
    debugf("[CEM V3] random grid %dx%d patches=%d shared instances=%d build=%lu us\n",cols,rows,patch_count,instance_count,(unsigned long)((get_ticks()-started)*1000000/TICKS_PER_SECOND));
}
void crypt_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();crypt_active=true;
    textures[SOIL]=sprite_load("rom:/crypt_v1_floor.sprite");
    textures[MASONRY]=sprite_load("rom:/crypt_v1_wall.sprite");
    textures[SLATE]=sprite_load("rom:/cem_v3_masonry.sprite");
    textures[ROCK]=sprite_load("rom:/cem_v3_slate.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_decal_open_grave.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_decal_blood.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_decal_cracks.rgba16.sprite");
    assertf(textures[SOIL] && textures[MASONRY] && textures[SLATE] && textures[ROCK] &&
        textures[OPEN_GRAVE_PAINT] && textures[BONES_PAINT] && textures[BLOOD_PAINT] && textures[CRACKS_PAINT],"Missing crypt texture");
    kit=t3d_model_load("rom:/crypt_v1_kit.t3dm");assertf(kit,"Missing crypt kit");
    static const char *const names[]={"crypt_wall","crypt_pillar","crypt_niche","crypt_sarcophagus","crypt_arch","crypt_altar",
        "crypt_wall_damaged","crypt_rubble_a","crypt_rubble_b","crypt_fallen_column"};
    for(int i=CRYPT_WALL;i<ALT_EDGE;i++) {
        objects[i]=t3d_model_get_object(kit,names[i-CRYPT_WALL]);
        assertf(objects[i],"Missing crypt object %s",names[i-CRYPT_WALL]);
    }
    const int apron=7,hw=cols*36,hh=rows*36;
    patch_capacity=(rows+2*apron)*(cols+2*apron)+512;
    instance_capacity=rows*cols*5+768;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);
    floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);
    instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate crypt scene");
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        int x=c*72+36-hw,z=hh-r*72-36;
        bool blocked=wall(grid,rows,cols,c,r);
        int index=patch_count;
        floor_rect(x-36,z-36,x+36,z+36,blocked?ROCK:SOIL,blocked?56:0);
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        uint32_t color=blocked?0x58636AFF:((h%3)==0?0xD0D8DCFF:0xE4E8ECFF);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=color;
        if(!blocked)continue;
        bool niche=false;
        for(int d=0;d<4;d++) {
            if(wall(grid,rows,cols,c+dc[d],r+dr[d]))continue;
            // Tiny3D rotates local +Z toward -X for a positive Y angle.
            // Grid east is world +X, so exposed faces use the negative angle.
            bool damaged=((h+d*17)%7)==0;
            occurrence(damaged?CRYPT_WALL_DAMAGED:CRYPT_WALL,x,0,z,-d*1.5707963268f);
            if(!damaged && !niche && (h%5)<2) {occurrence(CRYPT_NICHE,x,0,z,-d*1.5707963268f);niche=true;}
        }
    }
    instance_capacity=instance_count+768;
    Occurrence *trimmed=realloc(instances,sizeof(Occurrence)*instance_capacity);
    assertf(trimmed,"Cannot retain crypt instances");instances=trimmed;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);
    assertf(matrices,"Cannot allocate crypt transforms");
    for(int i=0;i<instance_count;i++) {
        Occurrence *o=&instances[i];
        t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},
            (float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});
    }
    debugf("[CRYPT V1] random grid %dx%d floor patches=%d architecture=%d\n",cols,rows,patch_count,instance_count);
}
bool crypt_stage_place(CryptProp type,float x,float z,float yaw) {
    static const int types[]={CRYPT_PILLAR,CRYPT_SARCOPHAGUS,CRYPT_ALTAR,CRYPT_ARCH,CRYPT_RUBBLE_A,CRYPT_RUBBLE_B,CRYPT_FALLEN_COLUMN};
    if(!crypt_active || !cemetery_stage_ready() || (unsigned)type>=CRYPT_PROP_COUNT || instance_count>=instance_capacity)return false;
    occurrence(types[type],x,0,z,yaw);return true;
}
static void altar_post(float x,float z) {
    for(int i=object_head[ALT_POST];i>=0;i=instances[i].next)
        if(fabsf(instances[i].x-x)<.5f && fabsf(instances[i].z-z)<.5f)return;
    occurrence(ALT_POST,x,0,z,0);
}
void altar_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();altar_active=true;
    textures[SOIL]=sprite_load("rom:/altar_v1_floor.sprite");
    textures[ROCK]=sprite_load("rom:/crypt_v1_wall.sprite");
    textures[MASONRY]=sprite_load("rom:/cem_v3_slate.sprite");
    textures[SLATE]=sprite_load("rom:/altar_v1_lava.sprite");
    textures[IRON]=sprite_load("rom:/altar_v1_rail.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_hell_decal_ritual.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_hell_decal_ash.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_hell_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_hell_decal_blood.rgba16.sprite");
    assertf(textures[SOIL] && textures[ROCK] && textures[MASONRY] && textures[SLATE] && textures[IRON] &&
        textures[OPEN_GRAVE_PAINT] && textures[CRACKS_PAINT] && textures[BONES_PAINT] && textures[BLOOD_PAINT],"Missing altar material");
    kit=t3d_model_load("rom:/altar_v1_kit.t3dm");assertf(kit,"Missing altar kit");
    static const char *const names[]={"altar_edge","altar_rail","altar_post","altar_pylon","altar_chain","altar_cage","altar_cage_bars","altar_throne"};
    for(int i=ALT_EDGE;i<FOREST_TREE_A;i++) {
        objects[i]=t3d_model_get_object(kit,names[i-ALT_EDGE]);assertf(objects[i],"Missing altar object %s",names[i-ALT_EDGE]);
    }
    const int apron=7,hw=cols*36,hh=rows*36;
    patch_capacity=(rows+14)*(cols+14)+768;instance_capacity=rows*cols*7+1024;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);
    instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate altar scene");
    // Lava is below solid platforms, including the regions behind the railings.
    for(int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2)
        floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SLATE,-40);
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=0;r<rows;r++)for(int c=0;c<cols;c++) {
        if(wall(grid,rows,cols,c,r))continue;
        int x=c*72+36-hw,z=hh-r*72-36,index=patch_count;bool edge=false;
        floor_rect(x-36,z-36,x+36,z+36,SOIL,0);
        for(int d=0;d<4;d++) {
            if(!wall(grid,rows,cols,c+dc[d],r+dr[d]))continue;
            edge=true;float angle=-d*1.5707963268f;
            occurrence(ALT_EDGE,x,0,z,angle);
            occurrence_scaled(ALT_RAIL,x,0,z,angle,1,d==2?.82f:1.1f);
            for(int side=-1;side<=1;side+=2) {
                float px=x+cosf(angle)*side*36-sinf(angle)*36;
                float pz=z+sinf(angle)*side*36+cosf(angle)*36;
                altar_post(px,pz);
            }
        }
        uint32_t color=edge?0xD6B8A8FF:0xD4D8DEFF;
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=color;
    }
    instance_capacity=instance_count+768;
    Occurrence *trimmed=realloc(instances,sizeof(Occurrence)*instance_capacity);assertf(trimmed,"Cannot retain altar instances");instances=trimmed;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);assertf(matrices,"Cannot allocate altar transforms");
    for(int i=0;i<instance_count;i++) {
        Occurrence *o=&instances[i];
        t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});
    }
    debugf("[ALTAR] platforms=%d architecture=%d\n",patch_count,instance_count);
}
bool altar_stage_place(AltarProp type,float x,float z,float yaw) {
    static const int types[]={ALT_PYLON,ALT_CAGE,ALT_THRONE,ALT_CHAIN};
    if(!altar_active || !cemetery_stage_ready() || (unsigned)type>=ALT_PROP_COUNT || instance_count+3>instance_capacity)return false;
    occurrence(types[type],x,0,z,yaw);
    if(type==ALT_PROP_CAGE)occurrence(ALT_CAGE_BARS,x,0,z,yaw);
    if(type==ALT_PROP_PYLON)occurrence(ALT_CHAIN,x+18*cosf(yaw),0,z+18*sinf(yaw),yaw);
    return true;
}
static int altar_paint_material(AltarPaint type) {
    static const int mats[]={OPEN_GRAVE_PAINT,CRACKS_PAINT,BONES_PAINT,BLOOD_PAINT};
    return mats[(unsigned)type<4?type:ALT_PAINT_ASH];
}
int altar_stage_paint(AltarPaint type,float x,float z,float width) {
    if(!altar_active || patch_count>=patch_capacity || width<=0)return -1;
    int index=patch_count,mat=altar_paint_material(type);
    float hz=width*((type==ALT_PAINT_RITUAL || type==ALT_PAINT_ASH)?.5f:.27f);
    floor_rect(lroundf(x-width*.5f),lroundf(z-hz),lroundf(x+width*.5f),lroundf(z+hz),mat,1);
    int w=textures[mat]->width,h=textures[mat]->height;
    const int st[6][2]={{w,h},{0,h},{0,0},{w,h},{0,0},{w,0}};
    for(int v=0;v<6;v++) {int16_t *uv=t3d_vertbuffer_get_uv(floor_vertices,index*6+v);uv[0]=st[v][0]*32;uv[1]=st[v][1]*32;}
    return index;
}
void altar_stage_paint_set(int handle,AltarPaint type) {
    if(!altar_active || handle<0 || handle>=patch_count)return;
    int mat=altar_paint_material(type),old=patches[handle].material;
    if(old>=MATERIAL_COUNT)return; // a completed ritual's painting has been removed
    if(old==mat)return;
    int prev=-1;
    for(int i=floor_head[old];i>=0;prev=i,i=patches[i].next)if(i==handle) {
        if(prev<0)floor_head[old]=patches[i].next;else patches[prev].next=patches[i].next;
        if(floor_tail[old]==i)floor_tail[old]=prev;
        break;
    }
    patches[handle].next=-1;
    if(old==OPEN_GRAVE_PAINT && type==ALT_PAINT_ASH) {
        // The small ember image looks stretched at the ritual's footprint.
        // Reveal the existing floor instead; the broken obelisk remains visible.
        patches[handle].material=MATERIAL_COUNT;
        return;
    }
    patches[handle].material=mat;
    if(floor_tail[mat]>=0)patches[floor_tail[mat]].next=handle;else floor_head[mat]=handle;
    floor_tail[mat]=handle;
}
// Feather the forest's existing floor shapes over continuous leaf mold. Blobs
// retain their fan center; paths retain their connected strip cross-sections.
static void forest_ground_quad(const int16_t *xx,const int16_t *zz,int mat,uint32_t rgb,const uint8_t *alpha) {
    int index=patch_count;floor_patch(xx,zz,mat,mat==MOSS?1:2);
    for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=rgb|alpha[v];
}
static void forest_ground_shape(const FloorQuad *f,bool fan) {
    int mat=f->kind==3?MOSS:PAVING;
    uint32_t rgb=snow_active?0xDEE4E800:mat==MOSS?0xB0C2A400:0xDCD8CC00;
    uint8_t opacity=mat==MOSS?192:(fan?150:220);
    const uint8_t solid[6]={opacity,opacity,opacity,opacity,opacity,opacity};
    const uint8_t fade[6]={opacity,opacity,0,opacity,0,0};
    int16_t ix[6],iz[6];
    if(fan) {
        for(int v=0;v<6;v++) {
            ix[v]=lroundf(f->x[0]+(f->x[v]-f->x[0])*.64f);
            iz[v]=lroundf(f->z[0]+(f->z[v]-f->z[0])*.64f);
        }
    } else {
        // Start and end midpoints of the strip; feather only its two sides.
        float mx[2]={(f->x[0]+f->x[1])*.5f,(f->x[2]+f->x[5])*.5f};
        float mz[2]={(f->z[0]+f->z[1])*.5f,(f->z[2]+f->z[5])*.5f};
        for(int v=0;v<6;v++) {
            int end=(v==2 || v==4 || v==5);
            ix[v]=lroundf(mx[end]+(f->x[v]-mx[end])*.72f);
            iz[v]=lroundf(mz[end]+(f->z[v]-mz[end])*.72f);
        }
    }
    forest_ground_quad(ix,iz,mat,rgb,solid);
    const int edges[2][2]={{fan?1:0,fan?2:5},{fan?4:1,fan?5:2}};
    for(int k=0;k<2;k++) {
        int a=edges[k][0],b=edges[k][1];
        int16_t xx[6]={ix[a],ix[b],f->x[b],ix[a],f->x[b],f->x[a]};
        int16_t zz[6]={iz[a],iz[b],f->z[b],iz[a],f->z[b],f->z[a]};
        forest_ground_quad(xx,zz,mat,rgb,fade);
    }
}
void forest_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();forest_active=true;
    textures[SOIL]=sprite_load("rom:/forest_v6_ground.sprite");
    textures[PAVING]=sprite_load("rom:/forest_v2_dirt.sprite");
    textures[MOSS]=sprite_load("rom:/forest_v5_grass.sprite");
    textures[BARK]=sprite_load("rom:/cem_v3_bark.sprite");
    textures[ROCK]=sprite_load("rom:/cem_v3_rock.sprite");
    // Forest has no open graves: reuse this ground-paint slot for spider silk.
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_cave_decal_web.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_decal_blood.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_decal_cracks.rgba16.sprite");
    const int required[]={SOIL,PAVING,MOSS,BARK,ROCK,OPEN_GRAVE_PAINT,BONES_PAINT,BLOOD_PAINT,CRACKS_PAINT};
    for(unsigned i=0;i<sizeof(required)/sizeof(required[0]);i++)assertf(textures[required[i]],"Missing forest material %d",required[i]);
    kit=t3d_model_load("rom:/forest_v7_kit.t3dm");assertf(kit,"Missing forest kit");
    static const char *const names[]={"forest_tree_a","forest_tree_b","forest_tree_c","forest_root_mat","forest_log","forest_stump","forest_stone",
        "forest_broken_fence"};
    for(int i=FOREST_TREE_A;i<SWAMP_TREE_A;i++) {
        objects[i]=t3d_model_get_object(kit,names[i-FOREST_TREE_A]);assertf(objects[i],"Missing forest object %s",names[i-FOREST_TREE_A]);
    }
    const FloorQuad *floor;int nf=mapgen_floor(&floor);
    const int apron=7,hw=cols*36,hh=rows*36;
    // Continuous terrain and feathered dirt/grass; no experimental paving.
    patch_capacity=((rows+14+1)/2)*((cols+14+1)/2)+nf*4+512;
    instance_capacity=(rows+14)*(cols+14)*6+768;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);
    floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);
    instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate forest scene");
    // Continuous leaf mold under clearings, thickets and the exterior apron.
    for(int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2) {
        int index=patch_count;
        floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SOIL,0);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=0xBCC1C2FF;
    }
    for(int i=0;i<nf;i++) {
        if(floor[i].kind==3) {
            int mx=(floor[i].x[0]+hw)/(72*MG_STEP),my=(hh-floor[i].z[0])/(72*MG_STEP);
            // Whole irregular grass fans in a few clearings; never a green
            // floor under the entire forest. Shared centers keep fans intact.
            bool alive=(mx==MG_CW-1 && my<2) || ((mx+my*MG_CW)%4==1);
            unsigned patch=(uint32_t)(int32_t)floor[i].x[0]*73856093u^(uint32_t)(int32_t)floor[i].z[0]*19349663u;
            if(!alive || patch%3==0)continue;
        }
        bool fan=floor[i].kind!=1;
        if(!fan)for(int j=0;j<nf;j++) {
            if(j!=i && floor[j].kind==1 && floor[j].x[0]==floor[i].x[0] && floor[j].z[0]==floor[i].z[0]) {fan=true;break;}
        }
        FloorQuad shape=floor[i];
        if(!fan) {
            float sx=(shape.x[0]+shape.x[1])*.5f,sz=(shape.z[0]+shape.z[1])*.5f;
            float ex=(shape.x[2]+shape.x[5])*.5f,ez=(shape.z[2]+shape.z[5])*.5f;
            for(int v=0;v<6;v++) {
                bool end=v==2 || v==4 || v==5;
                float mx=end?ex:sx,mz=end?ez:sz;
                shape.x[v]=lroundf(mx+(shape.x[v]-mx)*.68f);
                shape.z[v]=lroundf(mz+(shape.z[v]-mz)*.68f);
            }
        } else if(shape.kind!=3) {
            // Clearings stay open, but no longer read as broad brown road discs.
            for(int v=0;v<6;v++) {
                shape.x[v]=lroundf(floor[i].x[0]+(shape.x[v]-floor[i].x[0])*.72f);
                shape.z[v]=lroundf(floor[i].z[0]+(shape.z[v]-floor[i].z[0])*.72f);
            }
        }
        forest_ground_shape(&shape,fan);
    }
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        if(!wall(grid,rows,cols,c,r))continue;
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        float x=c*72+36-hw,z=hh-r*72-36;bool edge=false;
        for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d]))edge=true;
        // Broken enclosure fragments sit INSIDE blocked woodland, aligned to
        // the opening. The grid already supplies their collision boundary.
        for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d]) && (h+d*11)%4==0)
            occurrence(FOREST_BROKEN_FENCE,x+dc[d]*26,0,z-dr[d]*26,-d*1.5707963268f);
        // Root tangles reach every blocked boundary; crooked trunks interrupt
        // the grid with varied forks, rotation, spacing and height. No wall mesh.
        if(edge || h%3==0)occurrence(FOREST_ROOT_MAT,x,0,z,(h%4)*1.5707963268f);
        if(edge || h%3!=0) {
            float scale=.86f+((h>>4)%8)*.025f;
            float hs=.88f+((h>>8)%9)*.04f;
            occurrence_scaled(FOREST_TREE_A+((h>>2)%3),x+(int)(h%11)-5,0,z+(int)((h>>5)%11)-5,
                (h%59)*.107f,scale,hs);
        }
        if(edge && h%7==0)occurrence_scaled(FOREST_STONE,x,0,z,((h>>3)%6)*.52f,1.1f,.8f);
    }
    instance_capacity=instance_count+768;
    Occurrence *trimmed=realloc(instances,sizeof(Occurrence)*instance_capacity);
    assertf(trimmed,"Cannot retain forest instances");instances=trimmed;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);assertf(matrices,"Cannot allocate forest transforms");
    for(int i=0;i<instance_count;i++) {
        Occurrence *o=&instances[i];
        t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});
    }
    debugf("[FOREST V1] random grid %dx%d floor=%d woodland=%d\n",cols,rows,patch_count,instance_count);
}
bool forest_stage_place(ForestProp type,float x,float z,float yaw,float scale) {
    static const int types[]={FOREST_TREE_A,FOREST_TREE_B,FOREST_TREE_C,FOREST_LOG,FOREST_STUMP,FOREST_STONE,FOREST_BROKEN_FENCE};
    if(!forest_active || !cemetery_stage_ready() || (unsigned)type>=FOREST_PROP_COUNT || instance_count+3>instance_capacity)return false;
    occurrence_scaled(types[type],x,0,z,yaw,scale,scale);
    return true;
}
bool forest_stage_spot_free(float x,float z,float radius) {
    if(!forest_active && !swamp_active && !snow_active)return true;
    for(int i=0;i<instance_count;i++) {
        const Occurrence *o=&instances[i];
        if(o->object==FOREST_ROOT_MAT || o->object==SWAMP_ROOTS)continue;
        float px=clampf(x,o->min.v[0],o->max.v[0]);
        float pz=clampf(z,o->min.v[2],o->max.v[2]);
        float dx=x-px,dz=z-pz;
        if(dx*dx+dz*dz<radius*radius)return false;
    }
    return true;
}
bool forest_stage_web(float x,float z,float width) {
    return forest_active && cemetery_stage_decal(CEM_DECAL_OPEN_GRAVE,x,z,width);
}
static bool ground_triangle(float x,float z,float ax,float az,float bx,float bz,float cx,float cz) {
    if(fabsf((bx-ax)*(cz-az)-(bz-az)*(cx-ax))<.1f)return false;
    float a=(x-bx)*(az-bz)-(ax-bx)*(z-bz),b=(x-cx)*(bz-cz)-(bx-cx)*(z-cz),c=(x-ax)*(cz-az)-(cx-ax)*(z-az);
    return !((a<0 || b<0 || c<0) && (a>0 || b>0 || c>0));
}
bool swamp_stage_pool(float x,float z,float radius,bool mud) {
    if(!swamp_active || wet_count>=64 || patch_count+12>=patch_capacity)return false;
    WetPatch p={.speed=mud?.48f:.62f};
    for(int i=0;i<12;i++) {
        float a=i*6.2831853f/12,r=radius*(.9f+.08f*sinf(i*2.1f+x*.03f+z*.01f));
        p.x[i]=lroundf(x+cosf(a)*r);p.z[i]=lroundf(z+sinf(a)*r*.78f);
        int c=(int)floorf((p.x[i]+swamp_cols*36)/72.0f),row=(int)floorf((swamp_rows*36-p.z[i])/72.0f);
        if(wall(swamp_grid,swamp_rows,swamp_cols,c,row))return false;
    }
    wet_patches[wet_count++]=p;
    for(int pass=0;pass<(mud?1:2);pass++)for(int i=0;i<12;i+=2) {
        int a=i,b=(i+1)%12,c=(i+2)%12,index=patch_count;
        float shrink=pass?.88f:1;
        int16_t xx[6]={lroundf(x),lroundf(x+(p.x[a]-x)*shrink),lroundf(x+(p.x[b]-x)*shrink),lroundf(x),lroundf(x+(p.x[b]-x)*shrink),lroundf(x+(p.x[c]-x)*shrink)};
        int16_t zz[6]={lroundf(z),lroundf(z+(p.z[a]-z)*shrink),lroundf(z+(p.z[b]-z)*shrink),lroundf(z),lroundf(z+(p.z[b]-z)*shrink),lroundf(z+(p.z[c]-z)*shrink)};
        floor_patch(xx,zz,pass?ROCK:CLIFF,pass?2:1);
        uint32_t tint=pass?0xC1CDC5FF:0x89928CFF;
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=tint;
    }
    return true;
}
float swamp_stage_speed(float x,float z) {
    if(!swamp_active)return 1;
    // The visibly dry route and timber deck always override wet areas below.
    for(int i=object_head[SWAMP_WALKWAY];i>=0;i=instances[i].next) {
        const Occurrence *o=&instances[i];float dx=x-o->x,dz=z-o->z;
        float lx=cosf(o->yaw)*dx+sinf(o->yaw)*dz,lz=-sinf(o->yaw)*dx+cosf(o->yaw)*dz;
        if(fabsf(lx)<=35 && fabsf(lz)<=22)return 1;
    }
    if(swamp_dry_field) {
        float fc=(x+swamp_cols*36)/24,fr=(z+swamp_rows*36)/24;
        int c=(int)floorf(fc),r=(int)floorf(fr),stride=swamp_cols*3+1;
        if(c>=0 && r>=0 && c<swamp_cols*3 && r<swamp_rows*3) {
            float tx=fc-c,tz=fr-r;int i=r*stride+c;
            float coverage=lerpf(lerpf(swamp_dry_field[i],swamp_dry_field[i+1],tx),lerpf(swamp_dry_field[i+stride],swamp_dry_field[i+stride+1],tx),tz);
            if(coverage>=128)return 1;
        }
    }
    float speed=1;
    for(int i=0;i<wet_count;i++) {
        const WetPatch *p=&wet_patches[i];bool inside=false;
        for(int a=0,b=11;a<12;b=a++)if(((p->z[a]>z)!=(p->z[b]>z)) && x<(p->x[b]-p->x[a])*(z-p->z[a])/(p->z[b]-p->z[a])+p->x[a])inside=!inside;
        if(inside && p->speed>.6f)return p->speed; // visible water is above mud
        if(inside && p->speed<speed)speed=p->speed;
    }
    return speed;
}
void swamp_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();swamp_active=true;swamp_grid=grid;swamp_rows=rows;swamp_cols=cols;
    textures[SOIL]=sprite_load("rom:/polish_swamp.sprite");
    textures[PAVING]=sprite_load("rom:/polish_trail.sprite");
    textures[CLIFF]=sprite_load("rom:/polish_swamp.sprite");
    textures[ROCK]=sprite_load("rom:/swamp_v1_water.sprite");
    textures[BARK]=sprite_load("rom:/cem_v3_bark.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_swamp_decal_lily.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_swamp_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_swamp_decal_algae.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_swamp_decal_mud.rgba16.sprite");
    const int req[]={SOIL,PAVING,CLIFF,ROCK,BARK,OPEN_GRAVE_PAINT,BONES_PAINT,BLOOD_PAINT,CRACKS_PAINT};
    for(unsigned i=0;i<sizeof(req)/sizeof(req[0]);i++)assertf(textures[req[i]],"Missing swamp material %d",req[i]);
    kit=t3d_model_load("rom:/swamp_v1_kit.t3dm");assertf(kit,"Missing swamp kit");
    static const char *const names[]={"swamp_tree_a","swamp_tree_b","swamp_tree_c","swamp_roots","swamp_log","swamp_walkway","swamp_shelter","swamp_fence"};
    for(int i=SWAMP_TREE_A;i<=SWAMP_FENCE;i++){objects[i]=t3d_model_get_object(kit,names[i-SWAMP_TREE_A]);assertf(objects[i],"Missing swamp object");}
    swamp_route_count=mapgen_floor(&swamp_routes);
    const int apron=7,hw=cols*36,hh=rows*36;
    int trail_quads=0;swamp_dry_field=path_union(swamp_routes,swamp_route_count,rows,cols,&trail_quads,true);
    patch_capacity=((rows+15)/2)*((cols+15)/2)+trail_quads+64*12+512;
    instance_capacity=(rows+14)*(cols+14)*3+768;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate swamp scene");
    for(int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2) {
        int index=patch_count;floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SOIL,0);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=0xD3DCD8FF;
    }
    int stride=cols*3+1;const int offsets[6]={0,1,stride+1,0,stride+1,stride};
    for(int r=0;r<rows*3;r++)for(int c=0;c<cols*3;c++) {
        int i=r*stride+c;
        if(!(swamp_dry_field[i] || swamp_dry_field[i+1] || swamp_dry_field[i+stride] || swamp_dry_field[i+stride+1]))continue;
        int index=patch_count;floor_rect(c*24-hw,r*24-hh,(c+1)*24-hw,(r+1)*24-hh,PAVING,3);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=0xD4D2C800u|swamp_dry_field[i+offsets[v]];
    }
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        if(!wall(grid,rows,cols,c,r))continue;
        bool edge=false;for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d]))edge=true;
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        float x=c*72+36-hw,z=hh-r*72-36;
        if(edge || h%3!=0)occurrence_scaled(SWAMP_TREE_A+h%3,x,0,z,(h%61)*.103f,.85f+(h%7)*.025f,.82f+((h>>4)%9)*.04f);
        if(edge || h%4==0)occurrence(SWAMP_ROOTS,x,0,z,(h%4)*1.5707963268f);
    }
    instance_capacity=instance_count+768;
    Occurrence *trim=realloc(instances,sizeof(Occurrence)*instance_capacity);assertf(trim,"Cannot retain swamp scene");instances=trim;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);assertf(matrices,"Cannot allocate swamp matrices");
    for(int i=0;i<instance_count;i++){Occurrence *o=&instances[i];t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});}
    debugf("[SWAMP] terrain=%d roots/trees=%d\n",patch_count,instance_count);
}
bool swamp_stage_place(SwampProp type,float x,float z,float yaw) {
    if(!swamp_active || (unsigned)type>=SWAMP_PROP_COUNT || instance_count>=instance_capacity)return false;
    // Fence runs along world X only; never turn it down the screen.
    occurrence(SWAMP_TREE_A+type,x,type==SWAMP_PROP_WALKWAY?1:0,z,type==SWAMP_PROP_FENCE?0:yaw);return true;
}
static void heart_surface(float x,float z,float rx,float rz,int mat,int y,uint32_t tint) {
    int16_t px[8],pz[8];
    for(int i=0;i<8;i++) {
        float a=i*.7853981634f,s=.91f+.07f*sinf(i*2.3f+x*.01f);
        px[i]=lroundf(x+cosf(a)*rx*s);pz[i]=lroundf(z+sinf(a)*rz*s);
    }
    for(int i=0;i<8;i+=2) {
        int a=i,b=(i+1)%8,c=(i+2)%8,index=patch_count;
        int16_t xx[6]={lroundf(x),px[a],px[b],lroundf(x),px[b],px[c]};
        int16_t zz[6]={lroundf(z),pz[a],pz[b],lroundf(z),pz[b],pz[c]};
        floor_patch(xx,zz,mat,y);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=tint;
    }
}
void heart_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();heart_active=true;
    textures[SOIL]=sprite_load("rom:/polish_bedrock.sprite");
    textures[PAVING]=sprite_load("rom:/polish_bedrock.sprite");
    textures[MOSS]=sprite_load("rom:/polish_swamp.sprite");
    textures[DOOR]=sprite_load("rom:/heart_v3_planks.sprite");
    textures[CLIFF]=sprite_load("rom:/cem_v3_cliff.sprite");
    textures[SLATE]=sprite_load("rom:/cem_v3_rock.sprite");
    textures[ROCK]=sprite_load("rom:/swamp_v1_water.sprite");
    textures[BARK]=sprite_load("rom:/cem_v3_bark.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_swamp_decal_algae.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_swamp_decal_algae.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_swamp_decal_algae.rgba16.sprite");
    const int req[]={SOIL,PAVING,MOSS,CLIFF,ROCK,BARK,SLATE,DOOR,OPEN_GRAVE_PAINT,BONES_PAINT,BLOOD_PAINT,CRACKS_PAINT};
    for(unsigned i=0;i<sizeof(req)/sizeof(req[0]);i++)assertf(textures[req[i]],"Missing cave material %d",req[i]);
    kit=t3d_model_load("rom:/heart_v1_kit.t3dm");assertf(kit,"Missing heart cave kit");
    static const char *const names[]={"heart_wall","heart_cap","heart_root","heart_rock_b","heart_rock_c","heart_bridge","heart_bank","heart_fence"};
    for(int i=HEART_WALL;i<=HEART_FENCE;i++){objects[i]=t3d_model_get_object(kit,names[i-HEART_WALL]);assertf(objects[i],"Missing heart cave object");}
    int apron=5,hw=cols*36,hh=rows*36;
    // Bound the two stone layers per blocked cell, plus shore detail and floor.
    patch_capacity=64;
    for(int r=0;r<rows;r++)for(int c=0;c<cols;c++)if(grid[r][c]!='#')patch_capacity+=5;
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        if(wall(grid,rows,cols,c,r))patch_capacity+=2;
        else if(grid[r][c]=='.') {
            unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
            if(h%3==0 && (wall(grid,rows,cols,c-1,r) || wall(grid,rows,cols,c+1,r) || wall(grid,rows,cols,c,r-1) || wall(grid,rows,cols,c,r+1)))patch_capacity+=8;
        }
    }
    instance_capacity=(rows+10)*(cols+10)*3+512;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate heart cave");
    for(int r=0;r<rows;r++)for(int c=0;c<cols;c++) {
        if(grid[r][c]=='#')continue;
        bool water=grid[r][c]=='^' || grid[r][c]=='=';int index=patch_count;
        int mat=water?ROCK:r<16?PAVING:SOIL;
        floor_rect(c*72-hw,hh-(r+1)*72,(c+1)*72-hw,hh-r*72,mat,water?-18:0);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=water?0x89AE8FFF:0xC2CED0FF;
        if(grid[r][c]=='=' && c==17 && r==17)occurrence(HEART_BRIDGE,(c+.5f)*72-hw,-6,hh-(r+.5f)*72,0);
        if(grid[r][c]=='=' && c==17) {
            float x=(c+.5f)*72-hw,z=hh-(r+.5f)*72;
            occurrence_scaled(HEART_FENCE,x-35,2,z,1.5707963268f,1.2f,1);
            occurrence_scaled(HEART_FENCE,x+35,2,z,-1.5707963268f,1.2f,1);
        }
    }
    // Irregular, continuous sediment field over exposed bedrock. One owner per
    // half-cell avoids circular decals and layered seams. Never cover the moat
    // or deck; their visual and blocking geometry remains exactly as approved.
    for(int r=0;r<rows;r++)for(int c=0;c<cols;c++) {
        if(grid[r][c]=='#' || grid[r][c]=='^' || grid[r][c]=='=')continue;
        for(int sy=0;sy<2;sy++)for(int sx=0;sx<2;sx++) {
            int x0=c*72-hw+sx*36,z0=hh-(r+1)*72+sy*36;
            const int off[6][2]={{0,0},{1,0},{1,1},{0,0},{1,1},{0,1}};
            uint8_t alpha[6];bool any=false;
            for(int v=0;v<6;v++) {
                float x=x0+off[v][0]*36,z=z0+off[v][1]*36;
                float broad=.55f*sinf(x*.010f+z*.003f)+.45f*cosf(z*.008f-x*.005f);
                float value=clampf((broad+.03f)*1.25f,0,1);value=value*value*(3-2*value);
                alpha[v]=lroundf(value*190);any|=alpha[v]>0;
            }
            if(any) {
                int index=patch_count;floor_rect(x0,z0,x0+36,z0+36,MOSS,1);
                for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=0xC4CDC200u|alpha[v];
            }
        }
    }
    // Stone floor follows only the remaining island cells, never the water.
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        float x=c*72+36-hw,z=hh-r*72-36;
        bool blocked=wall(grid,rows,cols,c,r),edge=false;
        if(c>=0 && r>=0 && c<cols && r<rows && grid[r][c]=='^')continue;
        int mask=0;
        for(int d=0;d<4;d++) {
            int nc=c+dc[d],nr=r+dr[d];
            bool neighbour_wall=wall(grid,rows,cols,nc,nr);
            if(neighbour_wall!=blocked){edge=true;mask|=1<<d;}
            if(nc>=0 && nr>=0 && nc<cols && nr<rows && grid[nr][nc]=='^' && (blocked || grid[r][c]!='='))occurrence(HEART_BANK,x,0,z,-d*1.5707963268f);
        }
        if(blocked) {
            // Tiny3D positive yaw turns local +Z toward -X; exposed east uses -90.
            for(int d=0;d<4;d++)if(mask&(1<<d))occurrence(HEART_WALL,x,0,z,-d*1.5707963268f);
            // Continuous opaque backing closes corner wedges beneath the boulders.
            floor_rect(x-36,z-36,x+36,z+36,SLATE,-1);
            int x0=x-36,x1=x+36,z0=z-36,z1=z+36;
            if(mask&1)z1=z;
            if(mask&2)x1=x;
            if(mask&4)z0=z;
            if(mask&8)x0=x;
            floor_rect(x0,z0,x1,z1,SLATE,46);
            int rock=h%3==0?HEART_CAP:h%3==1?HEART_ROCK_B:HEART_ROCK_C;
            float scale=edge?.9f:1.12f+(h%7)*.04f;
            occurrence_scaled(rock,x,edge?24:43,z,(h%4)*1.5707963268f,scale,.72f+((h>>8)%8)*.07f);
            if(edge && h%4==0)occurrence_scaled(HEART_ROOT,x,0,z,(h%4)*1.5707963268f,.8f,.75f);
        } else if(edge && h%3==0 && grid[r][c]=='.') {
            // Each shallow puddle fits inside its walkable tile, preserving routes.
            heart_surface(x,z,34,31,CLIFF,1,0x667C72FF);
            heart_surface(x,z,29,26,ROCK,2,0xA7BBB5FF);
        }
    }
    instance_capacity=instance_count+32;
    Occurrence *trim=realloc(instances,sizeof(Occurrence)*instance_capacity);assertf(trim,"Cannot retain cave instances");instances=trim;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);assertf(matrices,"Cannot allocate cave matrices");
    for(int i=0;i<instance_count;i++){Occurrence *o=&instances[i];t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});}
    debugf("[HEART] cave terrain=%d rock/root instances=%d\n",patch_count,instance_count);
}
static void snow_rock(int variant,float x,float y,float z,float yaw,float scale,float sy) {
    occurrence_scaled(SNOW_ROCK_A+variant,x,y,z,yaw,scale,sy);
    occurrence_scaled(SNOW_CAP_A+variant,x,y,z,yaw,scale,sy);
}
static float snow_edge_distance(float x,float z,float ax,float az,float bx,float bz) {
    float dx=bx-ax,dz=bz-az,l=dx*dx+dz*dz;
    float t=l>.01f?clampf(((x-ax)*dx+(z-az)*dz)/l,0,1):0;
    return hypotf(x-ax-t*dx,z-az-t*dz);
}
static uint8_t *path_union(const FloorQuad *floors,int nf,int rows,int cols,int *quads,bool marsh) {
    const int step=24;
    int nx=cols*3,nz=rows*3,stride=nx+1,hw=cols*36,hh=rows*36;
    uint8_t *field=calloc((nx+1)*(nz+1),1);assertf(field,"Cannot allocate snow trail union");
    for(int i=0;i<nf;i++) {
        const FloorQuad *source=&floors[i];if(source->kind==3 || (!marsh && source->kind!=1 && i%3!=0))continue;
        bool fan=false;
        for(int j=0;j<nf;j++)if(j!=i && floors[j].kind==source->kind && floors[j].x[0]==source->x[0] && floors[j].z[0]==source->z[0]){fan=true;break;}
        FloorQuad shape=*source;const FloorQuad *f=&shape;
        // Contract only the painted trail, preserving the playable space and routes.
        for(int v=0;v<6;v++) {
            int end=v==2 || v==4 || v==5;
            float cx=fan?source->x[0]:(source->x[end?2:0]+source->x[end?5:1])*.5f;
            float cz=fan?source->z[0]:(source->z[end?2:0]+source->z[end?5:1])*.5f;
            float scale=marsh?(fan?.84f:.86f):(fan?.6f:.62f);
            shape.x[v]=lroundf(cx+(source->x[v]-cx)*scale);
            shape.z[v]=lroundf(cz+(source->z[v]-cz)*scale);
        }
        int x0=f->x[0],x1=x0,z0=f->z[0],z1=z0;
        for(int v=1;v<6;v++){if(f->x[v]<x0)x0=f->x[v];if(f->x[v]>x1)x1=f->x[v];if(f->z[v]<z0)z0=f->z[v];if(f->z[v]>z1)z1=f->z[v];}
        int c0=clampf(floorf((x0+hw)/(float)step),0,nx),c1=clampf(ceilf((x1+hw)/(float)step),0,nx);
        int r0=clampf(floorf((z0+hh)/(float)step),0,nz),r1=clampf(ceilf((z1+hh)/(float)step),0,nz);
        for(int r=r0;r<=r1;r++)for(int c=c0;c<=c1;c++) {
            float x=c*step-hw,z=r*step-hh;
            bool inside=ground_triangle(x,z,f->x[0],f->z[0],f->x[1],f->z[1],f->x[2],f->z[2]) || ground_triangle(x,z,f->x[3],f->z[3],f->x[4],f->z[4],f->x[5],f->z[5]);
            if(!inside)continue;
            int a=fan?1:0,b=fan?2:5,d=fan?4:1,e=fan?5:2;
            float edge=fminf(snow_edge_distance(x,z,f->x[a],f->z[a],f->x[b],f->z[b]),snow_edge_distance(x,z,f->x[d],f->z[d],f->x[e],f->z[e]));
            float inset=(marsh?6:3.5f)+(marsh?4:2.5f)*sinf(x*.043f+z*.019f)+1.5f*sinf(z*.067f-x*.013f);
            // Keep snow at the fringe, with a clearer exposed-earth centre.
            float t=clampf((edge-inset)/(marsh?18:12),0,1);t=t*t*(3-2*t);
            uint8_t coverage=lroundf(255*t);
            if(coverage>field[r*stride+c])field[r*stride+c]=coverage;
        }
    }
    // Smooth shared coverage, never blend separate path layers at intersections.
    uint8_t *soft=calloc((nx+1)*(nz+1),1);assertf(soft,"Cannot soften snow trail union");
    for(int r=1;r<nz;r++)for(int c=1;c<nx;c++) {
        int sum=0;
        for(int dr=-1;dr<=1;dr++)for(int dc=-1;dc<=1;dc++)sum+=field[(r+dr)*stride+c+dc]*(dr==0?2:1)*(dc==0?2:1);
        int raw=field[r*stride+c],blur_weight=64-raw*raw*40/(255*255);
        soft[r*stride+c]=(raw*(256-blur_weight)+(sum/16)*blur_weight)/256;
    }
    free(field);field=soft;
    *quads=0;
    for(int r=0;r<nz;r++)for(int c=0;c<nx;c++)if(field[r*stride+c] || field[r*stride+c+1] || field[(r+1)*stride+c] || field[(r+1)*stride+c+1])(*quads)++;
    return field;
}
void snow_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();snow_active=true;
    textures[SOIL]=sprite_load("rom:/snow_v1_snow.sprite");
    textures[PAVING]=sprite_load("rom:/snow_v1_trail.sprite");
    textures[CLIFF]=sprite_load("rom:/cem_v3_cliff.sprite");
    textures[SLATE]=sprite_load("rom:/cem_v3_rock.sprite");
    textures[DOOR]=sprite_load("rom:/snow_v1_snow.sprite");
    textures[MASONRY]=sprite_load("rom:/cem_v3_masonry.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_snow_decal_ice.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_snow_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_snow_decal_blood.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_snow_decal_tracks.rgba16.sprite");
    const int req[]={SOIL,PAVING,CLIFF,SLATE,DOOR,MASONRY,OPEN_GRAVE_PAINT,BONES_PAINT,BLOOD_PAINT,CRACKS_PAINT};
    for(unsigned i=0;i<sizeof(req)/sizeof(req[0]);i++)assertf(textures[req[i]],"Missing snow material %d",req[i]);
    kit=t3d_model_load("rom:/snow_v1_kit.t3dm");assertf(kit,"Missing snowy pass kit");
    static const char *const names[]={"snow_face","snow_rock_a","snow_rock_b","snow_rock_c","snow_cap_a","snow_cap_b","snow_cap_c","snow_ruin","snow_bank"};
    for(int i=SNOW_FACE;i<=SNOW_BANK;i++){objects[i]=t3d_model_get_object(kit,names[i-SNOW_FACE]);assertf(objects[i],"Missing snow object %s",names[i-SNOW_FACE]);}
    const FloorQuad *floors;int nf=mapgen_floor(&floors),apron=7,hw=cols*36,hh=rows*36;
    int trail_quads=0;uint8_t *trail=path_union(floors,nf,rows,cols,&trail_quads,false);
    int blocked=0;for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++)if(wall(grid,rows,cols,c,r))blocked++;
    patch_capacity=((rows+15)/2)*((cols+15)/2)+trail_quads+blocked+256;
    instance_capacity=blocked*7+1024;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate snowy pass");
    for(int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2)floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SOIL,0);
    int stride=cols*3+1;
    for(int r=0;r<rows*3;r++)for(int c=0;c<cols*3;c++) {
        if(!(trail[r*stride+c] || trail[r*stride+c+1] || trail[(r+1)*stride+c] || trail[(r+1)*stride+c+1]))continue;
        int index=patch_count;floor_rect(c*24-hw,r*24-hh,(c+1)*24-hw,(r+1)*24-hh,PAVING,2);
        const int offsets[6]={0,1,stride+1,0,stride+1,stride};
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=0xDEE4E800u|trail[r*stride+c+offsets[v]];
    }
    free(trail);
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        if(!wall(grid,rows,cols,c,r))continue;
        float x=c*72+36-hw,z=hh-r*72-36;int mask=0;
        for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d]))mask|=1<<d;
        int exposed=0;for(int d=0;d<4;d++)if(mask&(1<<d))exposed++;
        bool spur=exposed>=2;
        if(!spur)for(int d=0;d<4;d++)if(mask&(1<<d))occurrence(SNOW_FACE,x,0,z,-d*1.5707963268f);
        floor_rect(x-36,z-36,x+36,z+36,SLATE,-1);
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        // Rounded snow volume replaces the visibly square raised tile cap.
        occurrence_scaled(SNOW_BANK,x,0,z,(h%4)*1.5707963268f,mask?.85f:1.4f,spur?.55f:.9f+((h>>9)%6)*.035f);
        float scale=spur?1.02f:mask?.9f+(h%5)*.015f:1.12f+(h%7)*.04f;
        snow_rock(h%3,x,spur?8:mask?24:43,z,(h%4)*1.5707963268f,scale,.78f+((h>>5)%7)*.045f);
    }
    instance_capacity=instance_count+768;
    Occurrence *trim=realloc(instances,sizeof(Occurrence)*instance_capacity);assertf(trim,"Cannot retain snow instances");instances=trim;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);assertf(matrices,"Cannot allocate snow matrices");
    for(int i=0;i<instance_count;i++){Occurrence *o=&instances[i];t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});}
    debugf("[SNOW] terrain=%d rock/cap instances=%d\n",patch_count,instance_count);
}
bool snow_stage_place(SnowProp type,float x,float z,float yaw,float scale) {
    if(!snow_active || (unsigned)type>=SNOW_PROP_COUNT || instance_count+2>instance_capacity)return false;
    if(type<=SNOW_PROP_ROCK_C)snow_rock(type,x,1,z,yaw,scale,scale);
    else occurrence_scaled(SNOW_RUIN,x,1,z,yaw,scale,scale);
    return true;
}
// Gameplay and rendering consume the same rounded polygon, not a larger circle.
float ice_stage_grip(float x,float z) {
    if(!ice_active)return 1;
    for(int p=0;p<ice_patch_count;p++) {
        const IcePatch *q=&ice_patches[p];bool inside=false;
        for(int i=0,j=11;i<12;j=i++)if(((q->z[i]>z)!=(q->z[j]>z)) &&
            x<(q->x[j]-q->x[i])*(z-q->z[i])/(float)(q->z[j]-q->z[i])+q->x[i])inside=!inside;
        if(inside)return .08f;
    }
    return 1;
}
static void ice_surface(float x,float z,float radius,unsigned seed) {
    if(ice_patch_count>=MG_CW*MG_CH)return;
    IcePatch *q=&ice_patches[ice_patch_count++];
    for(int i=0;i<12;i++) {
        float a=i*6.2831853f/12,rr=radius*(.92f+.07f*sinf(i*1.7f+seed%11));
        q->x[i]=lroundf(x+cosf(a)*rr);q->z[i]=lroundf(z+sinf(a)*rr*.78f);
    }
    for(int i=0;i<12;i+=2) {
        int j=(i+1)%12,k=(i+2)%12;
        int16_t xx[6]={lroundf(x),q->x[i],q->x[j],lroundf(x),q->x[j],q->x[k]};
        int16_t zz[6]={lroundf(z),q->z[i],q->z[j],lroundf(z),q->z[j],q->z[k]};
        floor_patch(xx,zz,PAVING,2);
    }
}
// A shared coverage field gives the exposed stone and frost connected, soft
// contours without stacking independently blended circles at room joins.
static uint8_t *ice_ground_cover(const char *const *grid,int rows,int cols,int *quads) {
    int nx=cols*3/2,nz=rows*3/2,stride=nx+1,plane=stride*(nz+1);
    uint8_t *field=calloc(plane*2,1);assertf(field,"Cannot allocate ice ground coverage");
    for(int r=0;r<=nz;r++)for(int c=0;c<=nx;c++) {
        float x=c*48-cols*36,z=r*48-rows*36;
        int gc=(int)floorf((x+cols*36)/72),gr=(int)floorf((rows*36-z)/72);
        if(wall(grid,rows,cols,gc,gr))continue;
        float stone=0,frost=0;
        for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
            MapVisualCell cell;if(!mapgen_visual_cell(mx,my,&cell))continue;
            float cx=(mx*MG_STEP+4)*72-cols*36,cz=rows*36-(my*MG_STEP+4)*72;
            float side=(mx+my+cell.index)%2?1:-1;
            float dx=x-cx,dz=z-cz;
            float bend=20*sinf(z*.021f+x*.008f)+12*sinf(x*.033f-z*.012f);
            float d=hypotf((dx+side*68+bend)/155,(dz-25+12*sinf(x*.018f))/185);
            float t=clampf((1-d)/.36f,0,1);t=t*t*(3-2*t);stone=fmaxf(stone,t);
            d=hypotf((dx-side*102-bend*.6f)/143,(dz+67+18*sinf(x*.014f))/111);
            t=clampf((1-d)/.45f,0,1);t=t*t*(3-2*t);frost=fmaxf(frost,t);
        }
        field[r*stride+c]=lroundf(stone*245);
        field[plane+r*stride+c]=lroundf(frost*(1-stone)*235);
    }
    *quads=0;
    for(int layer=0;layer<2;layer++)for(int r=0;r<nz;r++)for(int c=0;c<nx;c++) {
        int i=layer*plane+r*stride+c;
        if(field[i] || field[i+1] || field[i+stride] || field[i+stride+1])(*quads)++;
    }
    return field;
}
static void ice_ground_paint(const uint8_t *field,int rows,int cols) {
    int nx=cols*3/2,nz=rows*3/2,stride=nx+1,plane=stride*(nz+1);
    const int offsets[6]={0,1,stride+1,0,stride+1,stride};
    for(int layer=0;layer<2;layer++)for(int r=0;r<nz;r++)for(int c=0;c<nx;c++) {
        int i=layer*plane+r*stride+c;
        if(!(field[i] || field[i+1] || field[i+stride] || field[i+stride+1]))continue;
        int index=patch_count;
        floor_rect(c*48-cols*36,r*48-rows*36,(c+1)*48-cols*36,(r+1)*48-rows*36,layer?ROCK:MOSS,1);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=
            (layer?0xDCE6ED00u:0xADBCCB00u)|field[i+offsets[v]];
    }
}
void ice_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();ice_active=true;
    textures[SOIL]=sprite_load("rom:/ice_v1_floor.sprite");
    textures[PAVING]=sprite_load("rom:/ice_v1_slick.sprite");
    textures[MOSS]=sprite_load("rom:/cem_v3_rock.sprite");
    textures[ROCK]=sprite_load("rom:/snow_v1_snow.sprite");
    textures[CLIFF]=sprite_load("rom:/ice_v1_wall.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_ice_decal_frozen.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_ice_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_ice_decal_blood.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_ice_decal_crack.rgba16.sprite");
    const int req[]={SOIL,PAVING,MOSS,ROCK,CLIFF,OPEN_GRAVE_PAINT,BONES_PAINT,BLOOD_PAINT,CRACKS_PAINT};
    for(unsigned i=0;i<sizeof(req)/sizeof(req[0]);i++)assertf(textures[req[i]],"Missing ice material %d",req[i]);
    kit=t3d_model_load("rom:/ice_v1_kit.t3dm");assertf(kit,"Missing glacial cavern kit");
    static const char *const names[]={"ice_face","ice_mass_a","ice_mass_b","ice_mass_c","ice_fill"};
    for(int i=ICE_FACE;i<=ICE_FILL;i++){objects[i]=t3d_model_get_object(kit,names[i-ICE_FACE]);assertf(objects[i],"Missing ice object %s",names[i-ICE_FACE]);}
    int apron=7,hw=cols*36,hh=rows*36,blocked=0;
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++)if(wall(grid,rows,cols,c,r))blocked++;
    int cover_quads=0;uint8_t *cover=ice_ground_cover(grid,rows,cols,&cover_quads);
    patch_capacity=((rows+15)/2)*((cols+15)/2)+cover_quads+MG_CW*MG_CH*6+256;
    instance_capacity=blocked*6;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate glacial cavern");
    // One continuous base also covers the ground behind curved wall profiles.
    for(int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2)floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SOIL,0);
    ice_ground_paint(cover,rows,cols);free(cover);
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        if(!wall(grid,rows,cols,c,r))continue;
        float x=c*72+36-hw,z=hh-r*72-36;int mask=0;
        for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d]))mask|=1<<d;
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        for(int d=0;d<4;d++)if(mask&(1<<d))occurrence(ICE_FACE,x,0,z,-d*1.5707963268f);
        // Closed overlapping volumes fill blocked cells to beyond the camera.
        // Boundary fill stays within the collision tile, interior fill overlaps.
        occurrence_scaled(ICE_FILL,x,0,z,(h%4)*1.5707963268f,mask?1:1.3f,.85f+((h>>7)%7)*.035f);
        occurrence_scaled(ICE_MASS_A+h%3,x,48,z,(h%4)*1.5707963268f,mask?.85f:1.15f,.9f+((h>>5)%7)*.055f);
    }
    // Keep spawn/exit rooms dry and do not place slick ice beneath solid props.
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;if(!mapgen_visual_cell(mx,my,&cell) || cell.category>=3)continue;
        for(int attempt=0;attempt<9;attempt++) {
            int k=(attempt+cell.index)%9,c=mx*MG_STEP+2+k%3,r=my*MG_STEP+2+k/3;
            bool clear=true;
            for(int rr=r-1;rr<=r+1 && clear;rr++)for(int cc=c-1;cc<=c+1;cc++)
                if(wall(grid,rows,cols,cc,rr) || !strchr(".IHMVkogbv",grid[rr][cc])){clear=false;break;}
            if(!clear)continue;
            ice_surface(c*72+36-hw,hh-r*72-36,72+(cell.index%3)*8,mx+my*MG_CW);break;
        }
    }
    instance_capacity=instance_count;
    Occurrence *trim=realloc(instances,sizeof(Occurrence)*instance_capacity);assertf(trim,"Cannot retain ice instances");instances=trim;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);assertf(matrices,"Cannot allocate ice matrices");
    for(int i=0;i<instance_count;i++){Occurrence *o=&instances[i];t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});}
    debugf("[ICE] terrain=%d volumes=%d slick=%d\n",patch_count,instance_count,ice_patch_count);
}
static uint8_t *jarl_frost_cover(const char *const *grid,int rows,int cols,int *quads) {
    int nx=cols*3/2,nz=rows*3/2,stride=nx+1;
    uint8_t *field=calloc(stride*(nz+1),1);assertf(field,"Cannot allocate castle frost");
    for(int r=0;r<=nz;r++)for(int c=0;c<=nx;c++) {
        float x=c*48-cols*36,z=r*48-rows*36;
        int gc=floorf((x+cols*36)/72),gr=floorf((rows*36-z)/72);
        if(wall(grid,rows,cols,gc,gr))continue;
        float nearest=160;
        for(int rr=gr-2;rr<=gr+2;rr++)for(int cc=gc-2;cc<=gc+2;cc++) {
            if(!wall(grid,rows,cols,cc,rr))continue;
            float x0=cc*72-cols*36,z0=rows*36-(rr+1)*72;
            nearest=fminf(nearest,hypotf(x-clampf(x,x0,x0+72),z-clampf(z,z0,z0+72)));
        }
        float t=clampf((87-nearest+12*sinf(x*.027f+z*.011f)+7*sinf(z*.041f))/66,0,1);
        field[r*stride+c]=lroundf(t*t*(3-2*t)*220);
    }
    *quads=0;
    for(int r=0;r<nz;r++)for(int c=0;c<nx;c++) {
        int i=r*stride+c;if(field[i] || field[i+1] || field[i+stride] || field[i+stride+1])(*quads)++;
    }
    return field;
}
void jarl_stage_build(const char *const *grid,int rows,int cols) {
    cemetery_stage_release();jarl_active=true;
    textures[SOIL]=sprite_load("rom:/jarl_v1_floor.sprite");
    textures[PAVING]=sprite_load("rom:/jarl_v1_procession.sprite");
    textures[CLIFF]=sprite_load("rom:/jarl_v1_wall.sprite");
    textures[ROCK]=sprite_load("rom:/jarl_v1_cap.sprite");
    textures[MOSS]=sprite_load("rom:/snow_v1_snow.sprite");
    textures[BARK]=sprite_load("rom:/heart_v3_planks.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_frost_decal_ice.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_frost_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_frost_decal_blood.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_frost_decal_crack.rgba16.sprite");
    const int req[]={SOIL,PAVING,CLIFF,ROCK,MOSS,BARK,OPEN_GRAVE_PAINT,BONES_PAINT,BLOOD_PAINT,CRACKS_PAINT};
    for(unsigned i=0;i<sizeof(req)/sizeof(req[0]);i++)assertf(textures[req[i]],"Missing castle material %d",req[i]);
    kit=t3d_model_load("rom:/jarl_v1_kit.t3dm");assertf(kit,"Missing royal castle kit");
    static const char *const names[]={"jarl_wall","jarl_wall_1","jarl_wall_2","jarl_wall_3","jarl_pillar","jarl_table","jarl_ice_foot"};
    for(int i=JARL_WALL;i<=JARL_ICE_FOOT;i++){objects[i]=t3d_model_get_object(kit,names[i-JARL_WALL]);assertf(objects[i],"Missing castle object %s",names[i-JARL_WALL]);}
    int apron=7,hw=cols*36,hh=rows*36,blocked=0;
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++)if(wall(grid,rows,cols,c,r))blocked++;
    int frost_quads=0;uint8_t *frost=jarl_frost_cover(grid,rows,cols,&frost_quads);
    patch_capacity=((rows+15)/2)*((cols+15)/2)+blocked+frost_quads+rows+512;
    instance_capacity=blocked*5+128;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);instances=malloc(sizeof(Occurrence)*instance_capacity);
    assertf(patches && floor_vertices && instances,"Cannot allocate frozen castle");
    for(int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2)
        floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SOIL,0);
    // One ceremonial band: U spans the WHOLE 1.5-cell width, so the bronze
    // borders occur only on its outer sides, never as repeated narrow stripes.
    for(int r=3;r<=32;r++) {
        int index=patch_count;
        int center=16*72+36-hw;
        floor_rect(center-54,hh-(r+1)*72,center+54,hh-r*72,PAVING,1);
        const int uv[6][2]={{0,64},{64,64},{64,0},{0,64},{64,0},{0,0}};
        for(int i=0;i<6;i++){int16_t *st=t3d_vertbuffer_get_uv(floor_vertices,index*6+i);st[0]=uv[i][0]*32;st[1]=uv[i][1]*32;}
    }
    int nx=cols*3/2,nz=rows*3/2,stride=nx+1;const int off[6]={0,1,stride+1,0,stride+1,stride};
    for(int r=0;r<nz;r++)for(int c=0;c<nx;c++) {
        int i=r*stride+c;if(!(frost[i] || frost[i+1] || frost[i+stride] || frost[i+stride+1]))continue;
        int index=patch_count;floor_rect(c*48-hw,r*48-hh,(c+1)*48-hw,(r+1)*48-hh,MOSS,2);
        for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,index*6+v)=0xD5E3ED00u|frost[i+off[v]];
    }
    free(frost);
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        float x=c*72+36-hw,z=hh-r*72-36;
        if(!wall(grid,rows,cols,c,r)) {
            if(grid[r][c]=='p')occurrence(JARL_PILLAR,x,0,z,0);
            if(grid[r][c]=='t')occurrence(JARL_TABLE,x,0,z,0);
            continue;
        }
        floor_rect(x-36,z-36,x+36,z+36,ROCK,90);
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d])) {
            int left=(d+3)&3,right=(d+1)&3;
            int corner=(!wall(grid,rows,cols,c+dc[left],r+dr[left])?1:0)
                      |(!wall(grid,rows,cols,c+dc[right],r+dr[right])?2:0);
            occurrence(JARL_WALL+corner,x,0,z,-d*1.5707963268f);
            if((h+d*11)%4==0)occurrence(JARL_ICE_FOOT,x,0,z,-d*1.5707963268f);
        }
    }
    instance_capacity=instance_count;
    Occurrence *trim=realloc(instances,sizeof(Occurrence)*instance_capacity);assertf(trim,"Cannot retain castle objects");instances=trim;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_capacity);assertf(matrices,"Cannot allocate castle matrices");
    for(int i=0;i<instance_count;i++){Occurrence *o=&instances[i];t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});}
    debugf("[JARL] terrain=%d architecture=%d\n",patch_count,instance_count);
}
// Six new maps share this kit and the established grouped/culling renderer.
// All structures derive from the same grid used by world collision.
void expansion_stage_build(const char *const *grid,int rows,int cols,int campaign,int area) {
    cemetery_stage_release();expansion_active=true;
    bool desert=campaign==3;
    const char *soil=desert?(area==0?"rom:/ex_sand.sprite":area==1?"rom:/ex_limestone.sprite":"rom:/ex_basalt.sprite"):
        (area==2?"rom:/ex_mosaic.sprite":area==1?"rom:/forest_v6_ground.sprite":"rom:/cem_v3_soil.sprite");
    textures[SOIL]=sprite_load(soil);
    textures[PAVING]=sprite_load(desert?(area==2?"rom:/ex_basalt.sprite":"rom:/ex_limestone.sprite"):
        (area==2?"rom:/ex_mosaic.sprite":"rom:/ex_cobble.sprite"));
    textures[MOSS]=sprite_load(desert?"rom:/ex_sand.sprite":"rom:/cem_v3_soil.sprite");
    textures[CLIFF]=sprite_load(desert?"rom:/ex_sandstone.sprite":area==0?"rom:/ex_brick.sprite":"rom:/crypt_v1_wall.sprite");
    textures[ROCK]=sprite_load(desert?(area==0?"rom:/ex_sand.sprite":"rom:/ex_sandstone.sprite"):
        (area==0?"rom:/ex_slate.sprite":"rom:/crypt_v1_wall.sprite"));
    textures[BARK]=sprite_load("rom:/heart_v3_planks.sprite");
    textures[SLATE]=sprite_load("rom:/ex_slate.sprite");
    textures[IRON]=sprite_load("rom:/ex_window.sprite");
    textures[HEADSTONE_PAINT]=sprite_load("rom:/ex_door.sprite");
    textures[DOOR]=sprite_load("rom:/ex_tombdoor.sprite");
    textures[CROSS_PAINT]=sprite_load("rom:/ex_glass.sprite");
    textures[OPEN_GRAVE_PAINT]=sprite_load("rom:/scene_decal_open_grave.rgba16.sprite");
    textures[BONES_PAINT]=sprite_load("rom:/scene_decal_bones.rgba16.sprite");
    textures[BLOOD_PAINT]=sprite_load("rom:/scene_decal_blood.rgba16.sprite");
    textures[CRACKS_PAINT]=sprite_load("rom:/scene_decal_cracks.rgba16.sprite");
    for(int m=SOIL;m<MATERIAL_COUNT;m++)if(m!=MASONRY && m!=GRAVE_EARTH)
        assertf(textures[m],"Missing expansion material %d",m);
    kit=t3d_model_load("rom:/ex_v1_kit.t3dm");assertf(kit,"Missing expansion architecture");
    static const char *const names[]={"ex_cliff","ex_rock_0","ex_rock_1","ex_rock_2","ex_tomb_wall","ex_city_wall","ex_cathedral_wall",
        "ex_column","ex_pier","ex_obelisk","ex_tomb","ex_pew","ex_roof","ex_window","ex_door","ex_tombdoor","ex_glass","ex_pinnacle"};
    for(int i=EX_CLIFF;i<KIT_COUNT;i++){objects[i]=t3d_model_get_object(kit,names[i-EX_CLIFF]);assertf(objects[i],"Missing expansion object %s",names[i-EX_CLIFF]);}
    int apron=6,hw=cols*36,hh=rows*36,walkable=0,edges=0;
    const int dc[4]={0,1,0,-1},dr[4]={-1,0,1,0};
    for(int r=0;r<rows;r++)for(int c=0;c<cols;c++)if(!wall(grid,rows,cols,c,r))walkable++;
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++)if(wall(grid,rows,cols,c,r))
        for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d]))edges++;
    int cells=(rows+apron*2)*(cols+apron*2);
    patch_capacity=((rows+apron*2+1)/2)*((cols+apron*2+1)/2)+cells+(area<2?walkable*4:0)+256;
    instance_capacity=edges*3+cells/4+256;
    patches=malloc(sizeof(FloorPatch)*patch_capacity);floor_vertices=malloc_uncached(sizeof(T3DVertPacked)*3*patch_capacity);
    instances=malloc(sizeof(Occurrence)*instance_capacity);assertf(patches && floor_vertices && instances,"Cannot allocate expansion stage");
    // Underlay closes every edge beyond the camera; exposed floors own one tile.
    for(int r=-apron;r<rows+apron;r+=2)for(int c=-apron;c<cols+apron;c+=2)
        floor_rect(c*72-hw,hh-(r+2)*72,(c+2)*72-hw,hh-r*72,SOIL,-1);
    int height=desert?(area==0?64:80):96;
    for(int r=-apron;r<rows+apron;r++)for(int c=-apron;c<cols+apron;c++) {
        int x=c*72+36-hw,z=hh-r*72-36;
        unsigned hash=(unsigned)c*73856093u^(unsigned)r*19349663u;
        if(wall(grid,rows,cols,c,r)) {
            floor_rect(x-36,z-36,x+36,z+36,ROCK,height);
            if(desert && area==2 && c==18 && r==17)occurrence(EX_OBELISK,x,height,z,0);
            bool edge=false;
            for(int d=0;d<4;d++)if(!wall(grid,rows,cols,c+dc[d],r+dr[d])) {
                edge=true;
                int type=desert?(area==0?EX_CLIFF:EX_TOMB_WALL):(area==0?EX_CITY_WALL:EX_CATHEDRAL_WALL);
                float yaw=-d*1.5707963268f;
                occurrence(type,x,0,z,yaw);
                if(desert && area>0 && (hash+d)%4==0)occurrence(EX_TOMBDOOR,x,0,z,yaw);
                if(!desert && (hash+d)%3==0)
                    occurrence(area==0?((hash&1)?EX_WINDOW:EX_DOOR):EX_GLASS,x,0,z,yaw);
            }
            if(desert && area==0 && edge)occurrence_scaled(EX_ROCK_A+hash%3,x,64,z,(hash%8)*.785398f,1,1);
            if(!desert && area==0 && !(r&1) && !(c&1) &&
               wall(grid,rows,cols,c+1,r) && wall(grid,rows,cols,c,r+1) && wall(grid,rows,cols,c+1,r+1))
                occurrence(EX_ROOF,x+36,height,z-36,(hash&1)?0:1.5707963268f);
            if(!desert && area==2 && edge && hash%5==0)occurrence(EX_PINNACLE,x,height,z,0);
            continue;
        }
        char ch=grid[r][c];int mat=(ch=='+' || (!desert && area==2))?PAVING:SOIL;
        floor_rect(x-36,z-36,x+36,z+36,mat,0);
        if(ch=='p')occurrence(desert?EX_COLUMN:EX_PIER,x,0,z,0);
        if(ch=='t')occurrence(desert?EX_TOMB:EX_PEW,x,0,z,0);
        // Dust/sand encroaches in a single shared alpha field; no intersecting
        // broad transparent strips at junctions. Each half-tile has one owner.
        if(area<2)for(int sy=0;sy<2;sy++)for(int sx=0;sx<2;sx++) {
            int x0=x-36+sx*36,z0=z-36+sy*36;
            const int off[6][2]={{0,0},{1,0},{1,1},{0,0},{1,1},{0,1}};
            uint8_t alpha[6];bool any=false;
            for(int v=0;v<6;v++) {
                float px=x0+off[v][0]*36,pz=z0+off[v][1]*36;
                int vc=(int)floorf((px+hw)/72),vr=(int)floorf((hh-pz)/72);
                float distance=180;
                for(int yy=vr-1;yy<=vr+1;yy++)for(int xx=vc-1;xx<=vc+1;xx++)if(wall(grid,rows,cols,xx,yy)) {
                    float qx=xx*72+36-hw,qz=hh-yy*72-36;
                    float dx=fmaxf(0,fabsf(px-qx)-36),dz=fmaxf(0,fabsf(pz-qz)-36);
                    float d=sqrtf(dx*dx+dz*dz);if(d<distance)distance=d;
                }
                if(desert && area==0) {
                    // Sand encroaches from the actual paved/unpaved boundary,
                    // including isolated paving and intersections far from walls.
                    for(int yy=vr-2;yy<=vr+2;yy++)for(int xx=vc-2;xx<=vc+2;xx++) {
                        if(xx<0 || yy<0 || xx>=cols || yy>=rows || grid[yy][xx]=='+')continue;
                        float qx=xx*72+36-hw,qz=hh-yy*72-36;
                        float dx=fmaxf(0,fabsf(px-qx)-36),dz=fmaxf(0,fabsf(pz-qz)-36);
                        float d=sqrtf(dx*dx+dz*dz);if(d<distance)distance=d;
                    }
                }
                float wave=sinf(px*.039f+pz*.027f)*8+sinf(px*.013f-pz*.019f)*7;
                bool buried_road=desert && area==0;
                alpha[v]=(uint8_t)clampf((38+wave-distance)*(buried_road?7:5),0,buried_road?255:185);any|=alpha[v]>0;
            }
            if(any) {
                int idx=patch_count;floor_rect(x0,z0,x0+36,z0+36,MOSS,1);
                for(int v=0;v<6;v++)*t3d_vertbuffer_get_color(floor_vertices,idx*6+v)=0xFFFFFF00u|alpha[v];
            }
        }
    }
    // Retain the actual occurrences; the conservative construction allowance is
    // temporary. Keep room for 256 later ground paintings, not empty map grids.
    instance_capacity=instance_count;
    Occurrence *compact=realloc(instances,sizeof(Occurrence)*instance_capacity);assertf(compact,"Cannot retain expansion instances");instances=compact;
    int retained=patch_count+256;
    T3DVertPacked *verts=malloc_uncached(sizeof(T3DVertPacked)*3*retained);assertf(verts,"Cannot retain expansion floors");
    memcpy(verts,floor_vertices,sizeof(T3DVertPacked)*3*patch_count);free_uncached(floor_vertices);floor_vertices=verts;
    FloorPatch *floors=realloc(patches,sizeof(FloorPatch)*retained);assertf(floors,"Cannot retain expansion patches");patches=floors;patch_capacity=retained;
    matrices=malloc_uncached(sizeof(T3DMat4FP)*instance_count);assertf(matrices,"Cannot allocate expansion matrices");
    for(int i=0;i<instance_count;i++){Occurrence *o=&instances[i];t3d_mat4fp_from_srt_euler(&matrices[i],(float[]){o->scale,o->height_scale,o->scale},(float[]){0,o->yaw,0},(float[]){o->x,o->y,o->z});}
    debugf("[EXPANSION] campaign=%d phase=%d terrain=%d instances=%d\n",campaign,area,patch_count,instance_count);
}
static bool visible(T3DViewport *vp,const T3DVec3 *min,const T3DVec3 *max) {
    perf.cull_tests++;
    return !vp || t3d_frustum_vs_aabb(&vp->viewFrustum,min,max);
}
static void material(int index) {
    if(index>=HEADSTONE_PAINT) {
        // Existing 64x32 CI8 art fits TMEM. Clamp cutout edges, never mirror/repeat.
        rdpq_sprite_upload(TILE0,textures[index],NULL);
    } else rdpq_sprite_upload(TILE0,textures[index],&(rdpq_texparms_t){
        .s.repeats=REPEAT_INFINITE,.t.repeats=REPEAT_INFINITE,.s.mirror=true,.t.mirror=true
    });
}
static void draw_floor_material(int mat,T3DViewport *vp) {
    if(floor_head[mat]<0)return;
    bool lava=altar_active && mat==SLATE;
    bool ice_cover=(ice_active && (mat==MOSS || mat==ROCK)) || (jarl_active && mat==MOSS) || (expansion_active && mat==MOSS) || (heart_active && mat==MOSS);
    bool forest_blend=(forest_active && (mat==PAVING || mat==MOSS)) || ((snow_active || swamp_active) && mat==PAVING) || ice_cover;
    bool snow_alpha=((snow_active || swamp_active) && mat==PAVING) || ice_cover;
    if(forest_blend) {
        rdpq_mode_push();
        if(snow_alpha) {
            // Tiny3D fog writes its factor into vertex alpha; preserve trail coverage.
            t3d_fog_set_enabled(false);rdpq_mode_fog(0);
        }
        rdpq_mode_zbuf(true,false);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_mode_antialias(AA_NONE);
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0,ZERO,SHADE,ZERO),(ZERO,ZERO,ZERO,SHADE)));
    }
    if(lava) {
        float pulse=sinf(get_ticks_ms()*.0016f);
        rdpq_mode_push();rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
        rdpq_set_prim_color(RGBA32(255,(int)(225+20*pulse),(int)(205+20*pulse),255));
    }
    bool loaded=false;
    gfx_push_identity();
    for(int i=floor_head[mat];i>=0;i=patches[i].next) {
        const FloorPatch *p=&patches[i];
        T3DVec3 min={{p->min_x-2,p->y-1,p->min_z-2}},max={{p->max_x+2,p->y+1,p->max_z+2}};
        if(!visible(vp,&min,&max))continue;
        if(!loaded){material(mat);perf.textures++;loaded=true;}
        t3d_vert_load(floor_vertices+i*3,0,6);t3d_tri_draw_unindexed(0,2);t3d_tri_sync();
        perf.patches++;perf.triangles+=2;
    }
    t3d_matrix_pop(1);
    if(lava)rdpq_mode_pop();
    if(forest_blend)rdpq_mode_pop();
    if(snow_alpha)t3d_fog_set_enabled(g_dbg_fog);
}
void cemetery_stage_draw(float x,float z) {
    (void)x;(void)z;
    memset(&perf,0,sizeof(perf));
    if(!cemetery_stage_ready())return;
    uint64_t queued_at=get_ticks();
    perf.total_patches=patch_count;perf.total_objects=instance_count;
    perf.shared_posts=shared_fence_posts;
    T3DViewport *vp=t3d_viewport_get();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);rdpq_mode_blender(0);
    rdpq_mode_zbuf(true,true);rdpq_mode_alphacompare(0);
    t3d_state_set_drawflags(T3D_FLAG_SHADED|T3D_FLAG_TEXTURED|T3D_FLAG_DEPTH|T3D_FLAG_NO_LIGHT);
    static const int object_mat[KIT_COUNT]={CLIFF,ROCK,ROCK,ROCK,BARK,HEADSTONE_PAINT,CROSS_PAINT,MASONRY,
        MASONRY,MASONRY,SLATE,DOOR,IRON,MASONRY,SLATE,SLATE,SLATE,SLATE,SLATE,MASONRY,SLATE,SLATE,SLATE,
        ROCK,IRON,MASONRY,MASONRY,MASONRY,MASONRY,IRON,MASONRY,
        BARK,BARK,BARK,BARK,BARK,BARK,ROCK,BARK,BARK,BARK,BARK,BARK,BARK,BARK,BARK,BARK,CLIFF,SLATE,BARK,SLATE,SLATE,DOOR,CLIFF,BARK,
        [SNOW_FACE]=CLIFF,[SNOW_ROCK_A]=SLATE,[SNOW_ROCK_B]=SLATE,[SNOW_ROCK_C]=SLATE,
        [SNOW_CAP_A]=SOIL,[SNOW_CAP_B]=SOIL,[SNOW_CAP_C]=SOIL,[SNOW_RUIN]=MASONRY,[SNOW_BANK]=DOOR,
        [ICE_FACE]=CLIFF,[ICE_MASS_A]=CLIFF,[ICE_MASS_B]=CLIFF,[ICE_MASS_C]=CLIFF,[ICE_FILL]=CLIFF,
        [JARL_WALL]=CLIFF,[JARL_WALL_L]=CLIFF,[JARL_WALL_R]=CLIFF,[JARL_WALL_LR]=CLIFF,
        [JARL_PILLAR]=CLIFF,[JARL_TABLE]=BARK,[JARL_ICE_FOOT]=MOSS,
        [EX_CLIFF]=CLIFF,[EX_ROCK_A]=CLIFF,[EX_ROCK_B]=CLIFF,[EX_ROCK_C]=CLIFF,
        [EX_TOMB_WALL]=CLIFF,[EX_CITY_WALL]=CLIFF,[EX_CATHEDRAL_WALL]=CLIFF,
        [EX_COLUMN]=CLIFF,[EX_PIER]=CLIFF,[EX_OBELISK]=CLIFF,[EX_TOMB]=CLIFF,[EX_PEW]=BARK,[EX_ROOF]=SLATE,
        [EX_WINDOW]=IRON,[EX_DOOR]=HEADSTONE_PAINT,[EX_TOMBDOOR]=DOOR,[EX_GLASS]=CROSS_PAINT,[EX_PINNACLE]=CLIFF};

    // Complete opaque terrain first, including paths and cliff tops.
    for(int mat=0;mat<GRAVE_EARTH;mat++)if(!(swamp_active && mat==PAVING))draw_floor_material(mat,vp);
    // Dry trail cover blends over the already drawn mud/water, never under it.
    if(swamp_active)draw_floor_material(PAVING,vp);

    // Ground paintings cannot z-fight with the floor when depth is not compared.
    // They fit walkable terrain and are drawn BEFORE upright objects, which then
    // cover them normally. Leave the terrain's Z untouched for characters.
    rdpq_mode_push();
    rdpq_mode_zbuf(false,false);
    rdpq_mode_alphacompare(0);
    rdpq_mode_antialias(AA_NONE);
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER1((TEX0,ZERO,SHADE,ZERO),(ZERO,ZERO,ZERO,TEX0)));
    for(int mat=GRAVE_EARTH;mat<MATERIAL_COUNT;mat++)draw_floor_material(mat,vp);
    rdpq_mode_pop();

    // Upright volumes and cutouts occlude the ground paintings with their real
    // geometry; this avoids increasing the paintings' physical height as a bias.
    for(int mat=0;mat<GRAVE_EARTH;mat++) {
        bool loaded=false;
        rdpq_mode_zbuf(true,true);
        bool cutout=mat>=IRON || (expansion_active && mat==DOOR);
        rdpq_mode_alphacompare(cutout?128:0);
        if(cutout)
            rdpq_mode_combiner(RDPQ_COMBINER1((TEX0,ZERO,SHADE,ZERO),(ZERO,ZERO,ZERO,TEX0)));
        else rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
        for(int type=0;type<KIT_COUNT;type++) {
            if(object_mat[type]!=mat)continue;
            // Prelinked type lists preserve original order without rescanning every instance.
            for(int i=object_head[type];i>=0;i=instances[i].next) {
                const Occurrence *o=&instances[i];
                if(!visible(vp,&o->min,&o->max))continue;
                if(!loaded){material(mat);perf.textures++;loaded=true;}
                t3d_matrix_push(&matrices[i]);t3d_model_draw_object(objects[type],NULL);t3d_matrix_pop(1);
                // t3d_model_draw_object already synchronizes its triangle batches.
                perf.objects++;perf.triangles+=objects[type]->triCount;
            }
        }
    }
    rdpq_mode_alphacompare(0);
    rdpq_mode_zbuf(true,true);
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    t3d_state_set_drawflags(T3D_FLAG_SHADED|T3D_FLAG_DEPTH|gfx_cull_flag());
    perf.queue_us=(get_ticks()-queued_at)*1000000/TICKS_PER_SECOND;
}
