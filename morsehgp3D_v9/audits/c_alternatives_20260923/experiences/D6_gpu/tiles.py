# Statistiques de tuiles D3 pour dimensionner une execution GPU (memoire partagee 99 Kio/bloc).
# Entree : triplets u32 little-endian (mm). Aucune dependance externe.
import struct, sys, collections, math
def load(p):
    b=open(p,'rb').read(); n=len(b)//12
    return [struct.unpack_from('<3I',b,12*i) for i in range(n)]
def q(v,f):
    v=sorted(v); n=len(v)
    return v[min(n-1,int(f*n))]
for path in sys.argv[1:]:
    P=load(path); n=len(P)
    xs=[p[0] for p in P]; ys=[p[1] for p in P]; zs=[p[2] for p in P]
    print(path.split('/')[-1], 'n=',n, 'bbox_m=', [(max(a)-min(a))/1000 for a in (xs,ys,zs)])
    for T,h in ((250,200),(500,200),(500,500),(1000,500)):
        # tuiles cubiques de cote T mm ; halo h mm : on compte les points dans la tuile elargie (cube T+2h).
        tiles=collections.Counter((x//T,y//T,z//T) for x,y,z in P)
        # grille fine de pas g=50 mm pour sommer le halo exactement au pas pres (majorant)
        g=50
        fine=collections.Counter((x//g,y//g,z//g) for x,y,z in P)
        r=(h+g-1)//g
        tot=[];core=[]
        for (tx,ty,tz),c in tiles.items():
            s=0
            x0=(tx*T)//g-r; x1=((tx+1)*T-1)//g+r
            y0=(ty*T)//g-r; y1=((ty+1)*T-1)//g+r
            z0=(tz*T)//g-r; z1=((tz+1)*T-1)//g+r
            if (x1-x0+1)*(y1-y0+1)*(z1-z0+1) < len(fine):
                for X in range(x0,x1+1):
                    for Y in range(y0,y1+1):
                        for Z in range(z0,z1+1):
                            s+=fine.get((X,Y,Z),0)
            else:
                for (X,Y,Z),cc in fine.items():
                    if x0<=X<=x1 and y0<=Y<=y1 and z0<=Z<=z1: s+=cc
            tot.append(s); core.append(c)
        dup=sum(tot)/n
        print(f'  T={T}mm h={h}mm tuiles={len(tiles)} coeur p50/p99/max={q(core,.5)}/{q(core,.99)}/{max(core)} '
              f'tuile+halo p50/p99/max={q(tot,.5)}/{q(tot,.99)}/{max(tot)} duplication={dup:.2f} '
              f'part_tuiles_halo>2048={sum(1 for t in tot if t>2048)/len(tot):.3f} points_dans_ces_tuiles={sum(c for c,t in zip(core,tot) if t>2048)/n:.3f}')
