import json, subprocess

def cr(url):
    try:
        out = subprocess.run(['curl', '-sL', '--max-time', '40', url], capture_output=True, text=True, timeout=50).stdout
        try:
            d = json.loads(out)
        except Exception:
            return 'BADJSON ' + out[:80]
        if 'message' not in d:
            return 'NONE'
        m = d['message']
        t = (m.get('title') or [''])[0][:80]
        c = (m.get('container-title') or [''])[0][:40]
        y = m.get('published', {}).get('date-parts', [[None]])[0][0]
        return f'{t} | {c} | {y} | {m.get("DOI")} | vol{m.get("volume")}({m.get("issue")}) p{m.get("page")}'
    except Exception as e:
        return f'ERR {e}'

print('== A. Charnes-RS DOI ==')
print(cr('https://api.crossref.org/works/10.1007/BF00158473'))
print('== B. Aparicio-Pastor 2013 Omega ==')
print(cr('https://api.crossref.org/works?query.bibliographic=Closest+targets+and+strong+monotonicity+strongly+efficient+frontier+Aparicio&rows=2&select=title,container-title,DOI,volume,issue,page,published'))
print('== C. Mehdiloo-Podinovski ==')
print(cr('https://api.crossref.org/works?query.author=Mehdiloo+Podinovski&query.title=frontiers+production+assumptions&rows=3&select=title,container-title,DOI,volume,issue,page,published'))
print('== D. Fukuyama 2014 p-norm ==')
print(cr('https://api.crossref.org/works?query.bibliographic=Input-output+substitutability+strongly+monotonic+p-norm+least+distance+Fukuyama&rows=2&select=title,container-title,DOI,volume,issue,page,published'))
print('== E. Links ==')
for name, url in [
    ('ORSJ Wu2009', 'https://orsj.org/wp-content/or-archives50/pdf/e_mag/52-2-105-111.pdf'),
    ('DEAFrontier Context', 'https://www.deafrontier.net/papers/OMEGAContext.pdf'),
    ('GRIPS EBM', 'https://grips.repo.nii.ac.jp/record/1021/files/DP09-21.pdf'),
    ('arXiv MinDS 2023', 'https://arxiv.org/abs/2312.15422'),
    ('Vakili S2 PDF', 'https://pdfs.semanticscholar.org/3a46/9bae80a328744069d51fe25e95aff784eb4f.pdf'),
    ('IJIM 2013', 'https://journals.srbiau.ac.ir/article_2148_59fc5d8bc1164ec7224d8f1cee1b68c6.pdf'),
    ('Springer JPA 2025 Lins', 'https://link.springer.com/article/10.1007/s11123-025-00767-1'),
]:
    try:
        r = subprocess.run(['curl', '-sI', '-L', '--max-time', '20', url], capture_output=True, text=True, timeout=30).stdout
        code = r.splitlines()[0] if r else 'NO RESP'
        print(f'{name}: {code}')
    except Exception as e:
        print(f'{name}: ERR {e}')
