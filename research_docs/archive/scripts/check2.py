import json, subprocess, sys

# 1. Crossref: Charnes-Rousseau-Semple 1996
url = 'https://api.crossref.org/works?query.author=Charnes%20Rousseau%20Semple&query.title=frontier&filter=from-pub-date:1995-01-01,until-pub-date:1997-12-31&rows=6'
try:
    out = subprocess.run(['curl', '-s', url], capture_output=True, text=True, timeout=30).stdout
    d = json.loads(out)
    print('== CROSSREF ==')
    for i in d.get('message', {}).get('items', []):
        t = (i.get('title') or [''])[0][:90]
        c = (i.get('container-title') or [''])[0][:45]
        y = i.get('published', {}).get('date-parts', [[None]])[0][0]
        print(f'{t} | {c} | {y} | {i.get("DOI")}')
except Exception as e:
    print('CROSSREF ERR', e)

# 2. DEAPack tree
print('== DEAPACK_TREE ==')
try:
    out = subprocess.run(['gh', 'api', 'repos/daopingw/DEAPack/git/trees/main?recursive=1', '--jq', '.tree[].path'],
                         capture_output=True, text=True, timeout=30).stdout
    for line in out.splitlines():
        ll = line.lower()
        if 'cross' in ll or 'weak' in ll or 'preference' in ll or 'restrict' in ll or 'assurance' in ll:
            print(line)
except Exception as e:
    print('DEAPACK ERR', e)

# 3. deaR weights context
print('== DEAR_WEIGHTS_CTX ==')
with open('/tmp/deaR/R/model_basic.R', encoding='utf-8', errors='replace') as f:
    lines = f.readlines()
for n in range(430, 460):
    if 0 <= n < len(lines):
        print(f'{n+1}: {lines[n].rstrip()}')
