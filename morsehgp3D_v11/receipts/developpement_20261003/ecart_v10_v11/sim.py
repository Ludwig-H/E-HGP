import heapq, sys
for name in sys.argv[1:]:
    lines=[l for l in open(name).read().split("\n")[1:] if not l.startswith("{")]
    t=[tuple(map(int,l.split())) for l in lines if l.strip()]
    times=[x[0] for x in t]; counts=[x[1] for x in t]
    tot=sum(times); srt=sorted(t,reverse=True)
    print(name,'tasks',len(t),'total %.2fs'%(tot/1e9),'max %.3fs'%(srt[0][0]/1e9),'top3',[(round(a/1e9,3),c,d) for a,c,d in srt[:3]], 'maxdepth', max(x[2] for x in t))
    for W in (8,24,48):
        heap=[0.0]*W
        for x in times:
            s=heapq.heappop(heap); heapq.heappush(heap,s+x)
        mk=max(heap)
        order=sorted(range(len(t)), key=lambda i:(-counts[i],i))
        heap=[0.0]*W
        for i in order:
            s=heapq.heappop(heap); heapq.heappush(heap,s+times[i])
        lpt=max(heap)
        print('   W%d makespan plan-order %.3fs  count-LPT %.3fs  ideal %.3fs'%(W,mk/1e9,lpt/1e9,tot/W/1e9))
