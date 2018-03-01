
void serial_print_startup()
{
        Serial.print(F("CT 1 Cal: ")); Serial.println(Ical1);
        Serial.print(F("CT 2 Cal: ")); Serial.println(Ical2);
        Serial.print(F("VRMS AC ~"));
        Serial.print(vrms); Serial.println(F("V"));

        Serial.print(F("Country mode "));
        Serial.println(country);
        if (ACAC) {
                Serial.println(F("AC Wave Detected - Real Power calc enabled"));
                Serial.print(F("Vcal: ")); Serial.println(Vcal);
                Serial.print(F("Vrms: ")); Serial.print(Vrms); Serial.println(F("V"));
                Serial.print(F("Phase Shift: ")); Serial.println(phase_shift);
        } else {
                Serial.println(F("AC NOT detected - Apparent Power calc enabled"));
                Serial.print(F("Assuming VRMS: "));
                Serial.print(Vrms); Serial.println(F("V"));
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
void send_emonpi_serial()
{
        byte binarray[sizeof(emonPi)];
        memcpy(binarray, &emonPi, sizeof(emonPi));

        Serial.print(F("OK "));
        Serial.print(nodeID);
        for (byte i = 0; i < sizeof(binarray); i++) {
                Serial.print(F(" "));
                Serial.print(binarray[i]);
        }
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
