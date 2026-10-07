import itertools, struct, sys, random
def w(name, pts):
    with open(name,'wb') as f:
        for p in pts: f.write(struct.pack('<3I',*p))
    print(name, len(pts))
w('n1.u32le',[(5,5,5)])
w('n2.u32le',[(5,5,5),(9,5,5)])
w('n3.u32le',[(5,5,5),(9,5,5),(5,9,7)])
# 24 points cospheriques : permutations de (+-2,+-1,0) x 100, centre (1000,1000,1000)
s=set()
for perm in itertools.permutations((2,1,0)):
    for sg in itertools.product((1,-1),repeat=3):
        s.add(tuple(1000+100*sg[i]*perm[i] for i in range(3)))
w('sphere24.u32le',sorted(s))
# 12 points : (0,+-1,+-2) cycliques (icosaedre de reseau, cospheriques)
s12=set()
for a,b in itertools.product((1,-1),repeat=2):
    s12.add((1000,1000+100*a,1000+200*b)); s12.add((1000+100*a,1000+200*b,1000)); s12.add((1000+200*b,1000,1000+100*a))
w('sphere12.u32le',sorted(s12))
# 16 points cospheriques + quelques points autour
rnd=random.Random(1)
s16=sorted(s)[:16]
w('sphere16.u32le',s16)
w('sphere24_plus.u32le',sorted(s)+[(1000+rnd.randint(-400,400),1000+rnd.randint(-400,400),1000+rnd.randint(-400,400)) for _ in range(40)])
w('line20.u32le',[(100+7*i,200,300) for i in range(20)])
w('grid6.u32le',[(100+10*i,100+10*j,100+10*k) for i in range(6) for j in range(6) for k in range(6)])
w('grid10.u32le',[(100+10*i,100+10*j,100+10*k) for i in range(10) for j in range(10) for k in range(10)])
w('plane_grid12.u32le',[(100+10*i,100+10*j,500) for i in range(12) for j in range(12)])
# cercle : 24 points de reseau sur x^2+y^2=325 dans le plan z=0
c=[(x,y) for x in range(-18,19) for y in range(-18,19) if x*x+y*y==325]
w('circle325.u32le',[(1000+x,1000+y,0) for x,y in c]+[(1000,1000,0)])
c65=[(x,y) for x in range(-9,10) for y in range(-9,10) if x*x+y*y==65]
w('circle65.u32le',[(1000+x,1000+y,0) for x,y in c65]+[(1000,1000,0)])
