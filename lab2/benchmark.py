import json
from pathlib import Path
import random
import re
import statistics
import subprocess

root = Path(__file__).resolve().parent
counts = [1, 2, 4, 8, 10, 32, 128, 1024]
sizes = [400, 1000, 2000]
pattern = r'Потоки: (\d+); время: ([\d.]+) мс; площадь: ([\d.]+); точки: (\d+) (\d+) (\d+)'
samples = []
tables = []


def measure(n, p):
    output = subprocess.check_output([str(root / 'lab2'), '-t', str(p), '-n', str(n)], text=True)
    rows = re.findall(pattern, output)
    assert len(rows) == 2 and rows[0][2] == rows[1][2], output
    return {'n': n, 'threads': p, 'sequential_ms': float(rows[0][1]),
            'parallel_ms': float(rows[1][1]), 'area': float(rows[0][2])}


for n in sizes:
    print(f'n={n}: warmup', flush=True)
    for p in counts:
        measure(n, p)
    for repeat in range(5):
        order = counts.copy()
        random.Random(42 + repeat).shuffle(order)
        for p in order:
            samples.append(dict(measure(n, p), repeat=repeat + 1))
        print(f'n={n}: repeat {repeat + 1}/5', flush=True)
    baseline = statistics.median(s['sequential_ms'] for s in samples if s['n'] == n)
    rows = []
    for p in counts:
        elapsed = statistics.median(s['parallel_ms'] for s in samples if s['n'] == n and s['threads'] == p)
        rows.append({'threads': p, 'time_ms': elapsed, 'speedup': baseline / elapsed,
                     'efficiency': baseline / elapsed / p})
    tables.append({'n': n, 'sequential_ms': baseline, 'rows': rows})

(root / 'results-simple.json').write_text(json.dumps({'samples': samples, 'tables': tables}, indent=2) + '\n')
lines = ['# Замеры упрощённой версии', '',
         'Для каждого p выполнен прогрев и 5 измерений. Параллельное время — медиана 5 запусков.',
         'Последовательная база для каждого n — медиана 40 запусков. Каждый запуск сначала выполняет',
         'последовательный поиск, затем параллельный. Порядок p перемешан. Генерация и вывод исключены.', '']
for t in tables:
    lines += [f"## n = {t['n']}; последовательное время {t['sequential_ms']:.3f} мс", '',
              '| Потоки | Время, мс | Ускорение | Эффективность |', '|---:|---:|---:|---:|']
    lines += [f"| {r['threads']} | {r['time_ms']:.3f} | {r['speedup']:.3f} | {r['efficiency']:.4f} |" for r in t['rows']]
    lines.append('')
(root / 'results-simple.md').write_text('\n'.join(lines))
print('\n'.join(lines))
