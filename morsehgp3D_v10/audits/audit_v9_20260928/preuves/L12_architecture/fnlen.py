import re,sys
# crude: find lines that end with '{' and look like function headers, track brace depth to find end
for path in sys.argv[1:]:
    lines=open(path,errors='ignore').read().split('\n')
    res=[]
    depth=0
    starts=[]
    for i,l in enumerate(lines):
        s=re.sub(r'//.*','',l)
        s=re.sub(r'"([^"\\]|\\.)*"','""',s)
        s=re.sub(r"'([^'\\]|\\.)*'","''",s)
        opens=s.count('{'); closes=s.count('}')
        if opens>closes and re.search(r'\)\s*(const)?\s*(noexcept)?\s*(->\s*[\w:<>]+)?\s*\{\s*$',s) and not re.match(r'\s*(if|for|while|switch|else|catch|do)\b',s) and not re.search(r'\[.*\]\s*\(.*\)\s*(mutable)?\s*\{\s*$',s) and '=' not in s.split('(')[0]:
            starts.append((i,depth,s.strip()[:90]))
        depth+=opens-closes
        while starts and depth<=starts[-1][1]:
            st=starts.pop()
            res.append((i-st[0]+1,st[0]+1,st[2]))
    res.sort(reverse=True)
    for r in res[:8]: print(f"{path}:{r[1]} {r[0]} lines  {r[2]}")
