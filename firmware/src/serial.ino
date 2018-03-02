
void serial_print_startup()
{
        if (ACAC) {
                Serial.println(F("AC Wave Detected - Real Power calc enabled"));
                Serial.print(F("Vcal: ")); Serial.println(config.Vcal);
                Serial.print(F("Vrms: ")); Serial.print(config.Vrms); Serial.println(F("V"));
        } else {
                Serial.println(F("AC NOT detected - Apparent Power calc enabled"));
                Serial.print(F("Assuming VRMS: "));
                Serial.print(config.Vrms); Serial.println(F("V"));
        }

        Serial.print("Detected "); Serial.print(CT_count); Serial.println(" CT's");

        Serial.print(F("Detected ")); Serial.print(numSensors); Serial.println(F(" DS18B20"));

        if (RF_STATUS == 1) {
                #if (RF69_COMPAT)
                Serial.println(F("RFM69CW Init: "));
                #else
                Serial.println(F("RFM12B Init: "));
                #endif

                Serial.print(F("Node ")); Serial.print(nodeID);
                Serial.print(F(" Freq "));
                if (RF_freq == RF12_433MHZ) Serial.print(F("433Mhz"));
                if (RF_freq == RF12_868MHZ) Serial.print(F("868Mhz"));
                if (RF_freq == RF12_915MHZ) Serial.print(F("915Mhz"));
                Serial.print(F(" Network ")); Serial.println(networkGroup);

                showString(helpText1);
        }
}

void serial_print_config(struct Config *c)
{
        Serial.print(F("Configuration\n Vcal="));
        Serial.println(c->Vcal);
        Serial.print(F(" Vrms="));
        Serial.println(c->Vrms);
        Serial.print(F(" Ical1="));
        Serial.println(c->Ical1);
        Serial.print(F(" Ical2="));
        Serial.println(c->Ical2);
        Serial.print(F(" Phase="));
        Serial.println(c->phase_shift);
}
void serial_print_emonpi()
{
        Serial.print(F("P1:"));
        Serial.print(emonPi.power1);
        Serial.print(F(" P2:"));
        Serial.print(emonPi.power2);
        Serial.print(F(" Vrms:"));
        Serial.println(emonPi.Vrms);
        Serial.print("Pulsecount:");
        Serial.println(emonPi.pulseCount);
        for (int t = 0; t < numSensors; t++) {
                Serial.print(F(" T "));
                Serial.print(emonPi.temp[t]);
        }
        if (numSensors)
                Serial.println();
}

//Send emonPi data to Pi serial /dev/ttyAMA0 using struct JeeLabs RF12 packet structure
void serial_print_bytes(byte len, void* data)
{
        byte *bytes = (byte *)data;
        for (byte i = 0; i < len; i++) {
                Serial.print(F(" "));
                Serial.print(bytes[i]);
        }
}

void send_emonpi_serial()
{
        Serial.print(F("OK "));
        Serial.print(nodeID);
        serial_print_bytes(sizeof(emonPi), &emonPi);
        Serial.print(F(" (-0)"));
        Serial.println();

        delay(10);
}

static void showString (PGM_P s) {
        for (;; ) {
                char c = pgm_read_byte(s++);
                if (c == 0)
                        break;
                if (c == '\n')
                        Serial.print('\r');
                Serial.print(c);
        }
}
