import json
for line in open('/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/batchA.jsonl'):
    line=line.strip()
    if not line.startswith('{'): continue
    r = json.loads(line)
    out=[]
    for m in ('eom','leaf'):
        a=r['as_is_lambda_'+m]; f=r['fall_only_lambda_'+m]; c=r['hdbscan_conform_lambda_'+m]
        out.append('%s: as_is %d/%.3f tinypts=%d | fall_only %d/%.3f tinypts=%d | conform %d/%.3f' % (m, a['clusters'], a['ari'], a['tiny_root_points'], f['clusters'], f['ari'], f['tiny_root_points'], c['clusters'], c['ari']))
    print(r['tag'][:22], r['conv'][:3], 'roots', r['roots'], ' || '.join(out))
