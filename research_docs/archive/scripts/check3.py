import json, subprocess

def cr(url):
    try:
        out = subprocess.run(['curl', '-sL', url], capture_output=True, text=True, timeout=30).stdout
        d = json.loads(out)
        if 'message' not in d:
            return 'NONE'
        m = d['message']
        t = (m.get('title') or [''])[0][:85]
        c = (m.get('container-title') or [''])[0][:45]
        y = m.get('published', {}).get('date-parts', [[None]])[0][0]
        return f'{t} | {c} | {y} | {m.get("DOI")} | {m.get("volume")}-{m.get("issue")} p{m.get("page")}'
    except Exception as e:
        return f'ERR {e}'

print('== 1. Charnes-RS DOI ==')
print(cr('https://api.crossref.org/works/10.1007/BF00158473'))
print('== 2. Aparicio-Pastor 2013 Omega (bibliographic) ==')
print(cr('https://api.crossref.org/works?query.bibliographic=Closest+targets+and+strong+monotonicity+on+the+strongly+efficient+frontier+Aparicio+Pastor&rows=2&select=title,container-title,DOI,volume,issue,page,published'))
print('== 3. Mehdiloo-Podinovski EJOR ==')
print(cr('https://api.crossref.org/works?query.author=Mehdiloo+Podinovski&query.title=frontiers+technologies&rows=3&select=title,container-title,DOI,volume,issue,page,published'))
print('== 4. Link checks ==')
for name, url in [
    ('ORSJ Wu2009', 'https://orsj.org/wp-content/or-archives50/pdf/e_mag/52-2-105-111.pdf'),
    ('DEAFrontier Context', 'https://www.deafrontier.net/papers/OMEGAContext.pdf'),
    ('GRIPS EBM', 'https://grips.repo.nii.ac.jp/record/1021/files/DP09-21.pdf'),
    ('arXiv MinDS 2023', 'https://arxiv.org/abs/2312.15422'),
]:
    try:
        r = subprocess.run(['curl', '-sI', '-L', '--max-time', '20', url], capture_output=True, text=True, timeout=30).stdout
        code = r.splitlines()[0] if r else 'NO RESP'
        print(f'{name}: {code}')
    except Exception as e:
        print(f'{name}: ERR {e}')
