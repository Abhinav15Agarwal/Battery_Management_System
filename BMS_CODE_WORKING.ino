#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h" //built in header files
#include "LT_SPI.h"
#include "UserInterface.h"
#include "LTC68041.h"
#include <SPI.h>

#define sig 9 //BMS fault light

const uint8_t TOTAL_IC = 2; //number of ICs in the daisy chain
uint16_t cell_codes[TOTAL_IC][12]; // 12 cells total measurable
uint16_t aux_codes[TOTAL_IC][6]; //SIX GPIO PINS
uint8_t tx_cfg[TOTAL_IC][6]; //configuration data to be written onto the 6804
uint8_t rx_cfg[TOTAL_IC][8];//data that is read from the 6804


void setup()
{
    Serial.begin(115200);//transfer BAUD RATES
    LTC6804_initialize();//Initialize LTC6804 hardware, from the 68041.h header files.
    init_cfg();//initialize the 6804 configuration array to be written
    pinMode(sig,OUTPUT); //BMS fault light
    digitalWrite(sig,HIGH);//BMS fault light is not on until the error actualises, or the switch os from no to nc
}

void loop() //the infinite arduino loop
{
    int8_t error = 0;
    int t,w,tfc=0,fc=0,c=0,tc=0;
    //fc is to count the number of faulty runs before the allowable count is reached
    //c is a regular counter
    while(1)
    {
        c++;
        tc++;
        wakeup_sleep();//LTC68041.h
        LTC6804_wrcfg(TOTAL_IC,tx_cfg);//LTC68041.h
        wakeup_idle(); //LTC68041.h
        LTC6804_adcv();//LTC68041.h
        delay(10);
        LTC6804_adax();//LTC68041.h
        delay(3);
        wakeup_idle();//LTC68041.h
        error = LTC6804_rdcv(0, TOTAL_IC,cell_codes) || LTC6804_rdaux(0,TOTAL_IC,aux_codes);//LTC68041.h
        if (error == -1)
        {
            Serial.println("A PEC error was detected in the received data");// PEC is for packet error code which is a redundancy erroe check value
        }
        w=check_ovuv();
        t=check_ot();
        Serial.print("t=");
        Serial.println(t); 
        if(w==1)
            fc++;
        if(t==1)
        tfc++;
        
        if(((c==10)&&(fc==10))||((tc==10)&&(tfc==10)))
        {
            Serial.println("Fault detected");
            print_cells();
            print_aux();
            delay(500);
            for(;;)
            {
                digitalWrite(sig,LOW);   // turn the LED off (LOW is the voltage level)
                delay(1000);              // wait for a second≥
            }
        }
     
        if((fc!=10)&&(c==10))
        {
            fc=0;
            c=0;
        }
        if((tfc!=10)&&(tc==10))
        {
            tfc=0;
            tc=0;
        }
        print_cells();
        print_aux();
        delay(500);
    }
}
//check_ovuv is a fucntion to shut down the BMS, the "fault", if the cell voltage is either lesser than 2.8 or greater than 3.8, but is this initial measurement, or for the entire process. Its for the entire process, the internal counter does this opeation over and over.
int check_ovuv() //check undervoltage, overvoltage
{
    int e;
    for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
    {
        
        for (int i=0; i<8; i++)
        {
            
            if((cell_codes[current_ic][i]*0.0001>2.800) && (cell_codes[current_ic][i]*0.0001< 3.8000))
            {
                e=0;
                Serial.println(e,DEC);
            }
            else
            {
                e=1;
                Serial.println(e,DEC);
                goto fault;
            }
        }
    }
fault: return e;
}

void init_cfg()
{
    for (int i = 0; i<TOTAL_IC; i++)
    {
        tx_cfg[i][0] = 0xFE;
        tx_cfg[i][1] = 0x00 ;
        tx_cfg[i][2] = 0x00 ;
        tx_cfg[i][3] = 0x00 ;
        tx_cfg[i][4] = 0x00 ;
        tx_cfg[i][5] = 0x00 ;
    }
}

void print_cells()
{
    for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
    {
        Serial.print(" IC ");
        Serial.print(current_ic+1,DEC);
        for (int i=0; i<8; i++)
        {
            Serial.print(" C");
            Serial.print(i+1,DEC);
            Serial.print(":");
            Serial.print(cell_codes[current_ic][i]*0.0001,4);
            Serial.print(",");
        }
        Serial.println();
    }
    Serial.println();
}

int check_ot() //over teperature
{
    int o=0;
    for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
    {
        for (int i=0; i < 5; i++)
        {
            if(aux_codes[current_ic][i]*0.0001>0.3)
            {
                Serial.print(" GPIO-");
                Serial.print(i+1,DEC);
                Serial.print(":");
                Serial.print(aux_codes[current_ic][i]*0.0001,4);
                Serial.println("Overtemperature detected");
                o=1;
                Serial.println(o,DEC);
                goto fault;
            }
            else
            {
                o=0;
                Serial.println(o,DEC);
            }
        }
        //Serial.print(" Vref2"); //replace with print_aux and you'll monitor ot as well
        //Serial.print(":");
        //Serial.print(aux_codes[current_ic][5]*0.0001,4);
        Serial.println();
    }
    fault: return o;
    Serial.println();
}

void print_aux()
{
    
    for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
    {
        Serial.print(" IC ");
        Serial.print(current_ic+1,DEC);
        for (int i=0; i < 5; i++)
        {
            Serial.print(" GPIO-");
            Serial.print(i+1,DEC);
            Serial.print(":");
            Serial.print(aux_codes[current_ic][i]*0.0001,4);
            Serial.print(",");
        }
        //Serial.print(" Vref2");
        //Serial.print(":");
        //Serial.print(aux_codes[current_ic][5]*0.0001,4);
        Serial.println();
    }
    Serial.println();
}
