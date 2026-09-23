import json,glob,os
D='/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/d3_echelles'
E=[50,100,200,400,800,1600,3200,6400,12800]
files=sorted([x for x in glob.glob(D+'/attrib_s*.json') if 'scene' not in x])
for delta in (0.0,0.8,1.0):
  print(f'--- delta = {delta} us par fenetre chronometree (arete et rectangle)')
  for f in files:
    d=json.load(open(f))
    ce=[max(0,c-delta*1e3*e) for c,e in zip(d['cpu_ns'],d['edges'])]
    cr=[max(0,c-delta*1e3*e) for c,e in zip(d['rect_cpu_ns'],d['rect_count'])]
    tot=sum(ce)+sum(cr)
    row=[]
    for jthr in (3,4,5,6):
      g=sum(ce[jthr+1:])+sum(cr[jthr+1:])
      row.append(100*g/tot)
    long_edges=sum(d['edges'][6:]); 
    us_long=sum(ce[6:])/1e3/max(1,long_edges)
    us_short=sum(ce[:5])/1e3/max(1,sum(d['edges'][:5]))
    other=d['times_ms']['q2']+d['times_ms']['merge']+d['times_ms']['census']
    q34cpu=d['cpu_s']-other/1e3
    untimed=q34cpu-(sum(d['cpu_ns'])+sum(d['rect_cpu_ns']))/1e9
    print('%-40s tot=%5.1fs share h=.25/.5/1/2 : %5.1f %5.1f %5.1f %5.1f | us/arete longue %.2f courte %.1f | q34cpu~%.1f non chrono %.1f s = %.2f us/arete'%(os.path.basename(f)[7:-5],tot/1e9,*row,us_long,us_short,q34cpu,untimed,untimed*1e6/sum(d['edges'])))
