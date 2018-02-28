#include <LiquidCrystal_I2C.h>                                        // https://github.com/openenergymonitor/LiquidCrystal_I2C

static int i2c_lcd_address[2] = {0x27, 0x3f};                                  // I2C addresses to test for I2C LCD device
static int current_lcd_i2c_addr = 0;                                                  // Used to store current I2C address as found by i2_lcd_detect()

// emonPi used 16 x 2 I2C LCD display
static int i2c_lcd_detect()
{
        Wire.begin();
        for (int i = 0; i < 2; i++) {
                Wire.beginTransmission(i2c_lcd_address[i]);
                if (Wire.endTransmission() == 0) {
                        Serial.print("LCD found i2c 0x"); Serial.println(i2c_lcd_address[i], HEX);
                        return (i2c_lcd_address[i]);
                }
        }
        Serial.println("LCD not found");
        return(0);
}


static void emonPi_LCD_Startup()
{
        current_lcd_i2c_addr = i2c_lcd_detect();

        if (!current_lcd_i2c_addr)
                return;

        LiquidCrystal_I2C lcd(current_lcd_i2c_addr, 16, 2); // LCD I2C address to 0x27, 16x2 line display
        lcd.init();                // initialize the lcd
        lcd.backlight();           // Or lcd.noBacklight()
        lcd.print(F("emonPi V")); lcd.print(firmware_version*0.01);
        lcd.setCursor(0, 1); lcd.print(F("OpenEnergyMon"));
}

static void lcd_print_startup(){
        if (!current_lcd_i2c_addr)
                return;

        //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        LiquidCrystal_I2C lcd(current_lcd_i2c_addr, 16, 2); // LCD I2C address to 0x27, 16x2 line display
        lcd.clear();
        lcd.backlight();

        lcd.print(F("AC "));
        if (not ACAC)
                lcd.print(F("NOT "));

        lcd.print(F("Detected"));

        lcd.setCursor(0, 1);
        lcd.print(F("Detected "));
        lcd.print(CT_count);
        lcd.print(F(" CT's"));

        delay(2000);

        lcd.clear();
        lcd.print(F("Detected: ")); lcd.print(numSensors);
        lcd.setCursor(0, 1);
        lcd.print(F("DS18B20 Temp"));

        delay(2000);

        lcd.clear();
        lcd.print(F("Raspberry Pi"));
        lcd.setCursor(0, 1);
        lcd.print(F("Booting..."));

        delay(20);
}
