#include <Arduino.h>
#include <stdint.h>
#include <Linduino.h>
#include <UserInterface.h>
#include <EEPROM.h>
#include <Communication.h>
extern "C" {
#include "ADT7420.h"
};

void setup()
{
    Serial.begin(115200);
    delay(100);

    uint8_t status = ADT7420_Init();

    print_title();
    load_eeprom();
    Serial.print(F("Status of part: "));
    Serial.println(status);
}

void loop()
{
    print_prompt();

    uint8_t user_command = read_int();
    Serial.println(user_command);
    Serial.flush();

    switch (user_command)
    {
    case 1:
        menu_1_read_temperature();
        break;

    case 2:
        menu_2_set_resolution();
        break;

    case 3:
        menu_3_set_op_mode();
        break;

    case 4:
        menu_4_bunchoftemps();
        break;

    case 5:
        menu_5_critical();
        break;

    case 9:
        EEPROM_WRITE_TEST();
        break;

    default:
        Serial.println(F("Invalid option"));
        break;
    }
}

void load_eeprom()
{
    uint8_t EEPROM_BaseAddr = 64;
    uint8_t EEPROM_Saved = 0;
    uint8_t EEPROM_Resolution = 0;

    EEPROM_Saved = EEPROM.read(EEPROM_BaseAddr);

    if (EEPROM_Saved)
    {
        Serial.println(F("\nLoading data from EEPROM"));

        EEPROM_Resolution = EEPROM.read(EEPROM_BaseAddr + 1);

        Serial.print(F("  Initial resolution: "));
        Serial.print(13 + 3 * EEPROM_Resolution);
        Serial.println(F("-bit"));
        
        ADT7420_SetResolution(EEPROM_Resolution);
    }
    else
    {
        Serial.println(F("No EEPROM data saved"));
    }
}

void print_title()
{
    Serial.println(F("*****************************************************************"));
    Serial.println(F("* EVAL-7420SDZ Demonstration Program                            *"));
    Serial.println(F("*                                                               *"));
    Serial.println(F("* This program demonstrates communication with the ADT7420      *"));
    Serial.println(F("* high accuracy digital temperature sensor                      *"));
    Serial.println(F("*                                                               *"));
    Serial.println(F("* Set the baud rate to 115200 select the newline terminator.    *"));
    Serial.println(F("*****************************************************************"));
}

void print_prompt()
{
    Serial.println(F("\nCommand Summary:"));

    Serial.println(F("  1- Read temperature"));
    Serial.println(F("  2- Set resolution"));
    Serial.println(F("  3- Set operation mode"));
    Serial.println(F("  4- Poll for a bunch of temperatures"));
    Serial.println(F("  5- Get critical temperature setting"));
    Serial.println(F("  9- Test & clear Linduino EEPROM"));
    Serial.println();

    Serial.print(F("Enter a command: "));
}

uint8_t menu_1_read_temperature()
{
    float temp = ADT7420_GetTemperature();

    Serial.print("Current temperature: ");
    Serial.print(temp);
    Serial.println(F(" C"));
}

uint8_t menu_2_set_resolution()
{
    Serial.println(F("  Available resolutions:"));
    Serial.println(F("    1- 13-bit"));
    Serial.println(F("    2- 16-bit"));
    Serial.print(F("  Select an option: "));

    uint8_t new_res = read_int();
    Serial.println(new_res);

    new_res = (new_res == 1) ? 0 : 1;

    ADT7420_SetResolution(new_res);

    Serial.print(F("Set resolution to "));
    Serial.print((13 + 3 * new_res));
    Serial.println(F("-bit"));

    EEPROM.write(64, 1);
    EEPROM.write(65, new_res);

    return 0;
}

uint8_t menu_3_set_op_mode()
{
    Serial.println(F("  Available operation modes:"));
    Serial.println(F("    1- Continuous conversion mode (default)"));
    Serial.println(F("    2- One-shot mode"));
    Serial.println(F("    3- 1 SPS mode"));
    Serial.println(F("    4- Shutdown"));
    Serial.print(F("  Select a mode: "));

    uint8_t new_mode = read_int();
    Serial.println(new_mode);

    switch (new_mode)
    {
    case 1:
        ADT7420_SetOperationMode(ADT7420_OP_MODE_CONT_CONV);
        break;

    case 2:
        ADT7420_SetOperationMode(ADT7420_OP_MODE_ONE_SHOT);
        break;

    case 3:
        ADT7420_SetOperationMode(ADT7420_OP_MODE_1_SPS);
        break;

    case 4:
        ADT7420_SetOperationMode(ADT7420_OP_MODE_SHUTDOWN);
        break;

    default:
        Serial.println(F("Invalid option"));
        break;
    }

    return 0;
}

uint8_t menu_4_bunchoftemps()
{
    Serial.print(F("  Enter number of desired samples: "));
    uint16_t num_samples = read_int();
    Serial.println(num_samples);

    Serial.print(F("  Enter a desired frequency in samples/sec (max 10): "));
    uint16_t sample_freq = read_int();
    sample_freq = constrain(sample_freq, 1, 10);
    Serial.println(sample_freq);

    uint16_t delay_sec = 1000 / sample_freq;

    Serial.print(F("  Gathering "));
    Serial.print(num_samples / sample_freq);
    Serial.println(F(" seconds of samples, press enter to continue"));

    uint8_t temp = read_int();

    for (int i = 0; i < num_samples; i++)
    {
        Serial.print(F("  #"));
        Serial.print(i + 1);
        Serial.print(F(":\t"));

        float temp = ADT7420_GetTemperature();
        Serial.println(temp);

        delay(delay_sec);
    }

    return 0;
}

void menu_5_critical()
{
    uint8_t msbCrit = 0;
    uint8_t lsbCrit = 0;
    uint16_t crit = 0;
    float critC = 0;

    msbCrit = ADT7420_GetRegisterValue(ADT7420_REG_T_CRIT_MSB);
    lsbCrit = ADT7420_GetRegisterValue(ADT7420_REG_T_CRIT_LSB);

    crit = ((uint16_t)msbCrit << 8) + lsbCrit;

    //Serial.print(F("Critical register values: "));
    //Serial.print(msbCrit, HEX);
    //Serial.println(lsbCrit, HEX);

    if (1)
    {
        if (crit & 0x8000)
        {
            /*! Negative temperature */
            critC = (float)((signed long)crit - 65536) / 128;
        }
        else
        {
            /*! Positive temperature */
            critC = (float)crit / 128;
        }
    }
    else
    {
        crit >>= 3;
        if (crit & 0x1000)
        {
            /*! Negative temperature */
            critC = (float)((signed long)crit - 8192) / 16;
        }
        else
        {
            /*! Positive temperature */
            critC = (float)crit / 16;
        }
    }

    Serial.print(F("Critical temperature is: "));
    Serial.println(critC);

    uint8_t msbLow = ADT7420_GetRegisterValue(ADT7420_REG_T_LOW_MSB);
    uint8_t lsbLow = ADT7420_GetRegisterValue(ADT7420_REG_T_LOW_LSB);
    uint16_t low = ((uint16_t)msbLow << 8) + lsbLow;
    uint16_t lowC = 0;

    if (low & 0x8000)
    {
        /*! Negative temperature */
        lowC = (float)((signed long)low - 65536) / 128;
    }
    else
    {
        /*! Positive temperature */
        lowC = (float)low / 128;
    }

    Serial.print(F("Low temperature is: "));
    Serial.println(lowC);

    float testtemp = 147;
    Serial.print(F("Temp to code test; converting "));
    Serial.println(testtemp);

    uint8_t *msb;
    uint8_t *lsb;

    temp_to_code(testtemp, msb, lsb, ADT7420_GetResolutionSetting());

    Serial.print(F("MSB: "));
    Serial.print(*msb, HEX);
    Serial.print(F(", LSB: "));
    Serial.println(*lsb, HEX);
}

/** @GREG this could potentially be added to the driver?
 * Converts a temperature float into a most and least significant byte
 * that the sensor understands.
 * 
 * @param temp - temperature float
 * @param msb - pointer to most sig byte
 * @param lsb - pointer to least sig byte
 * @param resolution - 0 for 13-bit, 1 for 16-bit
 */
void temp_to_code(float temp, uint8_t *msb, uint8_t *lsb, uint8_t resolution) // res of 0 = 13 bit, 1 = 16bit
{
    uint16_t code = 0;
    
    if(resolution)
    {
        if (temp < 0)
        {
            code = (uint16_t)((temp * 128) + 65536);
        }
        else
        {
            code = (uint16_t)(temp * 128);
        }
    }
    else
    {
        if(temp < 0)
        {
            code = (uint16_t) (temp * 16) + 8192;
        }
        else
        {
            code = (uint16_t) (temp * 16) + 8192;
        }
        
        code <<= 3;
    }

    *msb = (uint8_t)(code >> 8);
    *lsb = (uint8_t)(code & 255);
}

void EEPROM_WRITE_TEST()
{
    char towrite[] = "Hello World";

    uint16_t baseaddr = 255;
    uint8_t length = 11;

    for (int i = 0; i < length; i++)
    {
        uint16_t addr = baseaddr + i;
        EEPROM.write(addr, towrite[i]);
    }

    char readback[11];

    for (int i = 0; i < length; i++)
    {
        uint16_t addr = baseaddr + i;
        readback[i] = EEPROM.read(addr);
    }

    Serial.print(F("Read from EEPROM: "));

    for (int i = 0; i < length; i++)
    {
        Serial.print(readback[i]);
    }

    Serial.println("");
    
    Serial.println(F("Clearing EEPROM..."));
    for (int i = 0; i < EEPROM.length(); i++)
    {
        EEPROM.write(i, 0);
    }

    Serial.println(F("EEPROM Cleared"));

    /*Serial.println(F("TESTING EEPROM, WRITING 69"));
    EEPROM.write(255, 69);
    delay(50);
    byte prom = EEPROM.read(255);
    
    Serial.print(F("Value on EEPROM is: "));
    Serial.println(prom, DEC);*/
}
