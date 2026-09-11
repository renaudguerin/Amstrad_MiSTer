"""Adapt the P10 fixture to the generated production T80 and top-level /EXP.
Generated files stay in obj_dir; production RTL and CPRs are never modified.
"""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
out = root / 'sim/plus/obj_dir/d5_sources'
out.mkdir(parents=True, exist_ok=True)
s = (root / 'sim/plus/p10_boot_test_top.v').read_text()
s = s.replace('input             clk,', 'input [10:0] d5_key,\n\toutput [7:0] d5_romsel,\n\toutput [7:0] d5_a,\n\tinput             clk,', 1)
s = s.replace(".ps2_key(11'd0)", '.ps2_key(d5_key)').replace(".no_wait(1'b1)", ".no_wait(1'b0)")
s = s.replace('mb.CPU.cen_n', 'mb.CPU.CEN_n').replace('mb.CPU.cen_p;', 'mb.CPU.CEN_p;').replace('mb.CPU.wait_n', 'mb.CPU.WAIT_n')
s = s.replace('mb.CPU.u0.tstate', 'mb.CPU.tstate').replace('mb.CPU.u0.mc_max', 'mb.CPU.u0.mcycles')
s = s.replace('endmodule', 'assign d5_romsel = mmu.romsel;\nassign d5_a = mb.CPU.REG[7:0];\nendmodule', 1)
# Compile the actual production assignment, so reverting Amstrad.sv is tested.
config = re.search(r"^wire plus_exp_n = (.*);$", (root / 'Amstrad.sv').read_text(), re.M)
if not config:
    raise SystemExit('Cannot locate production plus_exp_n assignment')
if s.count("wire plus_exp_n  = 1'b1;") != 1:
    raise SystemExit('P10 /EXP fixture assignment changed; update adapter')
s = s.replace("wire plus_exp_n  = 1'b1;", 'wire plus_exp_n = ' + config[1].replace('plus_model', 'plus_model_i') + ';')
(out / 'p10_boot_test_top.v').write_text(s)
s = (root / 'rtl/Amstrad_motherboard.v').read_text()
a = s.index('T80pa CPU'); b = s.index('\n);', a); part = s[a:b]
ports = re.findall(r'\binput\s+(?:\[[^]]+\]\s*)?(\w+)', (root/'sim/obj_dir/t80/T80pa.v').read_text().split('module T80pa')[1].split(');')[0])
ports += ['A','DO','RD_n','WR_n','IORQ_n','MREQ_n','M1_n','RFSH_n']
for p in ports:
    part = re.sub(r'\.'+re.escape(p.lower())+r'\(', '.'+p+'(', part)
part += ",\n.OUT0(1'b0), .R800_mode(1'b0)"
(out / 'Amstrad_motherboard.v').write_text(s[:a]+part+s[b:])
