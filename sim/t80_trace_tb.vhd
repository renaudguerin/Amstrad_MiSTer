-- Trace testbench for native VHDL simulation of production T80pa.
-- Used to verify cycle-by-cycle equivalence against GHDL-synthesized Verilog.
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use std.textio.all;

entity t80_trace_tb is
end t80_trace_tb;

architecture sim of t80_trace_tb is
    signal clk     : std_logic := '0';
    signal reset_n : std_logic := '0';
    signal m1_n    : std_logic;
    signal mreq_n  : std_logic;
    signal iorq_n  : std_logic;
    signal rd_n    : std_logic;
    signal wr_n    : std_logic;
    signal a       : std_logic_vector(15 downto 0);
    signal di      : std_logic_vector(7 downto 0) := x"00";
    signal dout    : std_logic_vector(7 downto 0);
    signal halt_n  : std_logic;

    type rom_t is array (0 to 35) of std_logic_vector(7 downto 0);
    constant ROM : rom_t := (
        0 => x"00",                            -- NOP
        1 => x"01", 2 => x"05", 3 => x"BD",    -- LD BC, 0xBD05
        4 => x"ED", 5 => x"49",                -- OUT (C), C
        6 => x"01", 7 => x"43", 8 => x"BE",    -- LD BC, 0xBE43
        9 => x"21", 10 => x"20", 11 => x"00",  -- LD HL, 0x0020
        12 => x"ED", 13 => x"A3",              -- OUTI
        14 => x"76",                            -- HALT
        32 => x"42",                            -- data at 0x0020
        others => x"00"
    );

    function to_hstring(slv : std_logic_vector) return string is
        variable hex_val : string(1 to slv'length/4);
        variable nibble : std_logic_vector(3 downto 0);
        variable has_unknown : boolean;
    begin
        for i in hex_val'range loop
            nibble := slv(slv'high - (i-1)*4 downto slv'high - i*4 + 1);
            has_unknown := false;
            for j in 0 to 3 loop
                if nibble(j) /= '0' and nibble(j) /= '1' then
                    has_unknown := true;
                end if;
            end loop;
            if has_unknown then
                hex_val(i) := 'X';
            else
                case nibble is
                    when "0000" => hex_val(i) := '0';
                    when "0001" => hex_val(i) := '1';
                    when "0010" => hex_val(i) := '2';
                    when "0011" => hex_val(i) := '3';
                    when "0100" => hex_val(i) := '4';
                    when "0101" => hex_val(i) := '5';
                    when "0110" => hex_val(i) := '6';
                    when "0111" => hex_val(i) := '7';
                    when "1000" => hex_val(i) := '8';
                    when "1001" => hex_val(i) := '9';
                    when "1010" => hex_val(i) := 'A';
                    when "1011" => hex_val(i) := 'B';
                    when "1100" => hex_val(i) := 'C';
                    when "1101" => hex_val(i) := 'D';
                    when "1110" => hex_val(i) := 'E';
                    when "1111" => hex_val(i) := 'F';
                    when others => hex_val(i) := 'X';
                end case;
            end if;
        end loop;
        return hex_val;
    end function;

    function bit_char(s : std_logic) return character is
    begin
        if s = '0' then return '0';
        elsif s = '1' then return '1';
        else return 'X';
        end if;
    end function;

begin
    clk <= not clk after 5 ns;

    dut : entity work.T80pa
        port map (
            RESET_n => reset_n,
            CLK     => clk,
            CEN_p   => '1',
            CEN_n   => '1',
            WAIT_n  => '1',
            INT_n   => '1',
            NMI_n   => '1',
            BUSRQ_n => '1',
            M1_n    => m1_n,
            MREQ_n  => mreq_n,
            IORQ_n  => iorq_n,
            RD_n    => rd_n,
            WR_n    => wr_n,
            HALT_n  => halt_n,
            A       => a,
            DI      => di,
            DO      => dout
        );

    process(a, mreq_n, rd_n)
        variable addr_int : integer;
    begin
        addr_int := to_integer(unsigned(a));
        if mreq_n = '0' and rd_n = '0' and addr_int >= 0 and addr_int <= 35 then
            di <= ROM(addr_int);
        else
            di <= x"FF";
        end if;
    end process;

    process
        variable l : line;
        variable cycle : integer := 0;
    begin
        -- 4 cycles of reset active
        for c in 1 to 4 loop
            reset_n <= '0';
            wait until rising_edge(clk);
            wait for 1 ns;
            cycle := cycle + 1;
            write(l, string'("CYC=")); write(l, cycle);
            write(l, string'(" A=")); write(l, to_hstring(a));
            write(l, string'(" DO=")); write(l, to_hstring(dout));
            write(l, string'(" M1=")); write(l, bit_char(m1_n));
            write(l, string'(" MREQ=")); write(l, bit_char(mreq_n));
            write(l, string'(" IORQ=")); write(l, bit_char(iorq_n));
            write(l, string'(" RD=")); write(l, bit_char(rd_n));
            write(l, string'(" WR=")); write(l, bit_char(wr_n));
            write(l, string'(" HALT=")); write(l, bit_char(halt_n));
            writeline(output, l);
        end loop;

        -- Normal execution until HALT
        reset_n <= '1';
        while cycle < 150 loop
            wait until rising_edge(clk);
            wait for 1 ns;
            cycle := cycle + 1;
            write(l, string'("CYC=")); write(l, cycle);
            write(l, string'(" A=")); write(l, to_hstring(a));
            write(l, string'(" DO=")); write(l, to_hstring(dout));
            write(l, string'(" M1=")); write(l, bit_char(m1_n));
            write(l, string'(" MREQ=")); write(l, bit_char(mreq_n));
            write(l, string'(" IORQ=")); write(l, bit_char(iorq_n));
            write(l, string'(" RD=")); write(l, bit_char(rd_n));
            write(l, string'(" WR=")); write(l, bit_char(wr_n));
            write(l, string'(" HALT=")); write(l, bit_char(halt_n));
            writeline(output, l);

            if halt_n = '0' then
                -- Capture 2 additional cycles of HALT state
                for h in 1 to 2 loop
                    wait until rising_edge(clk);
                    wait for 1 ns;
                    cycle := cycle + 1;
                    write(l, string'("CYC=")); write(l, cycle);
                    write(l, string'(" A=")); write(l, to_hstring(a));
                    write(l, string'(" DO=")); write(l, to_hstring(dout));
                    write(l, string'(" M1=")); write(l, bit_char(m1_n));
                    write(l, string'(" MREQ=")); write(l, bit_char(mreq_n));
                    write(l, string'(" IORQ=")); write(l, bit_char(iorq_n));
                    write(l, string'(" RD=")); write(l, bit_char(rd_n));
                    write(l, string'(" WR=")); write(l, bit_char(wr_n));
                    write(l, string'(" HALT=")); write(l, bit_char(halt_n));
                    writeline(output, l);
                end loop;
                exit;
            end if;
        end loop;
        std.env.finish;
    end process;
end sim;
