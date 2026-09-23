import json,sys
# bucket j couvre (edge[j-1], edge[j]] mm ; seuil global = 1.633 h ~ borne de bucket
E=[50,100,200,400,800,1600,3200,6400,12800]
print('%-44s %6s %8s %8s %8s %8s %9s %9s %8s'%('fichier','h(m)','seuil','edgeCPU','rectCPU','glob%','boules>h','%boules','gainmax'))
for f in sys.argv[1:]:
    d=json.load(open(f))
    tot_e=sum(d['cpu_ns']); tot_r=sum(d['rect_cpu_ns']); tot=tot_e+tot_r
    for jthr,h,hi in ((3,0.245,2),(4,0.49,4),(5,0.98,6),(6,1.96,8)):
        # arete > E[jthr] => global ; rectangles de gap > E[jthr] => global
        ge=sum(d['cpu_ns'][jthr+1:]); gr=sum(d['rect_cpu_ns'][jthr+1:])
        share=(ge+gr)/tot
        nb=d['balls_r_above_h'][hi]
        print('%-44s %6.2f %8d %7.1fs %7.1fs %7.1f%% %9d %8.2f%% %7.2fx'%(f.replace('attrib_','')[:44],h,E[jthr],tot_e/1e9,tot_r/1e9,100*share,nb,100*nb/d['balls'],1/share))
