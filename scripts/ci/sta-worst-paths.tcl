# TimeQuest report hook: retain endpoints and path delays, not just clock slack.
# Runs after default analysis; leave its summary and the existing timing gate intact.
# Quartus 17 command reference:
# https://resources.altera.com/quartushelp/17.0/tafs/tafs/tcl_pkg_sta_ver_1.0_cmd_report_timing.htm
# Both CI routes already collect output_files/*.rpt, including on timing failure.
report_timing -setup -npaths 20 -detail full_path \
    -panel_name {Worst setup paths} -file output_files/Amstrad.sta.paths.rpt
report_timing -hold -npaths 10 -detail full_path -append \
    -panel_name {Worst hold paths} -file output_files/Amstrad.sta.paths.rpt
