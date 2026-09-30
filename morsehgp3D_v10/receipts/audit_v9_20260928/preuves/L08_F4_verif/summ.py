import json
for line in open('/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/batchA.jsonl'):
    line=line.strip()
    if not line.startswith('{'): continue
    r = json.loads(line)
    keys = [k for k in r if isinstance(r[k], dict)]
    a = r.get('as_is_lambda_eom'); c = r.get('hdbscan_conform_lambda_eom')
    al = r.get('as_is_lambda_leaf'); cl = r.get('hdbscan_conform_lambda_leaf')
    print(r['tag'][:40], r['conv'], 'roots', r['roots'], 'small', r['small_roots'], 'smass', r['small_root_mass'],
          '| eom as_is cl=%s tinysel=%s tinypts=%s ari=%.4f' % (a['clusters'], a['tiny_root_clusters'], a['tiny_root_points'], a['ari']),
          '| conform cl=%s ari=%.4f' % (c['clusters'], c['ari']),
          '| leaf as_is cl=%s tinypts=%s ari=%.4f / conf cl=%s ari=%.4f' % (al['clusters'], al['tiny_root_points'], al['ari'], cl['clusters'], cl['ari']))
