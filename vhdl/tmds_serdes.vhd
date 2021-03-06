library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.defs.all;

library unisim;
use unisim.vcomponents.all;

entity tmds_serdes is
  port (lcd_R, lcd_G, lcd_B : in byte_t;
        lcd_hsync, lcd_vsync, lcd_de : in std_logic;
        lcd_clk : in std_logic;

        hdmi_Rp, hdmi_Rn, hdmi_Gp, hdmi_Gn,
        hdmi_Bp, hdmi_Bn, hdmi_Cp, hdmi_Cn : out std_logic;

        led : out byte_t;

        spartan_m0, spartan_m1 : in std_logic;

        lcd_le : in std_logic; -- Actually spirom CS.
        ssp1_ssel, ssp1_sck, ssp1_mosi : in std_logic;
        ssp1_miso : out std_logic;

        flash_cs_inv, flash_sclk, flash_si : out std_logic;
        flash_so : in std_logic);
end tmds_serdes;
architecture tmds_serdes of tmds_serdes is
  signal R, G, B : byte_t;
  signal hsync, vsync, de : std_logic;
  signal txt_R, txt_G, txt_B : byte_t;
  signal txt_hsync, txt_vsync, txt_de : std_logic;
  signal clk_raw, clk_bit, clk_nibble_raw, clk_console_raw, locked : std_logic;
  signal clk, clk_nibble, clk_console : std_logic;

  attribute iob : string;
  attribute iob of R, G, B, hsync, vsync, de : signal is "TRUE";
  attribute pullup : string;
  attribute pullup of spartan_m0, spartan_m1 : signal is "TRUE";
  --attribute buffer_type : string;
  --attribute buffer_type of clk_console : signal is "bufg";
  --attribute buffer_type of clk : signal is "bufg";

  alias spirom_cs is lcd_le;

begin
  led(0) <= locked;

  w_de : entity work.watchdog generic map (16) port map (de, led(1), clk);
  w_vs : entity work.watchdog generic map (21) port map (vsync, led(2), clk);
  w_hs : entity work.watchdog generic map (11) port map (hsync, led(3), clk);

  led(4) <= spirom_cs;
  led(5) <= '1';
  led(6) <= spartan_m0;
  led(7) <= not spartan_m1;

  flash_cs_inv <= spirom_cs;
  flash_sclk <= ssp1_sck when spirom_cs = '0' else 'Z';
  flash_si <= ssp1_mosi when spirom_cs = '0' else 'Z';
  ssp1_miso <= flash_so;

  process
  begin
    wait until rising_edge(clk);

    R <= lcd_R(7 downto 3) & "000";
    G <= lcd_G(7 downto 2) & "00";
    B <= lcd_B(7 downto 3) & "000";
    hsync <= lcd_hsync;
    vsync <= lcd_vsync;
    de <= lcd_de;
  end process;

  -- 1280x720p @59.94/60 Hz
  -- 110 + 40 + 220 + 1280 = 1650 clocks/line
  -- 5 + 5 + 20 + 720 = 750 lines/frame
  -- 1650 * 750 = 123750 clocks/frame
  -- 123750 * 60 = 74250000 MHz.
  -- Let's call it 75MHz.

  -- 1024x1024R @60Hz.  74.75MHz.
  -- Horizontal: 1024 1072 1104 1184
  -- Vertical: 1024 1027 1037 1054
  lcd_pll : pll_base
    generic map(
      CLK_FEEDBACK   => "CLKOUT0",
      DIVCLK_DIVIDE  => 1,
      CLKOUT0_DIVIDE => 10,
      CLKOUT1_DIVIDE => 1,
      CLKOUT2_DIVIDE => 4,
      CLKOUT3_DIVIDE => 5,
      CLKIN_PERIOD   => 13.3779264214047)
    port map(
      CLKOUT0 => clk_raw, CLKOUT1 => clk_bit, CLKOUT2 => clk_nibble_raw,
      CLKOUT3 => clk_console_raw,
      CLKIN => lcd_clk, CLKFBIN => clk, RST => '0', LOCKED => locked);
  clk_bufg : bufg port map (I => clk_raw, O => clk);
  clk_nibble_bufg : bufg port map (I => clk_nibble_raw, O => clk_nibble);
  clk_console_bufg : bufg port map (I => clk_console_raw, O => clk_console);

  c : entity work.console port map (
    R, G, B, hsync, vsync, de,
    txt_R, txt_G, txt_B, txt_hsync, txt_vsync, txt_de, clk,
    ssp1_ssel, ssp1_mosi, ssp1_sck,
    open, -- fixme, use a gpio.
    clk_console);

  encode : entity work.tmds_encode port map (
    txt_R, txt_G, txt_B, txt_hsync, txt_vsync, '0', '0', '0', '0', txt_de,
    hdmi_Rp, hdmi_Rn, hdmi_Gp, hdmi_Gn, hdmi_Bp, hdmi_Bn, hdmi_Cp, hdmi_Cn,
    clk, clk_nibble, clk_bit, locked);
end tmds_serdes;
