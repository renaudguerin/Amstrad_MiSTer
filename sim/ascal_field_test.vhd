-- Exercise the production Avalon scaler with legacy or master-clock pixels.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use std.env.all;

entity ascal_field_test is
  generic (trace_path : string := "trace.txt"; pixel_mode : boolean := false;
           reader_mode : boolean := false; reader_scale : positive := 1;
           reader_width : positive := 96; reader_height : positive := 16;
           reader_hblank : positive := 64; reader_vblank : positive := 16);
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
  function output_setting(reader_value, pixel_value, legacy_value : natural) return natural is
  begin
    if reader_mode then return reader_value; end if;
    return setting(pixel_value, legacy_value);
  end function;
  signal out_red, out_green, out_blue : unsigned(7 downto 0);
  signal out_hs, out_vs, out_de, out_border : std_logic;
  signal ram_data : std_logic_vector(127 downto 0) := (others => '0');
  signal ram_valid, freeze_input, frozen : std_logic := '0';
  signal reader_run, reader_ce : std_logic := '0';
begin
  iclk <= not iclk after input_half_period;
  -- Pixel replays clock output configuration; only the optional reader advances it.
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
      o_r => out_red, o_g => out_green, o_b => out_blue, o_hs => out_hs, o_vs => out_vs,
      o_de => out_de, o_vbl => open, o_brd => out_border, o_ce => reader_ce, o_clk => oclk,
      pal1_dr => open, pal2_dr => open, o_lltune => open, i_hdmax => open, i_vdmax => open,
      run => reader_run, freeze => freeze_input, mode => "00000",
      htotal => output_setting(reader_width*reader_scale+reader_hblank,2303,127),
      hsstart => output_setting(reader_width*reader_scale+16,2100,100),
      hsend => output_setting(reader_width*reader_scale+32,2150,110),
      hdisp => output_setting(reader_width*reader_scale,2048,80), hmin => 0,
      hmax => output_setting(reader_width*reader_scale-1,2047,79),
      vtotal => output_setting(reader_height*reader_scale+reader_vblank,2303,31),
      vsstart => output_setting(0,2100,25),
      vsend => output_setting(2,2150,27),
      vdisp => output_setting(reader_height*reader_scale+8,2048,20),
      vmin => output_setting(8,0,0), vmax => output_setting(reader_height*reader_scale+7,2047,19),
      poly_clk => '0', poly_dw => (others => '0'), poly_a => (others => '0'), poly_wr => '0',
      avl_clk => aclk, avl_waitrequest => avl_wait, avl_readdata => ram_data,
      avl_readdatavalid => ram_valid, avl_burstcount => burstcount, avl_writedata => wd,
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
    if reader_mode then
      -- Input is a stable periodic image. Drain accepted writer traffic before
      -- freezing it, then assert in the RAM responder that reads never overlap writes.
      freeze_input <= '1';
      wait for 5 us;
      frozen <= '1';
      reader_ce <= '1';
      reader_run <= '1';
      write(message_line, string'("READER_START_NS "));
      write(message_line, now / 1 ns);
      writeline(output, message_line);
      wait for 5 * (reader_width*reader_scale+reader_hblank) * (reader_height*reader_scale+reader_vblank) * 14 ns;
    end if;
    stop;
    wait;
  end process;

  process(aclk)
    variable l : line;
    variable beat : natural := 0;
    variable cycles : natural := 0;
    type memory_type is array (0 to output_setting(2097152,65536,65536)-1) of std_logic_vector(7 downto 0);
    -- Populate only from known-address accepted Avalon writes, never the oracle.
    variable memory : memory_type := (others => (others => 'U'));
    type queue_type is array (0 to 7) of natural;
    variable read_address, read_count : queue_type := (others => 0);
    variable head, tail, queued, response_beat : natural := 0;
    variable byte_address : integer;
    variable response : std_logic_vector(127 downto 0);
    variable read_stalled : boolean := false;
    variable held_address : std_logic_vector(27 downto 0);
    variable held_count : std_logic_vector(7 downto 0);
  begin
    if rising_edge(aclk) then
      prev_wr <= wr;
      if pixel_mode then
        if trace_started='1' and not reader_mode then
          assert reader_run='0' and reader_ce='0' and rd='0'
            report "inactive pixel reader issued an Avalon read" severity failure;
        end if;
        if reader_mode then
          ram_valid <= '0';
          if read_stalled then
            assert rd='1' and addr=held_address and burstcount=held_count
              report "Avalon read command changed under waitrequest" severity failure;
          end if;
          read_stalled := rd='1' and avl_wait='1';
          if read_stalled then
            held_address := addr; held_count := burstcount;
            write(l, string'("READ_STALL ")); write(l, now / 1 ns);
            writeline(output,l);
          end if;
          if frozen='1' then
            assert wr='0' report "write after frozen reader acquisition" severity failure;
          end if;
          if rd='1' and avl_wait='0' then
            assert wr='0' and queued<8 report "invalid read command/queue" severity failure;
            read_address(tail) := to_integer(unsigned(addr));
            read_count(tail) := to_integer(unsigned(burstcount));
            tail := (tail+1) mod 8; queued := queued+1;
            write(l, string'("READ ")); write(l, now / 1 ns);
            write(l, string'(" ")); write(l, to_integer(unsigned(addr)));
            write(l, string'(" ")); write(l, to_integer(unsigned(burstcount)));
            writeline(output,l);
          end if;
          if queued>0 and cycles mod 7/=2 and cycles mod 7/=3 then
            byte_address := 16*(read_address(head)+response_beat)-to_integer(ram_base);
            assert byte_address>=0 and byte_address+15<memory'length
              report "read outside captured RAM" severity failure;
            for lane in 0 to 15 loop
              response(8*lane+7 downto 8*lane) := memory(byte_address+lane);
            end loop;
            ram_data <= response; ram_valid <= '1';
            write(l, string'("RETURN ")); write(l, now / 1 ns);
            write(l, string'(" ")); write(l, read_address(head)+response_beat);
            writeline(output,l);
            response_beat := response_beat+1;
            if response_beat=read_count(head) then
              response_beat:=0; head:=(head+1) mod 8; queued:=queued-1;
            end if;
          end if;
        end if;
        cycles := cycles + 1;
        if cycles mod 19 < 5 then avl_wait <= '1'; else avl_wait <= '0'; end if;
        if wr = '0' then
          assert beat=0 or beat=16 report "incomplete Avalon burst" severity failure;
          beat := 0;
        elsif wr = '1' and avl_wait = '0' then
          if reader_mode and not is_x(addr) then
            byte_address := 16*(to_integer(unsigned(addr))+beat)-to_integer(ram_base);
            assert byte_address>=0 and byte_address+15<memory'length
              report "write outside captured RAM addr=" & integer'image(to_integer(unsigned(addr))) & " byte=" & integer'image(byte_address) severity failure;
            for lane in 0 to 15 loop
              if byteenable(lane)='1' then
                memory(byte_address+lane) := wd(8*lane+7 downto 8*lane);
              end if;
            end loop;
          end if;
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
  reader_output : if reader_mode generate
    process(oclk)
      variable l : line;
    begin
      if falling_edge(oclk) and reader_run='1' then
        write(l, string'("OUT ")); write(l, now / 1 ns);
        write(l, string'(" ")); write(l, out_hs);
        write(l, string'(" ")); write(l, out_vs);
        write(l, string'(" ")); write(l, out_de);
        write(l, string'(" ")); write(l, out_border);
        write(l, string'(" ")); write(l, to_hstring(out_red & out_green & out_blue));
        writeline(output,l);
      end if;
    end process;
  end generate;
end architecture;
