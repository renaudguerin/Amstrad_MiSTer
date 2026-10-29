-- Exercise the real Avalon scaler using captured, CE-qualified video samples.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use std.env.all;

entity ascal_field_test is
  generic (trace_path : string := "trace.txt");
end entity;

architecture test of ascal_field_test is
  signal iclk : std_logic := '0';
  signal oclk : std_logic := '0';
  signal aclk : std_logic := '0';
  signal rst : std_logic := '0';
  signal hs, vs, fl, de : std_logic := '0';
  signal addr : std_logic_vector(27 downto 0);
  signal wd : std_logic_vector(127 downto 0);
  signal wr, rd : std_logic;
  signal prev_wr : std_logic := '0';
begin
  iclk <= not iclk after 20 ns;
  -- Output remains stopped; this test observes the input framebuffer writer.
  aclk <= not aclk after 5 ns;
  dut : entity work.ascal
    generic map (RAMBASE => x"00000000", RAMSIZE => x"00800000",
                 N_AW => 28, N_DW => 128, N_BURST => 256,
                 IHRES => 1024, OHRES => 1024, BYTESWAP => false,
                 MASK => x"01", DOWNSCALE => false, PALETTE => false,
                 PALETTE2 => false, ADAPTIVE => false)
    port map (i_r => x"80", i_g => x"40", i_b => x"20",
      i_hs => hs, i_vs => vs, i_fl => fl, i_de => de, i_ce => '1', i_clk => iclk,
      o_r => open, o_g => open, o_b => open, o_hs => open, o_vs => open,
      o_de => open, o_vbl => open, o_brd => open, o_ce => '0', o_clk => oclk,
      pal1_dr => open, pal2_dr => open, o_lltune => open, i_hdmax => open, i_vdmax => open,
      run => '0', mode => "00000", htotal => 127, hsstart => 100, hsend => 110,
      hdisp => 80, hmin => 0, hmax => 79, vtotal => 31, vsstart => 25,
      vsend => 27, vdisp => 20, vmin => 0, vmax => 19,
      poly_clk => '0', poly_dw => (others => '0'), poly_a => (others => '0'), poly_wr => '0',
      avl_clk => aclk, avl_waitrequest => '0', avl_readdata => (others => '0'),
      avl_readdatavalid => '0', avl_burstcount => open, avl_writedata => wd,
      avl_address => addr, avl_write => wr, avl_read => rd, avl_byteenable => open,
      reset_na => rst);

  process
    file trace : text open read_mode is trace_path;
    variable l : line;
    variable h,v,f,d : integer;
  begin
    wait for 500 ns;
    rst <= '1';
    while not endfile(trace) loop
      readline(trace, l);
      read(l,h); read(l,v); read(l,f); read(l,d);
      wait until falling_edge(iclk);
      if h=0 then hs <= '0'; else hs <= '1'; end if;
      if v=0 then vs <= '0'; else vs <= '1'; end if;
      if f=0 then fl <= '0'; else fl <= '1'; end if;
      if d=0 then de <= '0'; else de <= '1'; end if;
    end loop;
    wait for 5 us;
    stop;
    wait;
  end process;

  process(aclk)
    variable l : line;
  begin
    if rising_edge(aclk) then
      prev_wr <= wr;
      if wr = '1' and prev_wr = '0' and unsigned(addr) = 0 then
        write(l, string'("HEADER ")); write(l, to_hstring(wd)); write(l, string'(" "));
        write(l, now / 1 ns);
        write(l, string'(" "));
        write(l, wd(80));
        write(l, string'( " " ));
        write(l, wd(81));
        write(l, string'(" "));
        write(l, to_integer(unsigned(wd(63 downto 48))));
        writeline(output, l);
      end if;
    end if;
  end process;
end architecture;
