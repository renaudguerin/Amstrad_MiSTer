#!/usr/bin/env python3
"""Run every plus_hw_probes screen on the production simulation model.

Builds the D5 production-T80 fixture (sim/plus prepare_d5_boot.py, GHDL T80 netlist)
with diagnostic probe ports, then runs one cartridge per start screen and writes
<out>/<NN>.ppm plus <NN>-events.txt. Diagnostic only: outputs are simulation
predictions to compare with original-hardware photographs, never expectations.
"""
import argparse
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PORTS = {
    'g_hcc': (8, 'mb.asic_vid.hcc'),
    'g_vc': (7, 'mb.plus_vc'),
    'g_rc': (5, 'mb.plus_rc'),
    'g_fire': (1, 'mb.asic_ga.raster_fire'),
    'g_int_n': (1, 'mb.INT_n'),
    'g_adj': (1, 'mb.asic_vid.in_adj'),
    'g_split': (1, 'mb.asic_vid.split_latch_event'),
    'g_store': (14, 'mb.asic_vid.vma_latch'),
    'g_ssa': (14, 'mb.asic_vid.SSA'),
    'g_splt': (8, 'mb.asic_vid.SPLT'),
    # PA7 follow-up: passive cause-chain taps (diagnostic fixture only).
    # g_c52/g_intcnt/g_classicn/g_progn read asic_ga_timing internals
    # (intcnt52 pulse, 6-bit scanline counter, classic/programmed latches).
    # g_intcycle reads the GHDL-T80 netlist wire mb.CPU.intcycle_n
    # (T80pa.vhd:96, already proven by d5_ack_sample in prepare_d5_boot.py);
    # g_insn reads the motherboard cpu_insn_start observation port.
    'g_intcnt': (6, 'mb.asic_ga.intcnt_reg'),
    'g_c52': (1, 'mb.asic_ga.intcnt52'),
    'g_classicn': (1, 'mb.asic_ga.classic_int_n'),
    'g_progn': (1, 'mb.asic_ga.programmed_int_n'),
    'g_intcycle': (1, 'mb.CPU.intcycle_n'),
    'g_insn': (1, 'mb.cpu_insn_start'),
}


def build_model(obj, cxx):
    subprocess.run(['make', '-C', str(ROOT / 'sim'), 't80-netlist'], check=True)
    subprocess.run(['python3', 'prepare_d5_boot.py'], cwd=ROOT / 'sim/plus', check=True)
    top = ROOT / 'sim/plus/obj_dir/d5_sources/p10_boot_test_top.v'
    s = top.read_text()
    anchor = 'output      [1:0] dbg_video_gamode,'
    assert s.count(anchor) == 1, 'fixture port list changed'
    s = s.replace(anchor, anchor + '\n' + '\n'.join(f'output [{w - 1}:0] {n},' for n, (w, _) in PORTS.items()))
    s = s.replace('endmodule', '\n'.join(f'assign {n} = {e};' for n, (_, e) in PORTS.items()) + '\nendmodule', 1)
    probe_top = obj / 'p10_boot_test_top.v'
    obj.mkdir(parents=True, exist_ok=True)
    probe_top.write_text(s)
    # Reuse the Makefile's D5 source list rather than duplicating it.
    rtl = subprocess.run(['make', '-s', '-f', '-', 'print-d5'], cwd=ROOT / 'sim/plus',
                         input='include Makefile\nprint-d5:\n\t@echo $(D5_RTL)\n',
                         check=True, capture_output=True, text=True).stdout.split()
    cmd = ['verilator', '--cc', '--exe', '--build', '-j', '8', '+1364-2001ext+.v', '+1800-2017ext+.sv',
           '-UVERILATOR', '-DP10_DMA_MOBO_REAL_IO', '--top-module', 'p10_boot_test_top', '-GSYNC_FILTER=0',
           '--Mdir', str(obj), '-Wno-fatal', '-Wno-PROCASSWIRE', '-CFLAGS', f'-std=c++17 -O2 -I{ROOT / "sim/plus"}',
           '-o', 'plus_hw_probes_sim', str(probe_top), *rtl, str(HERE / 'plus_hw_probes_sim.cpp')]
    env = dict(os.environ, MAKEFLAGS=f'CXX={cxx}') if cxx else None
    subprocess.run(cmd, check=True, env=env, stdout=subprocess.DEVNULL)
    return obj / 'plus_hw_probes_sim'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'output_files/plus-hw-probes/sim')
    parser.add_argument('--frames', type=int, default=30)
    parser.add_argument('--screens', type=int, nargs='*')
    parser.add_argument('--cxx', default='/opt/homebrew/opt/llvm/bin/clang++')
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    obj = ROOT / 'sim/plus/obj_dir/plus_hw_probes_sim'
    binary = obj / 'plus_hw_probes_sim' if args.no_build else build_model(obj, args.cxx)
    import plus_hw_probes
    screens = args.screens if args.screens else range(1, len(plus_hw_probes.TESTS))
    if any(n < 0 or n >= len(plus_hw_probes.TESTS) for n in screens):
        parser.error('screen number out of range')
    args.out.mkdir(parents=True, exist_ok=True)
    for n in screens:
        plus_hw_probes.build(args.out, n)

    def run(n):
        cpr = args.out / f'plus-hw-probes-start{n:02d}.cpr'
        r = subprocess.run([str(binary), str(cpr), str(args.out / f'{n:02d}'), str(args.frames)],
                           capture_output=True, text=True)
        return n, r.returncode, (r.stdout + r.stderr).strip()

    failed = []
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        for n, code, text in pool.map(run, screens):
            print(f'{n:02d}: exit {code} {text}')
            if code:
                failed.append(n)
    if failed:
        raise SystemExit(f'probe simulation failed for screens: {failed}')


if __name__ == '__main__':
    import sys
    sys.path.insert(0, str(HERE))
    main()
