-- Exercise the production Avalon scaler with legacy or master-clock pixels.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use std.env.all;

entity ascal_field_test is
  generic (trace_path : string := "trace.txt"; pixel_mode : boolean := false);
end entity;

architecture test of ascal_field_test is
  function setting(pixel_value, legacy_value : natural) return natural is
  begin
    if pixel_mode then return pixel_value; else return legacy_value; end if;
  end function;
  function flag(value : integer) return std_logic is
  begin
    if value=0 then return '0'; else return '1'; end if;
  end function;
  constant input_half_period : time := setting(15625, 40000) * 1 ps / 2;
  constant ram_base : unsigned(31 downto 0) := to_unsigned(setting(16#20000000#, 0), 32);
  signal iclk, oclk, aclk, rst : std_logic := '0';
  signal hs, vs, fl, de : std_logic := '0';
  signal ce : std_logic := '1';
  signal red : unsigned(7 downto 0) := x"80";
  signal green : unsigned(7 downto 0) := x"40";
  signal blue : unsigned(7 downto 0) := x"20";
  signal addr : std_logic_vector(27 downto 0);
  signal wd : std_logic_vector(127 downto 0);
  signal byteenable : std_logic_vector(15 downto 0);
  signal burstcount : std_logic_vector(7 downto 0);
  signal wr, rd : std_logic;
  signal avl_wait : std_logic := '0';
  signal prev_wr : std_logic := '0';
  signal trace_started : std_logic := '0';
  signal reader_run, reader_ce : std_logic := '0';
begin
  iclk <= not iclk after input_half_period;
  -- Settle output-domain configuration while the framebuffer reader is idle.
  pixel_output_clock : if pixel_mode generate
    oclk <= not oclk after 7 ns;
  end generate;
  aclk <= not aclk after 5 ns;
  dut : entity work.ascal
    generic map (RAMBASE => ram_base, RAMSIZE => x"00800000",
                 N_AW => 28, N_DW => 128, N_BURST => 256,
                 IHRES => setting(2048,1024), OHRES => setting(2048,1024),
                 BYTESWAP => pixel_mode,
                 MASK => x"01", DOWNSCALE => false, PALETTE => false,
                 PALETTE2 => false, ADAPTIVE => false)
    port map (i_r => red, i_g => green, i_b => blue,
      i_hs => hs, i_vs => vs, i_fl => fl, i_de => de, i_ce => ce, i_clk => iclk,
      format => "01",
      o_r => open, o_g => open, o_b => open, o_hs => open, o_vs => open,
      o_de => open, o_vbl => open, o_brd => open, o_ce => reader_ce, o_clk => oclk,
      pal1_dr => open, pal2_dr => open, o_lltune => open, i_hdmax => open, i_vdmax => open,
      run => reader_run, mode => "00000", htotal => setting(2303,127),
      hsstart => setting(2100,100), hsend => setting(2150,110),
      hdisp => setting(2048,80), hmin => 0, hmax => setting(2047,79),
      vtotal => setting(2303,31), vsstart => setting(2100,25),
      vsend => setting(2150,27), vdisp => setting(2048,20),
      vmin => 0, vmax => setting(2047,19),
      poly_clk => '0', poly_dw => (others => '0'), poly_a => (others => '0'), poly_wr => '0',
      avl_clk => aclk, avl_waitrequest => avl_wait, avl_readdata => (others => '0'),
      avl_readdatavalid => '0', avl_burstcount => burstcount, avl_writedata => wd,
      avl_address => addr, avl_write => wr, avl_read => rd, avl_byteenable => byteenable,
      reset_na => rst);

  process
    file trace : text open read_mode is trace_path;
    variable l, message_line : line;
    variable h,v,f,d,c,r,g,b : integer;
    variable first : boolean := true;
  begin
    wait for 500 ns;
    rst <= '1';
    while not endfile(trace) loop
      readline(trace, l);
      read(l,h); read(l,v); read(l,f); read(l,d);
      if pixel_mode then read(l,c); read(l,r); read(l,g); read(l,b); end if;
      wait until falling_edge(iclk);
      if pixel_mode and first then
        write(message_line, string'("INPUT_START_FS "));
        write(message_line, (now + input_half_period) / 1 fs);
        writeline(output, message_line);
      end if;
      hs <= flag(h); vs <= flag(v);
      fl <= flag(f); de <= flag(d);
      if pixel_mode then
        ce <= flag(c);
        red <= to_unsigned(r,8); green <= to_unsigned(g,8); blue <= to_unsigned(b,8);
        if first then
          wait until rising_edge(iclk);
          trace_started <= '1';
        end if;
      end if;
      first := false;
    end loop;
    if pixel_mode then
      wait until falling_edge(iclk);
      ce <= '0';
    end if;
    wait for 5 us;
    stop;
    wait;
  end process;

  process(aclk)
    variable l : line;
    variable beat : natural := 0;
    variable cycles : natural := 0;
  begin
    if rising_edge(aclk) then
      prev_wr <= wr;
      if pixel_mode then
        if trace_started='1' then
          assert reader_run='0' and reader_ce='0' and rd='0'
            report "inactive pixel reader issued an Avalon read" severity failure;
        end if;
        cycles := cycles + 1;
        if cycles mod 19 < 5 then avl_wait <= '1'; else avl_wait <= '0'; end if;
        if wr = '0' then
          assert beat=0 or beat=16 report "incomplete Avalon burst" severity failure;
          beat := 0;
        elsif wr = '1' and avl_wait = '0' then
          assert beat<16 report "oversized Avalon burst" severity failure;
          write(l, string'("BEAT ")); write(l, now / 1 ns);
          write(l, string'(" ")); write(l, to_integer(unsigned(addr)));
          write(l, string'(" ")); write(l, to_integer(unsigned(burstcount)));
          write(l, string'(" ")); write(l, beat);
          write(l, string'(" ")); write(l, to_hstring(byteenable));
          write(l, string'(" ")); write(l, wd);
          writeline(output, l);
          beat := beat + 1;
        end if;
      elsif wr = '1' and prev_wr = '0' and unsigned(addr) = 0 then
        write(l, string'("HEADER ")); write(l, to_hstring(wd)); write(l, string'(" "));
        write(l, now / 1 ns);
        write(l, string'(" ")); write(l, wd(80));
        write(l, string'( " " )); write(l, wd(81));
        write(l, string'(" ")); write(l, to_integer(unsigned(wd(63 downto 48))));
        writeline(output, l);
      end if;
    end if;
  end process;
end architecture;
