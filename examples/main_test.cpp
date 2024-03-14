#define MAX_GLADIATOR 2
#define MAX_ENNEMIES 1
#define MAP_SIZE 12
#define KP 5
#define KI 5
#define KD 0
#define ANGLE_PRECISION 0.1
#include "gladiator.h"
#include <iostream>
#include <cmath>
#include <vector>
Gladiator* gladiator1;
byte PIFOU_ID;


double speed;
int reached;

enum state_e{
    INIT,
    THINKING,
    MOVING,
    REACH_AREA,
    SHOOT
};

state_e currentState=INIT;

enum direction_e{
    UP,
    DOWN,
    LEFT, 
    RIGHT
};

direction_e dir;
uint8_t area[12][12];
unsigned long start_time=millis();
unsigned long duration=20000;
int offset=0;
int k=0;

void clearMap(){
    for(int i = 0 ; i<12;i++){
        for(int j = 0 ; j<12;j++){
            area[i][j]=0;
        }
    }
}

void createMap(int offset){
    for(int i = 0 ; i<12-offset*2;i++){
        for(int j = 0 ; j<12-offset*2;j++){
        const MazeSquare * square = gladiator1->maze->getSquare(i+offset,j+offset);
        if(square->northSquare==nullptr){ 
            area[j][i]+=1;
        }
        if(square->eastSquare==nullptr){
            area[j][i]+=2;
        }
        if(square->southSquare==nullptr){
            area[j][i]+=4;
        }
        if(square->westSquare==nullptr){
            area[j][i]+=8;
        }
        if(square->coin.value==1){
            area[j][i]+=16;
        }
        if(square->danger==true){
            area[j][i]+=32;
        }
        if(square->possession==0){
            area[j][i]+=64;
        }
        else if(square->possession==PIFOU_ID){
            area[j][i]+=128;
        }
        else{
            area[j][i]+=128+64;
            }
        }
    }
}

void updateMap(){
    for(int i = 0 ; i<12-offset*2;i++){
        for(int j = 0 ; j<12-offset*2;j++){
        const MazeSquare * square = gladiator1->maze->getSquare(i+offset,j+offset);
        if(square->coin.value==1){
            area[j][i]+=16;
        }
        if(square->danger==true){
            area[j][i]+=32;
        }
        if(square->possession==0){
            area[j][i]+=64;
        }
        else if(square->possession==1){
            area[j][i]+=128;
        }
        else{
            area[j][i]+=128+64;
            }
        }
    }
}

double getDiffAngle(double x1, double y1, double x2, double y2){
    double diffX = x2 - x1;
    double diffY = y2 - y1;
    return atan2(diffY, diffX);
}

void gladiatorGoTo(double targetX , double targetY){
    double tolerance = 0.1;
    RobotData data = gladiator1->robot->getData();
    Position position = data.position;
    double currentX = position.x;
    double currentY = position.y;
    double currentAngle = position.a;
    double targetAngle = getDiffAngle(currentX, currentY, targetX, targetY);
    double diffAngle = targetAngle - currentAngle;
    gladiator1->log("diffangle : %f", diffAngle);
    if (abs(diffAngle) > tolerance){
        speed = 0;
        if (diffAngle < 0 ){
            gladiator1->control->setWheelSpeed(WheelAxis::RIGHT, -0.6*abs(sin(diffAngle)));
            gladiator1->control->setWheelSpeed(WheelAxis::LEFT, 0.6*abs(sin(diffAngle)));
        }else {
            gladiator1->control->setWheelSpeed(WheelAxis::RIGHT, 0.6*abs(sin(diffAngle)));
            gladiator1->control->setWheelSpeed(WheelAxis::LEFT, -0.6*abs(sin(diffAngle)));
        }
    }
    else{
        speed += 0.1;
        if (speed >= 0.6){
            speed = 0.6;
        }
        gladiator1->control->setWheelSpeed(WheelAxis::RIGHT,speed);
        gladiator1->control->setWheelSpeed(WheelAxis::LEFT, speed);
    }
}

int compareSquare(MazeSquare s1, MazeSquare s2){
    return (s1.i == s2.i && s1.j == s2.j);
}

int gladiatorNextTile(){
    const MazeSquare* nearestSquare = gladiator1->maze->getNearestSquare();
    float squareSize = gladiator1->maze->getSquareSize();
    Position centerCoor;
    centerCoor.x = (targetSquare->i + 0.5) * squareSize;
    centerCoor.y = (targetSquare->j + 0.5) * squareSize;
    gladiatorGoTo(centerCoor.x, centerCoor.y);
}

/* 
    On rentre en paramètre une liste vide et on obtient la liste des 2 ennemies
*/ 
void getEnemies(int enemies[MAX_ENNEMIES]){
    int index = 0;
    RobotList robotList = gladiator1->game->getPlayingRobotsId();
    for(int i=0; i<MAX_GLADIATOR; i++){
        unsigned char id = robotList.ids[i];
        RobotData data = gladiator1->game->getOtherRobotData(id);
        if (data.teamId != PIFOU_ID){
            enemies[index] = data.teamId;
            index ++;
        }
    }
}

int isNorthSquareValid(int i, int j){
    const MazeSquare* square = gladiator1->maze->getSquare(i, j);
    if (square->northSquare==nullptr) {
        return 0;
    }
    return 1;
}

int isSouthSquareValid(int i, int j){
    const MazeSquare* square = gladiator1->maze->getSquare(i, j);
    if (square->southSquare==nullptr) {
        return 0;
    }
    return 1;
}

int isWestSquareValid(int i, int j){
    const MazeSquare* square = gladiator1->maze->getSquare(i, j);
    if (square->westSquare==nullptr) {
        return 0;
    }
    return 1;
}

int isEastSquareValid(int i, int j){
    const MazeSquare* square = gladiator1->maze->getSquare(i, j);
    if (square->eastSquare==nullptr) {
        return 0;
    }
    return 1;
}

void reset();
void setup() {
    //instanciation de l'objet gladiator1
    gladiator1 = new Gladiator();
    //enregistrement de la fonction de reset qui s'éxecute à chaque fois avant qu'une partie commence
    gladiator1->game->onReset(&reset); // GFA 4.4.1
    gladiator1->control->setWheelPidCoefs(WheelAxis::RIGHT, KP , KI , KD);
    gladiator1->control->setWheelPidCoefs(WheelAxis::LEFT , KP , KI , KD);
    speed = 0;
}

void reset() {
    //fonction de reset:
    //initialisation de toutes vos variables avant le début d'un match
    gladiator1->log("Call of reset function"); // GFA 4.5.1
}

void init(){
    RobotData data = gladiator1->robot->getData();
    PIFOU_ID = data.teamId;
}

int isSquareUncatch(int i, int j){
    return ((area[i][j] & (1 << 6)) || ((area[i][j] & 0b11000000) == 0b11000000));
}

int thinking(){
    if(millis()-start_time>duration*k){
        clearMap();
        createMap(offset);
        start_time=millis();
        k=1;
        offset+=1;
    }
    // updateMap();  
    const MazeSquare* nearestSquare = gladiator1->maze->getNearestSquare();
    int currentI = nearestSquare->i;
    int currentJ = nearestSquare->j;
    gladiator1->log("current %d : %d", currentI, currentJ);
    if(isNorthSquareValid(currentI, currentJ) /*|| isSquareUncatch(currentI, currentJ+1)*/){
        gladiator1->log("go north");
        targetSquare = nearestSquare->northSquare;
    }else if(isEastSquareValid(currentI, currentJ) /*|| isSquareUncatch(currentI+1, currentJ)*/){
        gladiator1->log("go right");
        targetSquare = nearestSquare->eastSquare;
    }else if(isWestSquareValid(currentI, currentJ) /*|| isSquareUncatch(currentI-1, currentJ)*/){
        gladiator1->log("go left");
        targetSquare = nearestSquare->westSquare;
    }else if(isSouthSquareValid(currentI, currentJ) /*|| isSquareUncatch(currentI, currentJ-1)*/){
        gladiator1->log("go down");
        targetSquare = nearestSquare->southSquare;
    }
    return 0;
}

void loop() {
    if(gladiator1->game->isStarted()) { //tester si un match à déjà commencer
        //code de votre stratégie
        gladiator1->log("Hello world - Game Started"); // GFA 4.5.1
        int nextMove;
        switch(currentState) {
            case(INIT):
                init();
                currentState=THINKING;
                break;
            case(THINKING):
                nextMove = thinking();
                switch (nextMove)
                {
                case 0:
                    currentState = MOVING;
                    break;
                default:
                    break;
                }
                break;
            case(MOVING):
                gladiatorNextTile();
                const MazeSquare* nearestSquare = gladiator1->maze->getNearestSquare();
                if (compareSquare(*nearestSquare, *targetSquare)) currentState=THINKING;
            break;
        }
    }else {
        gladiator1->log("Hello world - Game not Startd yet"); // GFA 4.5.1
    }
}