import numpy as np, itertools, sys
D='/workspaces/E-HGP/build/v9-q3-payload-scaling-inputs-20260926/'
K=5
T3,T4=K-1,K-2
rng=np.random.default_rng(1)
def witnesses(P,a,b):
    # strict witnesses: H>0 and alpha*H^2 > Xi (exact integer arithmetic in int64/obj ok: coords < 2^16)
    za=P-a; bz=b-P
    H=(za*bz).sum(1)
    d=b-a
    cr=np.cross(np.broadcast_to(d,za.shape),za)
    Xi=(cr.astype(object)**2).sum(1) if False else (cr.astype(np.float64)**2).sum(1)
    Hf=H.astype(np.float64)
    pos=H>0
    w3=pos & (3*Hf*Hf>Xi)
    w4=pos & (2*Hf*Hf>Xi)
    return w3.sum(), w4.sum(), w3, w4
for n in (8000,16000,32000):
    P=np.fromfile(D+f'clusters_{n}.u32le',dtype='<u4').reshape(-1,3).astype(np.int64)
    lab=((P[:,0]>35000).astype(int))|((P[:,1]>35000).astype(int)<<1)|((P[:,2]>35000).astype(int)<<2)
    sizes=np.bincount(lab,minlength=8)
    inter=(n*n-(sizes**2).sum())//2
    print(n,'sizes',sorted(sizes.tolist()),'inter',inter, 'extent per cluster', [ (P[lab==c].max(0)-P[lab==c].min(0)).tolist() for c in range(1)])
    # for each cluster pair: extremal facing pair and its witness counts
    alive=0
    for i,j in itertools.combinations(range(8),2):
        A=P[lab==i]; B=P[lab==j]
        u=B.mean(0)-A.mean(0)
        a=A[np.argmax(A@u)]; b=B[np.argmin(B@u)]
        c3,c4,_,_=witnesses(P,a,b)
        s3=c3<T3; s4=c4<T4
        alive+= (s3 or s4)
    print('  cluster pairs with a surviving facing-extremal pair:',alive,'/ 28')
    if n==8000:
        # sample random inter-cluster pairs and count survivors, plus cone check
        m=4000; surv=0; conefail=0
        idx=np.arange(n)
        for t in range(m):
            while True:
                x,y=rng.integers(0,n,2)
                if lab[x]!=lab[y]: break
            a=P[x]; b=P[y]
            c3,c4,w3,w4=witnesses(P,a,b)
            if c3<T3 or c4<T4: surv+=1
            # cone check for witnesses of q3 near a: angle at a between (z-a) and (b-a) < 60 deg
            Z=P[w3]
            if len(Z):
                v=Z-a; ab=b-a
                cos=(v@ab)/np.linalg.norm(v,axis=1)/np.linalg.norm(ab)
                if (cos<=0.5-1e-12).any(): conefail+=1
        print('  sampled inter pairs',m,'surviving fraction',surv/m,'=> est. inter survivors',surv/m*inter,' cone violations',conefail)
