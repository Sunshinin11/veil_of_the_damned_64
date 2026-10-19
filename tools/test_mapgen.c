// Teste no PC: gcc -Iinclude tools/test_mapgen.c src/mapgen.c -lm && ./a.out
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapgen.h"
static int walk(char ch){ return ch!='#'; }
static int blocking(char ch){ return strchr("#TXUuGRNpJFLQAI MBh", ch)!=NULL; }
int main(int argc,char**argv){
    int bad=0, maxfaces=0, maxfloor=0, minC=99;
    for (int seed=1; seed<=2000; seed++) for (int area=0; area<6; area++){
        srand(seed); int R,C; const char *const*m=mapgen_build(area,&R,&C);
        // conectividade a partir do E (4 vizinhos, sem atravessar props que bloqueiam)
        static int seen[64][64]; memset(seen,0,sizeof seen); int st[4096][2],sp=0, er=-1,ec=-1;
        int nE=0,nP=0,nq=0,nC=0,faces=0;
        for(int r=0;r<R;r++)for(int c=0;c<C;c++){char ch=m[r][c]; if(ch=='E'){er=r;ec=c;nE++;} if(ch=='P')nP++; if(ch=='q')nq++; if(ch=='C')nC++;
            if(ch=='#'){ if(r>0&&walk(m[r-1][c]))faces++; if(r<R-1&&walk(m[r+1][c]))faces++; if(c>0&&walk(m[r][c-1]))faces++; if(c<C-1&&walk(m[r][c+1]))faces++; }}
        if(nE!=1||nP!=1||nq!=3){printf("seed %d area %d: E%d P%d q%d\n",seed,area,nE,nP,nq);bad++;}
        if(faces>maxfaces)maxfaces=faces; if(nC<minC)minC=nC;
        seen[er][ec]=1; st[sp][0]=er; st[sp][1]=ec; sp++;
        while(sp){sp--; int r=st[sp][0],c=st[sp][1]; int d[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
            for(int k=0;k<4;k++){int nr=r+d[k][0],nc=c+d[k][1]; if(nr<0||nc<0||nr>=R||nc>=C||seen[nr][nc])continue; if(blocking(m[nr][nc]))continue; seen[nr][nc]=1; st[sp][0]=nr;st[sp][1]=nc;sp++;}}
        for(int r=0;r<R;r++)for(int c=0;c<C;c++) if(strchr("PCsSZqa",m[r][c])&&!seen[r][c]){ if(m[r][c]!='C'){printf("seed %d area %d: '%c' inacessivel %d,%d\n",seed,area,m[r][c],r,c);bad++;} }
        const FloorQuad*f; int nf=mapgen_floor(&f); if(nf>maxfloor)maxfloor=nf;
        if((seed<=2)&&argc>1){ for(int r=0;r<R;r++)puts(m[r]); printf("floor %d faces %d C %d\n\n",nf,faces,nC);}
    }
    printf("ruins=%d  max faces de muro=%d  max chao=%d  min baus=%d\n",bad,maxfaces,maxfloor,minC);
    return 0;
}
