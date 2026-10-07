import os, re, sys
root = sys.argv[1]
pat = re.compile(rb'"descent_steps"')
for dp, dn, fn in os.walk(root):
    for f in fn:
        p = os.path.join(dp, f)
        if p.endswith(('.cpp', '.hpp', '.py', '.md')):
            continue
        try:
            data = open(p, 'rb').read()
        except Exception:
            continue
        if not pat.search(data):
            continue
        has10 = b'"k":10' in data or b'"k": 10' in data
        print(('K10 ' if has10 else '    '), len(data), p)
