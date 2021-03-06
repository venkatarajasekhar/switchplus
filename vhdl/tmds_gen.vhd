library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.defs.all;

library unisim;
use unisim.vcomponents.all;

entity tmds_gen is
  port (hdmi_Rp, hdmi_Rn, hdmi_Gp, hdmi_Gn,
        hdmi_Bp, hdmi_Bn, hdmi_Cp, hdmi_Cn : out std_logic;
        led : out byte_t;
        clkin100 : in std_logic);
end tmds_gen;
architecture tmds_gen of tmds_gen is
  signal xx : integer range 0 to 1649;
  signal yy : integer range 0 to 749;
  signal xxlast : boolean;
  signal yylast : boolean;
  signal clk_raw, clk_fb, clk_bit, clk_nibble_raw, locked : std_logic;
  signal clk, clk_nibble : std_logic;
  signal R, G, B : byte_t;
  signal hsync, vsync, de : std_logic;
begin
  led(7 downto 1) <= "1111111";
  led(0) <= locked;

  -- 1280x720p @59.94/60 Hz
  -- 110 + 40 + 220 + 1280 = 1650 clocks/line
  -- 5 + 5 + 20 + 720 = 750 lines/frame
  -- 1650 * 750 = 123750 clocks/frame
  -- 123750 * 60 = 74250000 MHz.
  -- Let's call it 75MHz.
  process
  begin
    wait until rising_edge(clk);
    xxlast <= (xx = 1648);
    yylast <= (yy = 748);

    if xxlast then
      xx <= 0;
    else
      xx <= xx + 1;
    end if;

    if xxlast then
      if yylast then
        yy <= 0;
      else
        yy <= yy + 1;
      end if;
    end if;

    hsync <= bb(xx >= 110 and xx < 150);
    vsync <= bb(yy >= 5 and yy < 10);
    de <= bb(xx >= 370 and yy >= 30);

    R <= to_unsigned (xx, 8);
    G <= to_unsigned (yy, 8);
    B <= not to_unsigned (yy, 8);
  end process;

  lcd_pll : pll_base
    generic map(
      CLK_FEEDBACK   => "CLKFBOUT",
      DIVCLK_DIVIDE  => 2, CLKFBOUT_MULT => 15,
      CLKOUT0_DIVIDE => 1,
      CLKOUT1_DIVIDE => 4,
      CLKOUT2_DIVIDE => 10,
      CLKIN_PERIOD   => 10.0)
    port map(
      CLKFBIN => clk_fb, CLKFBOUT => clk_fb,
      CLKOUT0 => clk_bit, CLKOUT1 => clk_nibble_raw, CLKOUT2 => clk_raw,
      RST => '0', LOCKED => locked, CLKIN => clkin100);
  clk_bufg : bufg port map (I => clk_raw, O => clk);
  clk_nibble_bufg : bufg port map (I => clk_nibble_raw, O => clk_nibble);

  encode : entity work.tmds_encode port map (
    R, G, B,
    hsync, vsync, '0', '0', '0', '0', de,
    hdmi_Rp, hdmi_Rn, hdmi_Gp, hdmi_Gn, hdmi_Bp, hdmi_Bn, hdmi_Cp, hdmi_Cn,
    clk, clk_nibble, clk_bit, locked);
end tmds_gen;
