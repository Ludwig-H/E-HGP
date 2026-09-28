import numpy as np
D='/workspaces/E-HGP/build/v9-q3-payload-scaling-inputs-20260926/'
K=5;T3,T4=K-1,K-2
rng=np.random.default_rng(7)
def counts(P,a,b):
    za=P-a; bz=b-P
    H=(za*bz).sum(1).astype(np.float64)
    cr=np.cross(b-a,za).astype(np.float64)
    Xi=(cr**2).sum(1)
    pos=H>0
    return (pos&(3*H*H>Xi)).sum(), (pos&(2*H*H>Xi)).sum()
for n,m in ((8000,20000),(16000,20000),(32000,20000)):
    P=np.fromfile(D+f'clusters_{n}.u32le',dtype='<u4').reshape(-1,3).astype(np.int64)
    lab=((P[:,0]>35000)*1)|((P[:,1]>35000)*2)|((P[:,2]>35000)*4)
    sizes=np.bincount(lab,minlength=8); inter=(n*n-(sizes**2).sum())//2
    x=rng.integers(0,n,4*m); y=rng.integers(0,n,4*m); keep=lab[x]!=lab[y]
    x=x[keep][:m]; y=y[keep][:m]
    s=0
    for i,j in zip(x,y):
        c3,c4=counts(P,P[i],P[j])
        s+= (c3<T3) or (c4<T4)
    p=s/m; se=np.sqrt(p*(1-p)/m)
    print(n,'inter surv frac',p,'+-',se,'est inter survivors',round(p*inter),'+-',round(se*inter))
