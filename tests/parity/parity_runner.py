#!/usr/bin/env python3
"""L1 对拍 harness runner（design_docs/04 册 §2 架构落地）。

清单驱动：读 parity_manifest.json → 双方运行（产品 CLI / 参考实现外部进程）→
列对齐 → 双阈值比较（P-010：绝对 1e-6 或相对 1e-9 任一满足即通过）→ 差异报告。

隔离约定：harness 仅经 CLI 可执行 + 文件与参考实现交互，不链接产品源码
（NFR-SEC-001 许可边界）。R/Python 参考工具不入 CI，本地全量运行（M2 门禁）。

用法:
    python3 tests/parity/parity_runner.py            # 全部用例
    python3 tests/parity/parity_runner.py --filter ccr_io
    python3 tests/parity/parity_runner.py --binary build/src/opensuperdea
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

PARITY_DIR = Path(__file__).resolve().parent
REPO_ROOT = PARITY_DIR.parent.parent


def run_product(binary: Path, case: dict, workdir: Path) -> dict:
    """运行产品 CLI（--format json），返回解析后的结果 JSON。"""
    product = case["product"]
    data_path = (PARITY_DIR / case["data"]).resolve()
    command = [
        str(binary),
        "run",
        "--model", product["model"],
        "--orientation", product["orientation"],
        "--data", str(data_path),
        "--format", "json",
        "-o", str(workdir),
    ]
    if product.get("rts"):
        command.extend(["--rts", product["rts"]])
    if product.get("super"):
        command.append("--super")
    if product.get("direction"):
        command.extend(["--direction", product["direction"]])
    proc = subprocess.run(command, capture_output=True, text=True, timeout=300)
    if proc.returncode != 0:
        raise RuntimeError(f"产品 CLI 退出码 {proc.returncode}: {proc.stderr.strip()}")
    result_files = list(workdir.glob("*_result.json"))
    if len(result_files) != 1:
        raise RuntimeError(f"期望 1 个结果 JSON，实得 {len(result_files)}")
    return json.loads(result_files[0].read_text(encoding="utf-8"))


def run_reference(case: dict, workdir: Path) -> dict[str, dict[str, float]]:
    """运行参考实现脚本（输出 TSV），返回 {dmu: {列: 值}}。"""
    reference = case["reference"]
    data_path = (PARITY_DIR / case["data"]).resolve()
    script = (PARITY_DIR / reference["script"]).resolve()
    out_path = workdir / "reference.tsv"
    with out_path.open("w", encoding="utf-8") as out_file:
        proc = subprocess.run(
            ["Rscript", str(script), str(data_path), *reference["args"]],
            stdout=out_file,
            stderr=subprocess.PIPE,
            text=True,
            timeout=300,
        )
    if proc.returncode != 0:
        raise RuntimeError(f"参考实现退出码 {proc.returncode}: {proc.stderr.strip()}")
    rows: dict[str, dict[str, float]] = {}
    with out_path.open(encoding="utf-8") as tsv_file:
        header = tsv_file.readline().rstrip("\n").split("\t")
        for line in tsv_file:
            fields = line.rstrip("\n").split("\t")
            if len(fields) != len(header):
                raise RuntimeError(f"参考 TSV 列数不一致: {line!r}")
            row = dict(zip(header, fields))
            rows[row["dmu"]] = {
                key: float(value)
                for key, value in row.items()
                if key != "dmu" and value != "" and value.lower() != "na"
            }
    return rows


def passes_dual_threshold(left: float, right: float, abs_tol: float, rel_tol: float) -> bool:
    """双阈值判定（P-010）：绝对 1e-6 或相对 1e-9 任一满足即通过。"""
    diff = abs(left - right)
    if diff <= abs_tol:
        return True
    scale = max(abs(left), abs(right))
    if scale == 0.0:
        return diff == 0.0
    return diff / scale <= rel_tol


def compare_case(case: dict, product_json: dict, reference_rows: dict,
                 tolerance: dict) -> tuple[bool, list[str]]:
    """列对齐 + 双阈值比较，返回（是否通过, 差异行列表）。

    compare 条目两种形态：
      "score": "efficiency"                       —— 直接同列比对
      "score": {"column": "efficiency",
                "transform": "reciprocal",         —— 参考值取倒数后比对
                "reason": "…"}                     —— 差异口径须显式标注理由（04 册 §2）
    """
    compare: dict = case["compare"]
    failures: list[str] = []
    product_scores = {row["dmu"]: row for row in product_json["results"]}
    ok = True
    for dmu, ref_values in reference_rows.items():
        if dmu not in product_scores:
            failures.append(f"{dmu}: 产品结果缺少该 DMU")
            ok = False
            continue
        for product_column, mapping in compare.items():
            if isinstance(mapping, str):
                reference_column, transform = mapping, None
            else:
                reference_column = mapping["column"]
                transform = mapping.get("transform")
            ref_value = ref_values.get(reference_column)
            if transform == "reciprocal" and ref_value is not None and ref_value != 0.0:
                ref_value = 1.0 / ref_value
            product_value = product_scores[dmu].get(product_column)
            if ref_value is None or product_value is None:
                # 不可行/缺值：双方一致为通过，单边有值为失败
                if ref_value is None and product_value is None:
                    continue
                failures.append(
                    f"{dmu}.{product_column}: 产品={product_value} 参考={ref_value}（单边缺值）")
                ok = False
                continue
            if not passes_dual_threshold(product_value, ref_value, tolerance["abs"],
                                         tolerance["rel"]):
                failures.append(
                    f"{dmu}.{product_column}: 产品={product_value} 参考={ref_value} "
                    f"偏差={abs(product_value - ref_value):.3e}")
                ok = False
    return ok, failures


def main() -> int:
    parser = argparse.ArgumentParser(description="OpenSuperDEA L1 对拍 runner")
    parser.add_argument("--manifest", default=str(PARITY_DIR / "parity_manifest.json"))
    parser.add_argument("--binary",
                        default=str(REPO_ROOT / "build/src/opensuperdea"),
                        help="产品 CLI 路径（默认仓库构建产物）")
    parser.add_argument("--filter", default="", help="用例名子串过滤")
    arguments = parser.parse_args()

    manifest = json.loads(Path(arguments.manifest).read_text(encoding="utf-8"))
    binary = Path(arguments.binary)
    if not binary.exists():
        print(f"产品 CLI 不存在: {binary}（先 cmake --build build）", file=sys.stderr)
        return 2

    passed = 0
    failed = 0
    for case in manifest["cases"]:
        name = case["name"]
        if arguments.filter and arguments.filter not in name:
            continue
        try:
            with tempfile.TemporaryDirectory(prefix=f"parity_{name}_") as workdir_str:
                workdir = Path(workdir_str)
                product_json = run_product(binary, case, workdir)
                reference_rows = run_reference(case, workdir)
            ok, failures = compare_case(case, product_json, reference_rows,
                                        manifest["tolerance"])
        except (RuntimeError, json.JSONDecodeError, KeyError) as error:
            ok, failures = False, [f"执行异常: {error}"]
        if ok:
            passed += 1
            print(f"PASS {name}")
        else:
            failed += 1
            print(f"FAIL {name}")
            for failure in failures:
                print(f"     {failure}")

    total = passed + failed
    print(f"\nparity: {passed}/{total} passed"
          + (f"（过滤: {arguments.filter}）" if arguments.filter else ""))
    return 0 if failed == 0 and total > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
