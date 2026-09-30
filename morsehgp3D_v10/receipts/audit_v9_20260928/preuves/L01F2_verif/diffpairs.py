import csv
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
def load(f): return {(r['scene'],r['seed']):r for r in csv.DictReader(open(B+f))}
s=load('tour_smoke.csv'); p=load('tour_z1b.csv')
for k in s:
    if abs(float(s[k]['ari'])-float(p[k]['ari']))>1e-12:
        print(k, s[k]['n'], s[k]['level'], s[k]['ari'], p[k]['ari'], s[k]['clusters'], p[k]['clusters'])
