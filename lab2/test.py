import itertools
import math
from pathlib import Path
import re
import subprocess
import sys

binary = str(Path(sys.argv[1] if len(sys.argv) > 1 else './lab2').resolve())
pattern = r'Потоки: (\d+); время: ([\d.]+) мс; площадь: ([\d.]+); точки: (\d+) (\d+) (\d+)'
checks = 0


def run(args, success=True):
    global checks
    result = subprocess.run([binary, *map(str, args)], capture_output=True, text=True)
    assert (result.returncode == 0) == success, result.stdout + result.stderr
    checks += 1
    return re.findall(pattern, result.stdout)


def area(points, ids):
    a, b, c = [sum((x - y) ** 2 for x, y in zip(points[u], points[v]))
               for u, v in itertools.combinations(ids, 2)]
    return math.sqrt(max(0, 2 * (a*b + a*c + b*c) - a*a - b*b - c*c)) / 4


for n in [3, 4, 10, 25, 51]:
    points = [(i * 17 % 2001 - 1000,
               i * i * 13 % 2001 - 1000,
               (i * i * 7 + i * 31) % 2001 - 1000) for i in range(n)]
    expected = max(area(points, ids) for ids in itertools.combinations(range(n), 3))
    for p in [1, 2, 4, 8, 10, 32, 128, 1024]:
        rows = run(['-t', p, '-n', n])
        assert len(rows) == 2
        assert [int(row[0]) for row in rows] == [1, p]
        for _, elapsed, actual, a, b, c in rows:
            ids = [int(a) - 1, int(b) - 1, int(c) - 1]
            assert 0 <= ids[0] < ids[1] < ids[2] < n
            assert float(elapsed) >= 0
            assert math.isclose(float(actual), expected, rel_tol=1e-12, abs_tol=1e-9)
            assert math.isclose(area(points, ids), expected, rel_tol=1e-12, abs_tol=1e-9)
for args in [[], ['-t', 0, '-n', 3], ['-t', 1025, '-n', 3], ['-t', 1, '-n', 2],
             ['-t', 1, '-n', 5001], ['-t', '', '-n', 3], ['-t', '1x', '-n', 3],
             ['-t', -1, '-n', 3], ['-t', '999999999999999999999', '-n', 3],
             ['', 1, '-n', 3], ['-', 1, '-n', 3], ['-tt', 1, '-n', 3],
             ['-t', 1, '', 3], ['-t', 1, '-', 3], ['-n', 3, '-t', 1]]:
    run(args, success=False)
rows = run(['-t', 10, '-n', 1000])
assert len(rows) == 2 and rows[0][2] == rows[1][2]
print(f'OK: {checks} launches; both versions, independent oracle, invalid input, 1..1024 threads')
