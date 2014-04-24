/* 
 * File:   test.c
 * Author: Henry
 *
 * Created on April 3, 2014, 5:10 PM
 */

#include <stdlib.h>
#include <stdio.h>
#include <kovan/kovan.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <libusb-1.0/libusb.h>

#include "claw-utils.h"

/*
 * 
 */

#define CUBE_SIZE_DIFFERENCE .3


#define AREA_MISSMATCH_TOLERANCE 19
#define X_MISSMATCH_TOLERANCE 20

#define SLEEP_TIME 10

#define DRIVE_TOLERANCE 15

#define APPROACH_TOLERANCE 10

#define ARC_TURN_RADIUS_CONSTANT 50000 // 75000

int average(int i1, int i2, int i3);
int arrAverage(int* arr, int length);

void openingRouteen() {
    create_drive_straight(0);
    create_drive_straight(-300);
    msleep(851);
    create_drive_straight(0);
    create_spin_CW(150);
    msleep(1291);
    create_drive_straight(0);
    create_drive_straight(-140);
    
    while (!digital(15)) {
        
    }
    
    //msleep(2031); //This block shoud be replcaed with a touch sensor check
    
    create_drive_straight(0);
    move_claw_amount(CLAW_CLOSE_AMOUNT);
}

void secondRouteen(int xSum) {
    create_drive_straight(0);
    create_drive_straight(150);
    msleep(511);
    create_drive_straight(0);
    create_spin_CCW(150);
    msleep(1210 + (xSum - 1000) / 30);
    create_drive_straight(0);
    create_drive_straight(-300);
    msleep(6500);
    create_drive_straight(0);
    create_drive_straight(200);
    msleep(271);
    create_drive_straight(0);
    create_spin_CW(150);
    msleep(1350);   //This is the code the make a 90% turn.
    create_drive_straight(0);
    create_drive_straight(-300);
    msleep(2000);
    create_drive_straight(0);
}

void thirdRouteen() {
    create_drive_straight(0);

    create_drive_straight(300);

    msleep(1129);

    create_drive_straight(0);

    raise_claw_to(CLAW_MIDDLE_POSITION);

    create_spin_CCW(150);

    msleep(2492);

    create_drive_straight(0);

    create_drive_straight(-70);

    msleep(1900);

    move_claw_amount(CLAW_OPEN_AMOUNT);
create_drive_straight(0);


    

}

int main(int argc, char** argv) {
    create_connect();
    create_drive_straight(0);
    
    raise_claw_to(CLAW_UP_POSITION);
    enable_servos();
    msleep(1000);
    printf("camera open response: %i\n", camera_open());
    
    create_spin_CW(150);
    msleep(1350); 
    create_drive_straight(0);
    
    bool possibleApproach = false;
    
    int objX[3] = {0, 0, 0};
    int objArea[3] = {0, 0, 0};
    
    int objDataIndex = 0;
    bool objDataInitialized = false;
    
    int xSum = 0;
    int xEarlySum = 0;
    
    while (true) {
        msleep(SLEEP_TIME);
        
        camera_update();
        
        // **** GET NEW VALUE ****
        if (get_object_count(0) > 1) {
            int lobj0Area = get_object_area(0,0);
            int lobj1Area = get_object_area(0,1);
            int lobj1X = get_object_center_x(0,1) - get_camera_width() / 2;
            int lobj0X = get_object_center_x(0,0) - get_camera_width() / 2;
            if (lobj0Area - lobj1Area < CUBE_SIZE_DIFFERENCE * lobj0Area) {
                printf("Detecting two cubes, going with one on right.  Areas are: %i, %i.  Xs are: %i, %i.\n", lobj0Area, lobj1Area, lobj0X, lobj1X);
                if (lobj0X > lobj1X) {
                    //Go with object 0
                    objX[objDataIndex] = lobj0X;
                    objArea[objDataIndex] = lobj0Area;
                } else {
                    //Go with object 1
                    objX[objDataIndex] = lobj1X;
                    objArea[objDataIndex] = lobj1Area;
                }
            } else {
                objArea[objDataIndex] = get_object_area(0, 0);
                objX[objDataIndex] = get_object_center_x(0, 0) - get_camera_width() / 2;
            }
        } else if (get_object_count(0) > 0) {
            objArea[objDataIndex] = get_object_area(0,0);
            objX[objDataIndex] = get_object_center_x(0,0) - get_camera_width() / 2;
        } else {
            printf("did not find object\n");
            continue;
        }
        
        // **** UPDATE DATA COUNTER ****
        if (!objDataInitialized) {
            objX[1] = objX[2] = objX[0];
            objArea[1] = objArea[2] = objArea[0];
            objDataInitialized = true;
        }
        objDataIndex = (objDataIndex + 1) % 3;
        
        // **** ADJUST CALIBRATION ****
        int averageArea = arrAverage(objArea, 3);
        if (abs(averageArea - objArea[0]) / averageArea >  AREA_MISSMATCH_TOLERANCE || abs(averageArea - objArea[1]) / averageArea >  AREA_MISSMATCH_TOLERANCE || abs(averageArea - objArea[2]) / averageArea >  AREA_MISSMATCH_TOLERANCE) {
            printf("Area mismatch: a1, a2, a3: %i, %i, %i\n", objArea[0], objArea[1], objArea[2]);
            continue;
        }
        printf("area consistent at: %i\n", averageArea);
        
        int averageX = arrAverage(objX, 3);
        if (abs(averageX - objX[0]) >  X_MISSMATCH_TOLERANCE || abs(averageX - objX[1]) >  X_MISSMATCH_TOLERANCE || abs(averageX - objX[2]) >  X_MISSMATCH_TOLERANCE) {
            printf("X mismatch: a1, a2, a3: %i, %i, %i\n", objX[0], objX[1], objX[2]);
            continue;
        }
        printf("X consistent at: %i\n", averageX);
        
        xSum += averageX;
        if (averageArea < 700) {
            xEarlySum += averageX;
        }
        if (averageArea > 2000) {
            possibleApproach = true;
        }
        if (possibleApproach) {
            printf("initial approach complete.\n");
            printf("finished final approach routeen.\n");
            create_drive_straight(-60);
            while (analog(0) > 800) {
            }
            printf("xsum: %i\n", xSum);
            printf("xearly sum: %i\n", xEarlySum);
            msleep(800);
            create_drive_straight(0);
            move_claw_amount(CLAW_CLOSE_AMOUNT);
            motor(0, 4);
            // LOL THIS IS EXIT POINT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            secondRouteen(xSum);
            thirdRouteen();
            
            return 0; 
        }
        
        if (abs(averageX) < DRIVE_TOLERANCE) {
            create_drive_straight(-60);
        } else if (averageX < 0) {
            create_drive(-60, (double)ARC_TURN_RADIUS_CONSTANT / (double)averageX );
        } else {
            //positive radius
            create_drive(-60, (double)ARC_TURN_RADIUS_CONSTANT / (double)averageX);
        }

    }
    
    camera_close();
    
    //printf("area: %i", get_object_area(0,0));
    /*
    raise_claw_to(CLAW_UP_POSITION);
    enable_servos();
    while (!digital(15)) {}
    move_claw_amount(CLAW_CLOSE_AMOUNT);*/
    //openingRouteen();
    //thirdRouteen();
    //move_claw_amount(CLAW_CLOSE_AMOUNT);
    //disable_servos();
    return (EXIT_SUCCESS);
}

int average(int i1, int i2, int i3) {
    return (i1 + i2 + i3) / 3;
}

int arrAverage(int* arr, int length) {
    int avr = 0;
    for (int i = 0; i < length; i++) {
        avr += arr[i];
    }
    return avr/length;
}