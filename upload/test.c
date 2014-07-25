/* 
 * File:   test.c
 * Author: Henry
 *
 * Created on April 3, 2014, 5:10 PM
 */


#include "starting-routine.h"
#include <libusb-1.0/libusb.h>


/*
 * 
 */

#define CUBE_SIZE_DIFFERENCE .3


#define AREA_MISSMATCH_TOLERANCE 19
#define X_MISSMATCH_TOLERANCE 20

#define SLEEP_TIME 10

#define DRIVE_TOLERANCE 15
#define CUBE_APPROACH_EXTRA_OFFSET 0
#define APPROACH_TOLERANCE 10

#define ARC_TURN_RADIUS_CONSTANT 50000 // 75000

int average(int i1, int i2, int i3);
int arrAverage(int* arr, int length);
int preformApproach(bool leftCube);
void fourthRouteen();
void clearCamera();

int wait = 0;
void kill();

void shutDownAfter(int seconds) {
    wait = seconds;
    thread tr = thread_create(kill);
    thread_start(tr);
}

void kill() {
    printf("killing in %i.", wait * 1000);
    msleep(wait * 1000);
    disable_servos();
    create_drive_straight(0);
    motor(0, 0);
    motor(1, 0);
    motor(2, 0);
    motor(3, 0);
    exit(1);
}

void clearCamera() {
    int n = 0;
    while (n < 10) {
        camera_update();
        msleep(500);
        n++;

    }
}

int preformApproach(bool leftCube) {
    bool possibleApproach = false;

    int objX[3] = {0, 0, 0};
    int objArea[3] = {0, 0, 0};

    int objDataIndex = 0;
    bool objDataInitialized = false;

    int xSum = 0;
    int xEarlySum = 0;

    while (true) {


        camera_update();
        msleep(SLEEP_TIME);
        // **** GET NEW VALUE ****
        if (get_object_count(0) > 1) {
            int lobj0Area = get_object_area(0, 0);
            int lobj1Area = get_object_area(0, 1);
            int lobj1X = get_object_center_x(0, 1) - get_camera_width() / 2 + CUBE_APPROACH_EXTRA_OFFSET;
            int lobj0X = get_object_center_x(0, 0) - get_camera_width() / 2 + CUBE_APPROACH_EXTRA_OFFSET;
            if (lobj0Area - lobj1Area < CUBE_SIZE_DIFFERENCE * lobj0Area) {
                printf("Detecting two cubes, going with one on right.  Areas are: %i, %i.  Xs are: %i, %i.\n", lobj0Area, lobj1Area, lobj0X, lobj1X);
                if (leftCube ? lobj0X < lobj1X : lobj0X > lobj1X) {
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
                objX[objDataIndex] = get_object_center_x(0, 0) - get_camera_width() / 2 + CUBE_APPROACH_EXTRA_OFFSET;
            }
        } else if (get_object_count(0) > 0) {
            objArea[objDataIndex] = get_object_area(0, 0);
            objX[objDataIndex] = get_object_center_x(0, 0) - get_camera_width() / 2 + CUBE_APPROACH_EXTRA_OFFSET;
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
        if (abs(averageArea - objArea[0]) / averageArea > AREA_MISSMATCH_TOLERANCE || abs(averageArea - objArea[1]) / averageArea > AREA_MISSMATCH_TOLERANCE || abs(averageArea - objArea[2]) / averageArea > AREA_MISSMATCH_TOLERANCE) {
            printf("Area mismatch: a1, a2, a3: %i, %i, %i\n", objArea[0], objArea[1], objArea[2]);
            continue;
        }
        printf("area consistent at: %i\n", averageArea);

        int averageX = arrAverage(objX, 3);
        if (abs(averageX - objX[0]) > X_MISSMATCH_TOLERANCE || abs(averageX - objX[1]) > X_MISSMATCH_TOLERANCE || abs(averageX - objX[2]) > X_MISSMATCH_TOLERANCE) {
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

            create_drive_straight(-60);
            struct timeval that_start_time;
            gettimeofday(&that_start_time, NULL);
            struct timeval that_end_time;
            gettimeofday(&that_end_time, NULL);
            while (analog(0) > 910 && that_end_time.tv_sec - that_start_time.tv_sec < 5) {//            !!!!!!!!!!!!!!!!!was 840
                gettimeofday(&that_end_time, NULL);
            }
            printf("xsum: %i\n", xSum);
            printf("xearly sum: %i\n", xEarlySum);
            msleep(800);
            create_drive_straight(0);
            move_claw_amount(CLAW_CLOSE_AMOUNT);
            motor(0, 4);
            printf("finished final approach routeen.\n");
            return xSum;
        }

        if (abs(averageX) < DRIVE_TOLERANCE) {
            create_drive_straight(-60);
        } else if (averageX < 0) {
            create_drive(-60, (double) ARC_TURN_RADIUS_CONSTANT / (double) averageX);
        } else {
            //positive radius
            create_drive(-60, (double) ARC_TURN_RADIUS_CONSTANT / (double) averageX);
        }

    }

    printf("APPROACH FAILED!\n");

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
    return (xSum);
}

int average(int i1, int i2, int i3) {
    return (i1 + i2 + i3) / 3;
}

int arrAverage(int* arr, int length) {
    int avr = 0;
    for (int i = 0; i < length; i++) {
        avr += arr[i];
    }
    return avr / length;
}

void cubeLoop() {
    disable_servos();
    while (true) {
        create_drive_straight(0);
        preformApproach(false);
        raise_claw_to(CLAW_UP_POSITION);
        enable_servo(CLAW_LEFT);
        enable_servo(CLAW_RIGHT);

        create_drive_straight(200);
        
        msleep(800);
        
        create_spin_CW(100); //PERFECT 90
        msleep(2040);
        create_drive_straight(0);
        create_drive_straight(200);
        
        while (!(get_create_rbump() || get_create_lbump())) {

        }

        
        
        create_spin_CW(100); 
        while (!digital(9)) {
        }
        create_drive_straight(0);
        
        create_spin_CCW(100);
        msleep(400);
        
        
        
        create_drive_straight(-150);
        msleep(3000);
        move_claw_amount(CLAW_OPEN_AMOUNT);
        create_drive_straight(150);
        msleep(1000);
        create_spin_CCW(100); //PERFECT 90
        msleep(2040);
        create_drive_straight(0);
        create_spin_CCW(100); //PERFECT 90
        msleep(2040);
        create_drive_straight(0);
        disable_servo(CLAW_LEFT);
        disable_servo(CLAW_RIGHT);
        clearCamera();
    }
}

int main(int argc, char** argv) {

    shutDownAfter(118);
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    printf("Create connecting...\n");
    printf("camera open response: %i\n", camera_open());
    create_connect();
    create_drive_straight(0);
    raise_claw_to(CLAW_UP_POSITION);
    enable_servo(CLAW_LEFT);
    enable_servo(CLAW_RIGHT);
    
    
    printf("waiting for input...\n");
    char s[20];
    scanf("%s", s);
    if (strcmp(s, "exit") == 0) {
        return 0;
    }
    preformStartingRoutine();
    msleep(500);
    
    
    create_drive_straight(300);
    msleep(700);
    create_drive_straight(0);
    create_spin_CCW(150);
    msleep(1000);
    create_drive_straight(-300);
    msleep(1000);
    create_spin_CW(150);
    msleep(1300);
    
    create_drive_straight(0);
    printf("facing cubes.\n");
    

    cubeLoop();

    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    printf("program complete.  Elapsed time in seconds is: %i.\n", (int) (end_time.tv_sec)-(int) (start_time.tv_sec));
}