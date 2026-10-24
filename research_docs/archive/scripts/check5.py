import json, subprocess

UA = '-H', 'User-Agent: OpenSuperDEA-research/1.0 (mailto:research@example.com)'

def cr(url):
    try:
        out = subprocess.run(['curl', '-sL', '--max-time', '40', '-H', 'User-Agent: OpenSuperDEA-research/1.0 (research)', url],
                             capture_output=True, text=True, timeout=50).stdout
        try:
            d = json.loads(out)
        except Exception:
            return 'BADJSON ' + out[:120]
        items = d.get('message', {}).get('items', [])
        if not items:
            return 'NO-ITEMS'
        rows = []
        for m in items[:3]:
            t = (m.get('title') or [''])[0][:75]
            c = (m.get('container-title') or [''])[0][:38]
            y = m.get('published', {}).get('date-parts', [[None]])[0][0]
            rows.append(f'{t} | {c} | {y} | {m.get("DOI")} | vol{m.get("volume")}({m.get("issue")}) p{m.get("page")}')
        return '\n  '.join(rows)
    except Exception as e:
        return f'ERR {e}'

print('== B2. Aparicio-Pastor strong monotonicity Omega ==')
print(cr('https://api.crossref.org/works?query.bibliographic=strong+monotonicity+strongly+efficient+frontier+DEA+Aparicio+Pastor&rows=4&select=title,container-title,DOI,volume,issue,page,published'))
print('== C2. Mehdiloo-Podinovski frontiers ==')
print(cr('https://api.crossref.org/works?query.author=Mehdiloo&query.bibliographic=strong+weak+Farrell+efficient+frontiers+technologies&rows=4&select=title,container-title,DOI,volume,issue,page,published'))
print('== D2. Fukuyama p-norm ==')
print(cr('https://api.crossref.org/works?query.bibliographic=least+distance+p-norm+strong+monotonicity+input-output+substitutability&rows=4&select=title,container-title,DOI,volume,issue,page,published'))
print('== F. Vakili venue (Semantic Scholar) ==')
try:
    out = subprocess.run(['curl', '-sL', '--max-time', '30', '-H', 'User-Agent: OpenSuperDEA-research/1.0',
                          'https://api.semanticscholar.org/graph/v1/paper/search?query=Vakili%20Amirmoshiri%20Mirnia%20closest%20weak%20strong%20projections&fields=title,venue,year,externalIds,journal&limit=3'],
                         capture_output=True, text=True, timeout=40).stdout
    d = json.loads(out)
    for p in d.get('data', []):
        print(f"{p.get('title','')[:70]} | {p.get('venue','')} | {p.get('year')} | {p.get('externalIds',{}).get('DOI','')} | jnl={p.get('journal',{})}")
except Exception as e:
    print('ERR', e)
