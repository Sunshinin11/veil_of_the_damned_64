#ifndef CEMETERY_MESH_H
#define CEMETERY_MESH_H
// Portable visual geometry helpers; no RNG, collision or gameplay dependencies.
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef struct { float x,y,z,u,v,c[4]; } CemPoint;
typedef void (*CemEmit)(const CemPoint *, int, int, void *);

static CemPoint cem_mix(CemPoint a, CemPoint b, float t) {
    CemPoint p;
    p.x=a.x+(b.x-a.x)*t; p.y=a.y+(b.y-a.y)*t; p.z=a.z+(b.z-a.z)*t;
    p.u=a.u+(b.u-a.u)*t; p.v=a.v+(b.v-a.v)*t;
    for(int k=0;k<4;k++) p.c[k]=a.c[k]+(b.c[k]-a.c[k])*t;
    return p;
}
static int cem_clip(const CemPoint *in,int n,CemPoint *out,int axis,float edge,int keep_high) {
    int m=0;
    for(int i=0;i<n;i++) {
        CemPoint a=in[i], b=in[(i+1)%n];
        float da=(axis?a.v:a.u)-edge, db=(axis?b.v:b.u)-edge;
        int ia=keep_high?da>=0:da<=0, ib=keep_high?db>=0:db<=0;
        if(ia) out[m++]=a;
        if(ia!=ib) {
            CemPoint p=cem_mix(a,b,da/(da-db));
            if(axis) p.v=edge; else p.u=edge;
            out[m++]=p;
        }
    }
    return m;
}
// Split the existing triangles at page boundaries, keeping their interpolated colors.
// UVs are in texels; each emitted polygon references a 32x32 core + 1px gutters.
static void cem_page_triangle(const CemPoint tri[3],CemEmit emit,void *context) {
    float lo[2]={tri[0].u,tri[0].v}, hi[2]={lo[0],lo[1]};
    for(int i=1;i<3;i++) {
        lo[0]=fminf(lo[0],tri[i].u); lo[1]=fminf(lo[1],tri[i].v);
        hi[0]=fmaxf(hi[0],tri[i].u); hi[1]=fmaxf(hi[1],tri[i].v);
    }
    int x0=(int)floorf(lo[0]/32), y0=(int)floorf(lo[1]/32);
    int x1=(int)ceilf(hi[0]/32), y1=(int)ceilf(hi[1]/32);
    for(int y=y0;y<y1;y++) for(int x=x0;x<x1;x++) {
        CemPoint a[12],b[12]; memcpy(a,tri,3*sizeof(*a));
        int n=cem_clip(a,3,b,0,x*32,1); if(n<3) continue;
        n=cem_clip(b,n,a,0,(x+1)*32,0); if(n<3) continue;
        n=cem_clip(a,n,b,1,y*32,1); if(n<3) continue;
        n=cem_clip(b,n,a,1,(y+1)*32,0); if(n<3) continue;
        for(int i=0;i<n;i++) { a[i].u=a[i].u-x*32+1; a[i].v=a[i].v-y*32+1; }
        emit(a,n,((y&3)*4+(x&3)),context);
    }
}
#endif
