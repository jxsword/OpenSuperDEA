import re
try:
    from pypdf import PdfReader
except Exception:
    from PyPDF2 import PdfReader
r = PdfReader('/tmp/deaR.pdf')
t = ''.join(p.extract_text() or '' for p in r.pages)
ms = sorted(set(re.findall(r'model_[a-z_]+', t)))
print('MODELS:', ms)
for kw in ['assurance', 'cone', 'weight restriction', 'context', 'malmquist', 'ebm', 'fuzzy', 'cross']:
    print(kw, t.lower().count(kw))
