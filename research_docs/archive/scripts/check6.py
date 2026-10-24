import json, subprocess

def cr(url):
    try:
        out = subprocess.run(['curl', '-sL', '--max-time', '40', '-H', 'User-Agent: OpenSuperDEA-research/1.0 (research)', url],
                             capture_output=True, text=True, timeout=50).stdout
        try:
            d = json.loads(out)
        except Exception:
            return 'BADJSON ' + out[:150]
        items = d.get('message', {}).get('items', [])
        if not items:
            return 'NO-ITEMS'
        rows = []
        for m in items[:6]:
            t = (m.get('title') or [''])[0][:80]
            a = ', '.join([x.get('family','') for x in (m.get('author') or [])[:4]])
            c = (m.get('container-title') or [''])[0][:35]
            y = m.get('published', {}).get('date-parts', [[None]])[0][0]
            rows.append(f'{t} | {a} | {c} | {y} | {m.get("DOI")} | v{m.get("volume")}({m.get("issue")}) p{m.get("page")}')
        return '\n  '.join(rows)
    except Exception as e:
        return f'ERR {e}'

print('== A1. Olyaie 作者全部论文 2013-2016 ==')
print(cr('https://api.crossref.org/works?query.author=Olyaie&filter=from-pub-date:2013-01-01,until-pub-date:2016-12-31&rows=8&select=title,author,container-title,DOI,volume,issue,page,published'))
print('== A2. Roshdi+Jahanshahloo 2014 合作 ==')
print(cr('https://api.crossref.org/works?query.author=Jahanshahloo+Roshdi&filter=from-pub-date:2013-01-01,until-pub-date:2015-12-31&rows=8&select=title,author,container-title,DOI,volume,issue,page,published'))
print('== B1. Vakili bibliographic ==')
print(cr('https://api.crossref.org/works?query.bibliographic=Vakili+Amirmoshiri+Mirnia+closest+weak+strong+efficient+projections&rows=4&select=title,author,container-title,DOI,volume,issue,page,published'))
