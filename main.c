/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/main.c to edit this template
 */

/* 
 * File:   main.c
 * Author: bluebit
 *
 * Created on 7 stycznia 2025, 18:47
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>   //Used for UART
#include <fcntl.h>   //Used for UART
#include <termios.h>  //Used for UART
#include <stdlib.h>
#include <time.h>                   //time
#include <string.h>
#include <stdbool.h>  //bool
#include <errno.h>

unsigned char Rx_flag_lineready = 0;
unsigned char Rx_bufindex = 0;
#define Rx_bufindex_MAX 254
unsigned char Rx_buf[Rx_bufindex_MAX];
int uart_filestream = -1;
unsigned char Receive_data_from_BMS();

struct BMSdata {
    unsigned short CumulativeVoltage;
    short Current;
    unsigned short SOC;
    unsigned short Power;
    unsigned short Minimal_cell_voltage;
    unsigned short Maximal_cell_voltage;
    unsigned char BatteryTemp;
    unsigned char State;
    unsigned short CellVoltage[21];
    unsigned char Error[7];
} BMS;

int main(int argc, char** argv) {


    uart_filestream = open("/dev/ttyUSB2", O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
    if (uart_filestream == -1) {
        printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
        return -1;
    }
    struct termios options;
    tcgetattr(uart_filestream, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD; //<Set baud rate
    options.c_cflag &= ~(PARODD | CSTOPB);
    options.c_iflag = IGNPAR; //PARENB;
    options.c_oflag = 0;
    options.c_oflag &= ~OPOST;
    options.c_lflag = 0;
    tcflush(uart_filestream, TCIFLUSH);
    tcsetattr(uart_filestream, TCSANOW, &options);

    unsigned char debug = 1;
    for (unsigned char ask = 0; ask <= 8; ask++) {
        if (ask == 4 || ask == 6 || ask == 7)continue;
        snprintf(buffor, 15, "%c%c%c%c%c%c%c%c%c%c%c%c%c", 0xA5, 0x40, 0x90 + ask, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D + ask);
        write(uart_filestream, buffor, 13);
        usleep(100000);
        while (Receive_data_from_BMS()) {
            if (Rx_flag_lineready) {
                switch (Rx_buf[2]) {
                    case 0x90:
                        BMS.CumulativeVoltage = Rx_buf[4] << 8 | Rx_buf[5];
                        BMS.Current = (Rx_buf[8] << 8 | Rx_buf[9]) - 30000;
                        BMS.SOC = Rx_buf[10] << 8 | Rx_buf[11];
                        if (debug)printf("Cumulative Voltage    : %.2f V \r\n", ((float) BMS.CumulativeVoltage) / 10.0);
                        if (debug)printf("Current               : %.2f A\r\n", ((float) BMS.Current) / 10.0);
                        if (debug)printf("Soc                   : %.2f %c\r\n", ((float) BMS.SOC) / 10.0,'%');
                        break;

                    case 0x91:
                        BMS.Maximal_cell_voltage = Rx_buf[4] << 8 | Rx_buf[5];
                        BMS.Minimal_cell_voltage = Rx_buf[7] << 8 | Rx_buf[8];
                        if (debug)printf("Maximal cell(%02d)      : %d mV\r\n", Rx_buf[6], BMS.Maximal_cell_voltage);
                        if (debug)printf("Minimal cell(%02d)      : %d mV\r\n", Rx_buf[9], BMS.Minimal_cell_voltage);
                        break;

                    case 0x92:
                        BMS.BatteryTemp = Rx_buf[4] - 40;
                        if (debug)printf("Maximum temp. value   : %d *C (at cell %d)\r\n", Rx_buf[4] - 40, Rx_buf[5]);
                        if (debug)printf("Minimum temp. value   : %d *C (at cell %d)\r\n", Rx_buf[6] - 40, Rx_buf[7]);
                        break;

                    case 0x93:
                        BMS.State = Rx_buf[4];
                        if (debug)printf("State                 : %d (%s)\r\n", Rx_buf[4], Rx_buf[4] == 0 ? "Stationary" : (Rx_buf[4] == 1) ? "charge" : (Rx_buf[4] == 2) ? "discharge" : "unknown");
                        if (debug)printf("Charge MOS state      : %d (%s)\r\n", Rx_buf[5],Rx_buf[5]==1?"ON":"OFF");
                        if (debug)printf("Discharge MOS state   : %d (%s)\r\n", Rx_buf[6],Rx_buf[6]==1?"ON":"OFF");
                        if (debug)printf("BMS life              : %d \r\n", Rx_buf[7]);
                        if (debug)printf("Remain capacity       : %d mAh\r\n", Rx_buf[8] << 24 | Rx_buf[9] << 16 | Rx_buf[10] << 8 | Rx_buf[11]);
                        break;

                    case 0x95:
                        BMS.CellVoltage[((Rx_buf[4] - 1)*3) + 1] = Rx_buf[5] << 8 | Rx_buf[6];
                        BMS.CellVoltage[((Rx_buf[4] - 1)*3) + 2] = Rx_buf[7] << 8 | Rx_buf[8];
                        BMS.CellVoltage[((Rx_buf[4] - 1)*3) + 3] = Rx_buf[9] << 8 | Rx_buf[10];
                        
                        if (debug)printf("Cell %02d               : %d mV \r\n", ((Rx_buf[4] - 1)*3) + 1, Rx_buf[5] << 8 | Rx_buf[6]);
                        if (debug)printf("Cell %02d               : %d mV \r\n", ((Rx_buf[4] - 1)*3) + 2, Rx_buf[7] << 8 | Rx_buf[8]);
                        if (debug)printf("Cell %02d               : %d mV \r\n", ((Rx_buf[4] - 1)*3) + 3, Rx_buf[9] << 8 | Rx_buf[10]);
                        break;

                    case 0x98:
                        for (int i = 0; i < 8; i++)BMS.Error[i] = Rx_buf[4 + i];
                        if (debug)printf("Errors bytes          : %02X %02X %02X %02X %02X %02X %02X %02X \r\n", Rx_buf[4], Rx_buf[5], Rx_buf[6], Rx_buf[7], Rx_buf[8], Rx_buf[9], Rx_buf[10], Rx_buf[11]);
                        break;

                    default:
                        if (debug) {
                                  printf("Received Unknown      : ");
                            for (int i = 0; i < Rx_bufindex; i++)printf("%02X ", Rx_buf[i]);
                            printf("\r\n");
                        }
                        break;
                }

                Rx_flag_lineready = 0;
            } else {
                printf("Nic nie odebralem \r\n");
            }
        }//while
    }
    close(uart_filestream );
    return (EXIT_SUCCESS);
}




unsigned char Start_char_read = 0;
unsigned char czesc_danych = 0;
unsigned char Rx_char_read2 = 0;
char Rx_char_read;
unsigned char bajt = 0;
unsigned char ktory_bajt = 0;
unsigned char UART_error = 0;

unsigned char Receive_data_from_BMS() {
    if (uart_filestream != -1) { //jesli jest wskaxnik
        while ((read(uart_filestream, &Rx_char_read, 1) > 0)&&(Rx_flag_lineready == 0)) {//dopóki możemy odebrać dane    
            Rx_char_read = Rx_char_read & 0xFF;
            if (Start_char_read) {
                Rx_buf[Rx_bufindex] = Rx_char_read;
                Rx_bufindex++;
            } else {

                if ((Rx_char_read & 0xFF) == 0xA5) {
                    Start_char_read = 1;
                    Rx_bufindex = 0;
                    Rx_buf[Rx_bufindex] = Rx_char_read;
                    Rx_bufindex++;
                }

            }
            if (Rx_bufindex == 13)break;
        }//while  

        if (Start_char_read) {
            Start_char_read = 0;
            unsigned char sum = 0;

            for (int i = 0; i < Rx_bufindex - 1; i++) {
                sum += Rx_buf[i];
            }
            if (sum == Rx_buf[Rx_bufindex - 1]) {
                Rx_flag_lineready = 1;
            } else {
                printf("CheckSum error \r\n");

            }
        }
    }//if
    return Rx_flag_lineready;
}