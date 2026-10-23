#include "cemetery.h"
#include "cemetery_mesh.h"
#include "gfx.h"
#include "cemetery_painting_data.h"
#include <limits.h>

#define PAGE_BYTES 1360
#define TILE_CACHE 192
// Each key is one finished map piece: floor, cap, N/E/S/W face.
typedef struct { uint32_t first,key; uint16_t count; int16_t bb[4]; uint16_t location; } Piece;
typedef struct { int key; uint32_t used; } TileSlot;
typedef struct { int key; uint16_t normal; } Context;
static T3DVertPacked *verts;
static Piece *pieces;
static uint32_t nv,np,frame;
static bool writing;
static int rows,cols,side,page_count;
static const char *const *grid;
static FILE *source;
typedef struct { uint8_t family,c,r,mx,my; } TileRef;
static TileRef refs[MG_ROWS*MG_COLS];
typedef struct { TileRef ref; uint8_t face; uint16_t image; } Cut;
static Cut cuts[MG_ROWS*MG_COLS*6];
static uint8_t *cache;
static uint16_t *palette;
static uint32_t *page_index;
static uint16_t *page_ids;
static int stored_stride;

static TileSlot slots[TILE_CACHE];
static T3DMat4FP *model;
static float half_w,half_h;
#define IMAGE_MAX (MG_ROWS*MG_COLS*6)
typedef struct { uint16_t pages[4]; int16_t slot; uint8_t flips; } ImageRef;
static ImageRef images[IMAGE_MAX];
static uint16_t image_n;
static int32_t tile_heads[MG_ROWS*MG_COLS],*piece_next;
static uint32_t *visible,visible_n;
typedef struct {
    uint32_t uploads,texture_bytes,palette_uploads,draws,triangles,hits,misses,rom_bytes,waits,visited;
    uint64_t cpu_ticks;
} CemeteryStats;
static CemeteryStats stats;
static bool prewarming,preload_fenced;


// A tile refers to the matching cut of an approved painting, including its own
// palette. Geometry and tile locations come exclusively from the existing map.
static void build_refs(void) {
    static const int offset[5]={0,9,12,15,17};
    for(int r=0;r<rows;r++)for(int c=0;c<cols;c++) {
        int rx=c/7,ry=r/7;
        if(c>0&&c%7==0)rx--;
        if(r>0&&r%7==0)ry--;
        MapVisualCell cell={0};bool found=mapgen_visual_cell(rx,ry,&cell);
        if(!found&&c%7==0){rx=c/7;found=mapgen_visual_cell(rx,ry,&cell);}
        if(!found&&r%7==0){ry=r/7;found=mapgen_visual_cell(rx,ry,&cell);}
        int id=found?offset[cell.category]+cell.index:15;
        int lc=c-rx*7,lr=r-ry*7;
        lc=lc<0?0:lc>7?7:lc;lr=lr<0?0:lr>7?7:lr;
        int mx=found?(cell.mx^painted_mirror[id][0]):0;
        int my=found?(cell.my^painted_mirror[id][1]):0;
        refs[r*cols+c]=(TileRef){id,mx?7-lc:lc,my?7-lr:lr,mx,my};
    }
}
static bool source_blocked(int family,int c,int r) {
    return c>=0&&r>=0&&c<8&&r<8&&(painted_blocked[family][r]&(1u<<c));
}
// A representative painting can have different doors. Select an existing cut
// with the required role; the real grid, never the source image, opens a passage.
static TileRef matching_cut(TileRef ref,int surface,int face,int tile) {
    int best=INT_MAX,bc=ref.c,br=ref.r;
    bool walk=grid[tile/cols][tile%cols]!='#';
    static const int dc[6]={0,0,0,1,0,-1},dr[6]={0,0,-1,0,1,0};
    for(int r=0;r<8;r++)for(int c=0;c<8;c++) {
        bool blocked=source_blocked(ref.family,c,r),valid;
        if(surface==0)valid=walk?!blocked:true;
        else if(surface==1)valid=blocked&&
            ((r>0&&!source_blocked(ref.family,c,r-1))||(r<7&&!source_blocked(ref.family,c,r+1))||
             (c>0&&!source_blocked(ref.family,c-1,r))||(c<7&&!source_blocked(ref.family,c+1,r)));
        else {
            int x=c+dc[face],y=r+dr[face];
            valid=blocked&&x>=0&&y>=0&&x<8&&y<8&&!source_blocked(ref.family,x,y);
        }
        int distance=(c-ref.c)*(c-ref.c)+(r-ref.r)*(r-ref.r);
        if(valid&&distance<best){best=distance;bc=c;br=r;}
    }
    if(best==INT_MAX&&ref.family!=15) {
        ref.family=15;ref.mx=ref.my=0;
        return matching_cut(ref,surface,surface,tile);
    }
    assertf(best!=INT_MAX,"No matching approved Cemetery cut");
    ref.c=bc;ref.r=br;return ref;
}
static void build_cuts(void) {
    for(int tile=0;tile<rows*cols;tile++)for(int surface=0;surface<6;surface++) {
        TileRef ref=refs[tile];int face=surface;
        if(ref.my&&(face==2||face==4))face=6-face;
        if(ref.mx&&(face==3||face==5))face=8-face;
        ref=matching_cut(ref,surface,face,tile);face=surface;
        if(ref.my&&(face==2||face==4))face=6-face;
        if(ref.mx&&(face==3||face==5))face=8-face;
        cuts[tile*6+surface]=(Cut){ref,face,UINT16_MAX};
    }
}
// Image identity includes exact page indices and flips, independently of world location/palette.
static void build_images(void) {
    uint16_t *table=calloc(16384,sizeof(*table));assertf(table,"Cannot index Cemetery images");
    image_n=0;
    for(int key=0;key<rows*cols*6;key++) {
        int surface=key%6;Cut *cut=&cuts[key];TileRef ref=cut->ref;
        int f=((ref.family*64+ref.r*8+ref.c)*6+cut->face)*4;
        uint16_t ids[4];memcpy(ids,page_ids+f,sizeof(ids));
        int u=surface<2?ref.mx:(surface==3||surface==5)?ref.my:ref.mx;
        int v=surface<2?ref.my:0;uint8_t flips=u|(v<<1);
        uint32_t hash=2166136261u;
        for(int k=0;k<4;k++){hash^=ids[k];hash*=16777619u;}
        hash^=flips;unsigned slot=hash&16383u;
        while(table[slot]) {
            ImageRef *im=&images[table[slot]-1];
            if(im->flips==flips&&!memcmp(im->pages,ids,sizeof(ids)))break;
            slot=(slot+1)&16383u;
        }
        if(!table[slot]) {
            assertf(image_n<IMAGE_MAX,"Too many Cemetery images");
            ImageRef *im=&images[image_n];memcpy(im->pages,ids,sizeof(ids));im->flips=flips;im->slot=-1;
            table[slot]=++image_n;
        }
        cut->image=table[slot]-1;
    }
    free(table);
}
static void make_image(int id,uint8_t *out) {
    ImageRef *im=&images[id];uint8_t input[4*PAGE_BYTES],image[66*66];
    // Adjacent indexed pages are fetched in a single read; shared pages keep random access.
    for(int page=0;page<4;) {
        int run=1;
        while(page+run<4&&im->pages[page+run]==im->pages[page]+run)run++;
        assertf(im->pages[page]!=65535,"Missing Cemetery page");
        size_t bytes=run*stored_stride;
        assertf(fseek(source,im->pages[page]*stored_stride,SEEK_SET)==0,"Cemetery page seek failed");
        assertf(fread(input+page*stored_stride,1,bytes,source)==bytes,"Cemetery page missing");
        stats.rom_bytes+=bytes;page+=run;
    }
    int stride=side+2,row_stride=stored_stride==1156?34:40;
    for(int j=0;j<stride;j++)for(int i=0;i<stride;i++) {
        int x=i*(64/side),y=j*(64/side);
        if(x>65)x=65;
        if(y>65)y=65;
        if(im->flips&1)x=65-x;
        if(im->flips&2)y=65-y;
        int px=x>32,py=y>32;
        image[j*stride+i]=input[(py*2+px)*stored_stride+(y-py*32)*row_stride+x-px*32];
    }
    int pages=side/32;
    for(int py=0;py<pages;py++)for(int px=0;px<pages;px++)for(int y=0;y<34;y++) {
        uint8_t *row=out+(py*pages+px)*PAGE_BYTES+y*40;
        memcpy(row,image+(py*32+y)*stride+px*32,34);memset(row+34,0,6);
    }
}
static uint8_t *get_image(int id) {
    ImageRef *im=&images[id];
    if(im->slot>=0) {
        assertf(slots[im->slot].key==id,"Invalid Cemetery cache binding");
        slots[im->slot].used=frame;stats.hits++;
        return cache+im->slot*page_count*PAGE_BYTES;
    }
    stats.misses++;int selected=-1;uint32_t oldest=UINT_MAX;
    for(int i=0;i<TILE_CACHE;i++) {
        if(slots[i].key<0){selected=i;break;}
        if(slots[i].used<oldest){oldest=slots[i].used;selected=i;}
    }
    // Preserve the original in-flight safety window. Before drawing, one drain
    // protects all subsequent replacements; never skip a necessary drain mid-draw.
    if(slots[selected].key>=0&&slots[selected].used+3>=frame&&(!prewarming||!preload_fenced)) {
        rdpq_fence();rspq_wait();stats.waits++;
        if(prewarming)preload_fenced=true;
    }
    if(slots[selected].key>=0)images[slots[selected].key].slot=-1;
    uint8_t *out=cache+selected*page_count*PAGE_BYTES;make_image(id,out);
    data_cache_hit_writeback(out,page_count*PAGE_BYTES);
    slots[selected]=(TileSlot){id,frame};im->slot=selected;return out;
}
static CemPoint read_vertex(const T3DVertPacked *src,int i) {
    const T3DVertPacked *a=&src[i/2];const int16_t *p=(i&1)?a->posB:a->posA;
    uint32_t color=(i&1)?a->rgbaB:a->rgbaA;
    CemPoint v={.x=p[0],.y=p[1],.z=p[2]};
    for(int k=0;k<4;k++)v.c[k]=(color>>(24-k*8))&255;
    return v;
}
static void put_vertex(uint32_t i,const CemPoint *p,uint16_t normal) {
    T3DVertPacked *v=&verts[i/2];uint32_t color=0;
    for(int k=0;k<4;k++)color=(color<<8)|(uint32_t)clampf(p->c[k],0,255);
    int16_t pos[3]={(int16_t)lroundf(p->x*16),(int16_t)lroundf(p->y*16),(int16_t)lroundf(p->z*16)};
    int16_t st[2]={(int16_t)lroundf(p->u*32),(int16_t)lroundf(p->v*32)};
    if(i&1){memcpy(v->posB,pos,6);memcpy(v->stB,st,4);v->normB=normal;v->rgbaB=color;}
    else {memcpy(v->posA,pos,6);memcpy(v->stA,st,4);v->normA=normal;v->rgbaA=color;}
}
static void emit(const CemPoint *p,int n,int page,void *ctx) {
    Context *c=ctx;float area=0;
    for(int i=1;i<n-1;i++)area+=fabsf((p[i].u-p[0].u)*(p[i+1].v-p[0].v)-(p[i+1].u-p[0].u)*(p[i].v-p[0].v));
    if(area<.0001f)return;
    int count=(n-2)*3;
    if(writing) {
        int local=(page/4)*(side/32)+(page%4);
        Piece *q=&pieces[np];*q=(Piece){.first=nv,.key=c->key*page_count+local,.count=count,.location=c->key};
        float x0=p[0].x,x1=x0,z0=p[0].z,z1=z0;uint32_t out=nv;
        for(int i=1;i<n-1;i++){put_vertex(out++,&p[0],c->normal);put_vertex(out++,&p[i],c->normal);put_vertex(out++,&p[i+1],c->normal);}
        for(int i=1;i<n;i++){x0=fminf(x0,p[i].x);x1=fmaxf(x1,p[i].x);z0=fminf(z0,p[i].z);z1=fmaxf(z1,p[i].z);}
        q->bb[0]=floorf(x0);q->bb[1]=floorf(z0);q->bb[2]=ceilf(x1);q->bb[3]=ceilf(z1);
    }
    nv+=count;np++;
}
static void floor_triangle(CemPoint p[3],uint16_t normal) {
    float u0=1e9f,u1=-1e9f,v0=1e9f,v1=-1e9f;
    for(int i=0;i<3;i++) {
        p[i].u=(p[i].x+half_w)*side/72;p[i].v=(half_h-p[i].z)*side/72;
        u0=fminf(u0,p[i].u);u1=fmaxf(u1,p[i].u);v0=fminf(v0,p[i].v);v1=fmaxf(v1,p[i].v);
    }
    for(int r=(int)floorf(v0/side);r<(int)ceilf(v1/side);r++)for(int c=(int)floorf(u0/side);c<(int)ceilf(u1/side);c++) {
        if(c<0||r<0||c>=cols||r>=rows)continue;
        CemPoint a[12],b[12];memcpy(a,p,sizeof(CemPoint)*3);
        int n=cem_clip(a,3,b,0,c*side,1);if(n<3)continue;
        n=cem_clip(b,n,a,0,(c+1)*side,0);if(n<3)continue;
        n=cem_clip(a,n,b,1,r*side,1);if(n<3)continue;
        n=cem_clip(b,n,a,1,(r+1)*side,0);if(n<3)continue;
        for(int i=0;i<n;i++){a[i].u-=c*side;a[i].v-=r*side;}
        Context ctx={(r*cols+c)*6,normal};
        for(int i=1;i<n-1;i++){CemPoint t[3]={a[0],a[i],a[i+1]};cem_page_triangle(t,emit,&ctx);}
    }
}
static void generate(const T3DVertPacked *base,int nb,const T3DVertPacked *floor,int nf,
                     const T3DVertPacked *walls,int nw,const T3DVertPacked *tops,int nt) {
    for(int pass=0;pass<4;pass++) {
        const T3DVertPacked *src=pass==0?base:pass==1?floor:pass==2?walls:tops;
        int count=pass==0?nb:pass==1?nf:pass==2?nw:nt;
        for(int q=0;q<count;q++) {
            uint16_t normal=src[q*3].normA;
            int nx=(normal>>11)&31,nz=normal&31;if(nx>=16)nx-=32;if(nz>=16)nz-=32;
            CemPoint a=read_vertex(src,q*6),b=read_vertex(src,q*6+2);
            int c=(int)floorf(((a.x+b.x)*.5f+half_w-nx*.001f)/72);
            int r=(int)floorf((half_h-(a.z+b.z)*.5f+nz*.001f)/72);
            int surface=pass==3?1:nz>0?2:nx>0?3:nz<0?4:5;
            for(int t=0;t<2;t++) {
                CemPoint p[3];for(int k=0;k<3;k++)p[k]=read_vertex(src,q*6+t*3+k);
                if(pass<2){floor_triangle(p,normal);continue;}
                if(c<0||r<0||c>=cols||r>=rows)continue;
                for(int k=0;k<3;k++) {
                    if(pass==3){p[k].u=(p[k].x-(c*72-half_w))*side/72;p[k].v=(half_h-r*72-p[k].z)*side/72;}
                    else {p[k].u=(nx?(half_h-r*72-p[k].z):(p[k].x-(c*72-half_w)))*side/72;p[k].v=(44-p[k].y)*side/44;}
                }
                Context ctx={(r*cols+c)*6+surface,normal};cem_page_triangle(p,emit,&ctx);
            }
        }
    }
}
static int compare_piece(const void *aa,const void *bb) {
    const Piece *a=aa,*b=bb;
    int fa=cuts[a->key/page_count].ref.family,fb=cuts[b->key/page_count].ref.family;
    if(fa!=fb)return fa<fb?-1:1;
    return a->key<b->key?-1:a->key>b->key?1:0;
}
// Pack consecutive triangles using the same image page into a single RSP load.
static void copy_packed_vertex(T3DVertPacked *dst,uint32_t di,const T3DVertPacked *src,uint32_t si) {
    const T3DVertPacked *a=src+si/2;T3DVertPacked *b=dst+di/2;
    const int16_t *p=(si&1)?a->posB:a->posA,*uv=(si&1)?a->stB:a->stA;
    uint16_t n=(si&1)?a->normB:a->normA;uint32_t c=(si&1)?a->rgbaB:a->rgbaA;
    if(di&1){memcpy(b->posB,p,6);memcpy(b->stB,uv,4);b->normB=n;b->rgbaB=c;}
    else {memcpy(b->posA,p,6);memcpy(b->stA,uv,4);b->normA=n;b->rgbaA=c;}
}
static void batch_pieces(void) {
    uint32_t size=0,last=UINT_MAX;int count=0;
    for(uint32_t i=0;i<np;i++) {
        if(pieces[i].key!=last||count+pieces[i].count>60) {
            size=(size+1)&~1u;count=0;last=pieces[i].key;
        }
        size+=pieces[i].count;count+=pieces[i].count;
    }
    T3DVertPacked *packed=malloc_uncached(((size+1)/2)*sizeof(*packed));
    assertf(packed,"Cannot batch Cemetery pieces");
    memset(packed,0,((size+1)/2)*sizeof(*packed));
    uint32_t out=0,n=0;
    for(uint32_t i=0;i<np;i++) {
        Piece p=pieces[i];
        if(n==0||pieces[n-1].key!=p.key||pieces[n-1].count+p.count>60) {
            out=(out+1)&~1u;pieces[n]=p;pieces[n].first=out;pieces[n].count=0;n++;
        }
        Piece *b=&pieces[n-1];
        for(int k=0;k<p.count;k++)copy_packed_vertex(packed,out++,verts,p.first+k);
        b->count+=p.count;
        b->bb[0]=b->bb[0]<p.bb[0]?b->bb[0]:p.bb[0];b->bb[1]=b->bb[1]<p.bb[1]?b->bb[1]:p.bb[1];
        b->bb[2]=b->bb[2]>p.bb[2]?b->bb[2]:p.bb[2];b->bb[3]=b->bb[3]>p.bb[3]?b->bb[3]:p.bb[3];
    }
    free_uncached(verts);verts=packed;nv=out;np=n;
    Piece *smaller=realloc(pieces,np*sizeof(*pieces));if(smaller)pieces=smaller;
}
static int compare_material(const void *aa,const void *bb) {
    const Piece *a=aa,*b=bb;
    if(a->key!=b->key)return a->key<b->key?-1:1;
    return a->first<b->first?-1:a->first>b->first?1:0;
}
static int compare_visible(const void *aa,const void *bb) {
    uint32_t a=*(const uint32_t*)aa,b=*(const uint32_t*)bb;return a<b?-1:a>b?1:0;
}
static void build_spatial(void) {
    piece_next=malloc(np*sizeof(*piece_next));visible=malloc(np*sizeof(*visible));
    assertf(piece_next&&visible,"Cannot index Cemetery visibility");
    for(int i=0;i<rows*cols;i++)tile_heads[i]=-1;
    for(uint32_t i=0;i<np;i++) {
        Piece *p=&pieces[i];int tile=p->location/6;
        piece_next[i]=tile_heads[tile];tile_heads[tile]=i;
    }
}
static void prepare_visible(float px,float pz) {
    visible_n=0;
    int c0=(int)floorf((px-372+half_w)/72),c1=(int)floorf((px+372+half_w)/72);
    int r0=(int)floorf((half_h-pz-372)/72),r1=(int)floorf((half_h-pz+232)/72);
    if(c0<0)c0=0;
    if(r0<0)r0=0;
    if(c1>=cols)c1=cols-1;
    if(r1>=rows)r1=rows-1;
    for(int r=r0;r<=r1;r++)for(int c=c0;c<=c1;c++)for(int i=tile_heads[r*cols+c];i>=0;i=piece_next[i]) {
        Piece *p=&pieces[i];float m=(p->location%6)?72:0;stats.visited++;
        if(p->bb[2]<=px-300-m||p->bb[0]>=px+300+m||p->bb[3]<=pz-160-m||p->bb[1]>=pz+300+m)continue;
        visible[visible_n++]=i;
    }
    qsort(visible,visible_n,sizeof(*visible),compare_visible);
    prewarming=true;preload_fenced=false;uint32_t last=UINT_MAX;
    for(uint32_t i=0;i<visible_n;i++) {
        int id=(pieces[visible[i]].key/page_count)%image_n;
        if(last!=(uint32_t)id){get_image(id);last=id;}
    }
    prewarming=false;
}
void cemetery_release(void) {
    if(verts){rdpq_fence();rspq_wait();free_uncached(verts);}if(model)free_uncached(model);
    free(pieces);free(cache);free(palette);free(page_index);free(piece_next);free(visible);if(source)fclose(source);
    verts=NULL;pieces=NULL;cache=NULL;palette=NULL;page_index=NULL;page_ids=NULL;model=NULL;source=NULL;piece_next=NULL;visible=NULL;visible_n=0;nv=np=0;
}
bool cemetery_ready(void){return verts&&pieces&&cache&&palette&&page_index&&source&&model;}
void cemetery_build(const char *const *map,int rr,int cc,const T3DVertPacked *base,int nb,
                    const T3DVertPacked *floor,const FloorQuad *fq,int nf,
                    const T3DVertPacked *walls,int nw,const T3DVertPacked *tops,int nt) {
    cemetery_release();rows=rr;cols=cc;grid=map;half_w=cols*36;half_h=rows*36;
    (void)fq;side=is_memory_expanded()?64:32;page_count=(side/32)*(side/32);
    writing=false;generate(base,nb,floor,nf,walls,nw,tops,nt);
    size_t vb=((nv+1)/2)*sizeof(T3DVertPacked),pb=np*sizeof(Piece);
    verts=malloc_uncached(vb);pieces=malloc(pb);
    cache=malloc(TILE_CACHE*page_count*PAGE_BYTES);model=malloc_uncached(sizeof(*model));
    source=fopen("rom:/cem_opt_pages.ci8","rb");int pal_size=0;palette=asset_load("rom:/cem_painted_palettes.tlut",&pal_size);
    int index_size=0;page_index=asset_load("rom:/cem_opt_index.bin",&index_size);
    assertf(page_index&&index_size==58384&&page_index[0]==0x43454d32&&page_index[3]==7296,"Invalid Cemetery page index");
    stored_stride=page_index[1];page_ids=(uint16_t*)(page_index+4);
    assertf(stored_stride==1360||stored_stride==1156,"Invalid Cemetery page stride");
    assertf(cemetery_ready()&&pal_size==19*512,"Cannot load assembled Cemetery pieces");
    build_refs();build_cuts();
    for(int i=0;i<rows*cols*6;i++) {
        TileRef ref=cuts[i].ref;int f=((ref.family*64+ref.r*8+ref.c)*6+cuts[i].face)*4;
        for(int p=0;p<4;p++)assertf(page_ids[f+p]!=65535&&page_ids[f+p]<page_index[2],"Cemetery reference was removed");
    }
    build_images();
    data_cache_hit_writeback(palette,pal_size);memset(verts,0,vb);
    T3DMat4 m;t3d_mat4_identity(&m);t3d_mat4_scale(&m,1.0f/16,1.0f/16,1.0f/16);t3d_mat4_to_fixed(model,&m);
    nv=np=0;writing=true;generate(base,nb,floor,nf,walls,nw,tops,nt);writing=false;
    qsort(pieces,np,sizeof(Piece),compare_piece);
    batch_pieces();
    for(uint32_t i=0;i<np;i++) {
        Piece *p=&pieces[i];Cut *c=&cuts[p->location];
        p->key=(c->ref.family*image_n+c->image)*page_count+(p->key%page_count);
    }
    qsort(pieces,np,sizeof(*pieces),compare_material);build_spatial();
    vb=((nv+1)/2)*sizeof(*verts);pb=np*sizeof(*pieces);
    for(int i=0;i<TILE_CACHE;i++)slots[i]=(TileSlot){-1,0};
    frame=0;
    debugf("[CEM PIECES] %d px, mesh %lu, %lu pieces; 19 families; original geometry\n",side,(unsigned long)(vb+pb),(unsigned long)np);
}
void cemetery_draw(bool walls,float px,float pz) {
    if(!cemetery_ready())return;
    uint64_t started=get_ticks();
    if(!walls){frame++;memset(&stats,0,sizeof(stats));prepare_visible(px,pz);}
    rdpq_mode_push();rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);rdpq_mode_tlut(TLUT_RGBA16);
    int last_palette=-1;
    t3d_state_set_drawflags(T3D_FLAG_SHADED|T3D_FLAG_TEXTURED|T3D_FLAG_DEPTH);gfx_matrix_push(model);
    uint32_t last_texture=UINT_MAX;
    for(uint32_t n=0;n<visible_n;n++) {
        Piece *p=&pieces[visible[n]];
        if(((p->location%6)!=0)!=walls)continue;
        int material=p->key/page_count,family=material/image_n,id=material%image_n;
        if(last_palette!=family) {
            rdpq_tex_upload_tlut(palette+family*256,0,256);last_palette=family;stats.palette_uploads++;
        }
        uint32_t texture=id*page_count+p->key%page_count;
        if(last_texture!=texture) {
            uint8_t *tile=get_image(id);
            surface_t page=surface_make(tile+(p->key%page_count)*PAGE_BYTES,FMT_CI8,34,34,40);
            rdpq_tex_upload(TILE0,&page,NULL);last_texture=texture;
            stats.uploads++;stats.texture_bytes+=PAGE_BYTES;
        }
        int skip=p->first&1;t3d_vert_load(verts+p->first/2,0,(p->count+skip+1)&~1);
        t3d_tri_draw_unindexed(skip,p->count/3);t3d_tri_sync();
        stats.draws++;stats.triangles+=p->count/3;
    }
    t3d_matrix_pop(1);rdpq_mode_pop();rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    t3d_state_set_drawflags(T3D_FLAG_SHADED|T3D_FLAG_DEPTH|gfx_cull_flag());
    stats.cpu_ticks+=get_ticks()-started;
    if(walls&&frame%120==0)debugf("[CEMPERF] uploads=%lu bytes=%lu pal=%lu draw=%lu tri=%lu hit=%lu miss=%lu ROM=%lu waits=%lu visited=%lu CPUus=%lu\n",
        (unsigned long)stats.uploads,(unsigned long)stats.texture_bytes,(unsigned long)stats.palette_uploads,
        (unsigned long)stats.draws,(unsigned long)stats.triangles,(unsigned long)stats.hits,(unsigned long)stats.misses,
        (unsigned long)stats.rom_bytes,(unsigned long)stats.waits,(unsigned long)stats.visited,
        (unsigned long)(stats.cpu_ticks*1000000/TICKS_PER_SECOND));
}
