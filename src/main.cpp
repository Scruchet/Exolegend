#define MAX_NEIGHBORS 3
#define KP 1
#define KI 5
#define KD 0
#define NB_SQUARE_BEFORE_BREAK 2
#define PERIOD 20000
#define ORIGINAL_MAP_SIZE 12
#define MAX_ACCELARATION 0.05
#define MAX_SPEED 0.6
#define VERY_MAX_SPEED 1
#define MIN_SPEED 0.3
#define TURNING_GAIN 0.4
#define TIME_MARGIN 1000
#define pifou_1 15;
#define pifou_2 21;
#define POSSESS_WEIGHT 10
#define VISITED_WEIGHT 10
#define ROCKET_WEIGHT 10
#define DANGER_WEIGHT -1000
#define MAX_ENNEMIES 2
#define MAX_GLADIATOR 4

#include "gladiator.h"
#include <Arduino.h>
enum state{
    INIT,
    PROCESSING,
    MOVING,
    HOMING //Go to the center of the map
};

enum direction{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

//-------------------------------------------------------
//-------------------VARIABLES GLOBALES------------------

Gladiator* gladiator;
state current_state = INIT;
byte PIFOU_ID;
MazeSquare* targetSquare;
const MazeSquare* currentSquare;
double speed;
int visitedSquares[12][12];
int mapSize;
unsigned long lastTimeMeasure;
int tick;
int list_id_ennemies[MAX_ENNEMIES];
double ennemie1_position[2];
double ennemie2_position[2];
int homingFlag;
//-------------------------------------------------------
//-------------------Fonctions---------------------------

double getDiffAngle(double x1, double y1, double x2, double y2){
    double diffX = x2 - x1;
    double diffY = y2 - y1;
    return atan2(diffY, diffX);
}

void getEnemiesID(){
    int index = 0;
    RobotList robotList = gladiator->game->getPlayingRobotsId();
    for(int i=0; i<MAX_GLADIATOR; i++){
        unsigned char id = robotList.ids[i];
        RobotData data = gladiator->game->getOtherRobotData(id);
        if (data.teamId != PIFOU_ID){
            list_id_ennemies[index] = data.teamId;
            index ++;
        }
    }
}

void getEnnemiesPosition(double ennemies[MAX_ENNEMIES]){
    RobotData ennemie1 = gladiator->game->getOtherRobotData(list_id_ennemies[0]);
    RobotData ennemie2 = gladiator->game->getOtherRobotData(list_id_ennemies[1]);
    ennemie1_position[1] = ennemie1.position.x;
    ennemie1_position[2] = ennemie1.position.y;
    ennemie2_position[1] = ennemie2.position.x;
    ennemie2_position[2] = ennemie2.position.y;
}

void distance_from_ennemies(){
    double distance_from_ennemie_1 = 0;
    double distance_from_ennemie_2 = 0;
    distance_from_ennemie_1 = sqrt(pow(abs(ennemie1_position[1]-currentSquare->i),2)+pow(abs(ennemie1_position[1]-currentSquare->j),2));
    distance_from_ennemie_2 = sqrt(pow(abs(ennemie2_position[1]-currentSquare->i),2)+pow(abs(ennemie2_position[1]-currentSquare->j),2));
}

void reset();
void setup() {
    //instanciation de l'objet gladiator
    gladiator = new Gladiator();
    //enregistrement de la fonction de reset qui s'éxecute à chaque fois avant qu'une partie commence
    gladiator->game->onReset(&reset); // GFA 4.4.1
    speed = 0;
}

int costDistance(int iActual, int jActual, int iTarget, int jTarget){
    int diffx = abs(iTarget - iActual); 
    int diffy = abs(jTarget - jActual); 
    if (diffx < diffy){
        return (diffx * 1.4 + diffy - diffx)*10;
    }else{
        return (diffy *1.4 + diffx - diffy)*10;
    }
}

int numberOfOutput(MazeSquare* s1){
    int score=0;
    if(s1->northSquare != nullptr) score ++;
    if(s1->southSquare != nullptr) score ++;
    if(s1->westSquare != nullptr) score ++;
    if(s1->eastSquare != nullptr) score ++;
    return score;

}


MazeSquare* processBest(MazeSquare* s1, MazeSquare* s2){
    // gladiator->log("Je cherche les bonus");
    int score1 = 0; 
    int score2 = 0; 
    if(s1->possession == 0){
        score1+= POSSESS_WEIGHT; 
    }else if(s1->possession != PIFOU_ID){
        score1+= POSSESS_WEIGHT*2;
    }
    else{
        score1 -= POSSESS_WEIGHT;
    }
    if(s2->possession == 0){
        score2+= POSSESS_WEIGHT; 
    }else if(s2->possession != PIFOU_ID){
        score2+= POSSESS_WEIGHT*2;
    }
    else{
        score2 -= POSSESS_WEIGHT;
    }
    if(s1->coin.value){
        score1+= ROCKET_WEIGHT; 
    }
    if(s2->coin.value){
        score2+= ROCKET_WEIGHT;
    }
    if(s1->danger){
        score1+= DANGER_WEIGHT; 
    }
    if(s2->danger){
        score2+= DANGER_WEIGHT;
    }
    score1 += numberOfOutput(s1);
    score1 += numberOfOutput(s2);

    // score1 += visitedSquares[s1->i][s1->j] * VISITED_WEIGHT;
    // score2 += visitedSquares[s1->i][s1->j] * VISITED_WEIGHT;

    // int bonus1 = (100 - costDistance(s1->i, s1->j, 5.5, 5.5))/10;
    // int bonus2 = (100 - costDistance(s2->i, s2->j, 5.5, 5.5))/10;
    // //gladiator->log("bonus1 et 2 %d %d", bonus1, bonus2);

    // score1 += bonus1;
    // score2 += bonus2;

    if (score1 > score2) return s1;
    return s2;
}

MazeSquare* best_choice(MazeSquare* squares[MAX_NEIGHBORS][MAX_NEIGHBORS]){
    MazeSquare* best = nullptr;
    for(int i=0; i< MAX_NEIGHBORS; i++){
        for(int j=0; j<MAX_NEIGHBORS; j++){
            if(squares[i][j] != nullptr){
                if (best == nullptr){
                    // gladiator->log("Je suis dans le if");
                    best = squares[i][j];
                }else{
                    // gladiator->log("Je suis dans le else");
                    best = processBest(best, squares[i][j]);
                }
            }
        }
    }
    return best;
}


void reset() {
    //fonction de reset:
    //initialisation de toutes vos variables avant le début d'un match
    //gladiator->log("Call of reset function"); // GFA 4.5.1
}

void initialisation(){
    tick = PERIOD - 1000;
    lastTimeMeasure = millis();
    currentSquare = gladiator->maze->getNearestSquare();
    gladiator->log("Initialisation");
    RobotData data = gladiator->robot->getData();
    PIFOU_ID = data.teamId;
    gladiator->control->setWheelPidCoefs(WheelAxis::RIGHT, KP , KI , KD);
    gladiator->control->setWheelPidCoefs(WheelAxis::LEFT , KP , KI , KD);
    mapSize = 12;
    for (int i=0; i<ORIGINAL_MAP_SIZE; i++){
        for (int j=0; j<ORIGINAL_MAP_SIZE; j++){
            visitedSquares[i][j] = 5;
        }
    }
    homingFlag = 0;
}

void checkPossibility(MazeSquare* possibilities[MAX_NEIGHBORS][MAX_NEIGHBORS]){
    if(currentSquare->northSquare != nullptr){
        possibilities[1][2] = currentSquare->northSquare;
    }else{
        possibilities[1][2] = nullptr;
    }
    if(currentSquare->eastSquare != nullptr){
        possibilities[2][1] = currentSquare->eastSquare;
    }else{
        possibilities[2][1] = nullptr;
    }
    if(currentSquare->southSquare != nullptr){
        possibilities[1][0] = currentSquare->southSquare;
    }else{
        possibilities[1][0] = nullptr;
    }
    if(currentSquare->westSquare != nullptr){
        possibilities[0][1] = currentSquare->westSquare;
    }else{
        possibilities[0][1] = nullptr;
    }
    possibilities[1][1] = nullptr;
    possibilities[0][0] = nullptr;
    possibilities[2][2] = nullptr;
    possibilities[0][2] = nullptr;
    possibilities[2][0] = nullptr;
}

double reductionAngle(double x)
{
    x = fmod(x + PI, 2 * PI);
    if (x < 0)
        x += 2 * PI;
    return x - PI;
}

direction getDirection(double targetAngle){
    if (targetAngle > M_PI_4 && targetAngle < (3 * M_PI_4)){
        return UP;
    }
    else if (targetAngle > -M_PI_4 && targetAngle < M_PI_4){
        return RIGHT;
    }
    else if (targetAngle > -(3*M_PI_4) && targetAngle < -M_PI_4){
        return DOWN;
    }
    else {
        return LEFT;
    }
}
int nbr_square_before_wall(direction direc){
    // currentSquare = gladiator->maze->getNearestSquare();
    int cmpt = 0;
    switch (direc)
    {
    case UP:
        while(currentSquare->northSquare != nullptr){
            currentSquare = currentSquare->northSquare;
            cmpt++;
        }
        return cmpt;
        break;
    case DOWN:
        while(currentSquare->southSquare != nullptr){
            currentSquare = currentSquare->southSquare;
            cmpt++;
        }
        return cmpt;
        break;
    case LEFT:
        while(currentSquare->westSquare != nullptr){
            currentSquare = currentSquare->westSquare;
            cmpt++;
        }
        return cmpt;
        break;
    case RIGHT:
        while(currentSquare->eastSquare != nullptr){
            currentSquare = currentSquare->eastSquare;
            cmpt++;
        }
        return cmpt;
        break;
    default:
        break;
    }
    return 0;
}

void gladiatorGoTo(double targetX , double targetY, bool max_speed){
    RobotData data = gladiator->robot->getData();
    Position position = data.position;
    double currentX = position.x;
    double currentY = position.y;
    double currentAngle = position.a;
    
    double targetAngle = getDiffAngle(currentX, currentY, targetX, targetY);
    double diffAngle = reductionAngle(targetAngle-currentAngle);
    double tolerance = 0.2;
    if (abs(diffAngle) > tolerance){//Tourner vers la position
        speed = 0;
        gladiator->control->setWheelSpeed(WheelAxis::RIGHT, TURNING_GAIN*diffAngle);
        gladiator->control->setWheelSpeed(WheelAxis::LEFT, -TURNING_GAIN*diffAngle);
    
    }
    else if(!max_speed){  // Avancer/Freiner
        if (nbr_square_before_wall(getDirection(targetAngle)) > NB_SQUARE_BEFORE_BREAK){
            speed += MAX_ACCELARATION;
            if(max_speed){
                speed = VERY_MAX_SPEED;
            }
            else if (speed >= MAX_SPEED){
                speed = MAX_SPEED;
            }
        }
        else{
            speed -= MAX_ACCELARATION;
            if (speed <= MIN_SPEED){
                speed = MIN_SPEED;
            }
        }
        gladiator->control->setWheelSpeed(WheelAxis::RIGHT,speed-diffAngle);
        gladiator->control->setWheelSpeed(WheelAxis::LEFT, speed-diffAngle);
    }
    else if (max_speed){
        gladiator->log("Je dois aller very max speed");
        gladiator->control->setWheelSpeed(WheelAxis::RIGHT,VERY_MAX_SPEED);
        gladiator->control->setWheelSpeed(WheelAxis::LEFT, VERY_MAX_SPEED);
    }
}



void update_map(){
    int elapsedTime = (millis() - lastTimeMeasure);
    lastTimeMeasure = millis();
    tick -= elapsedTime;
    if(tick <= TIME_MARGIN){
        tick = PERIOD + TIME_MARGIN;
        mapSize -= 2;
        gladiator->log("Map se diminue");
    }
}

int is_outside(float offset){
    float squareSize = gladiator->maze->getSquareSize();
    float infBorder = 1.5-((mapSize/2) * squareSize) + offset;
    float supBorder = 3 - infBorder - offset;
    RobotData data = gladiator->robot->getData();
    Position position = data.position;
    return ((position.x < infBorder) || (position.y < infBorder) || (position.x > supBorder) || (position.y > supBorder));
}

state processing(){
    currentSquare = gladiator->maze->getNearestSquare();
    MazeSquare* possibilities[MAX_NEIGHBORS][MAX_NEIGHBORS];
    checkPossibility(possibilities);
    targetSquare = best_choice(possibilities); //Renvoie le meilleur choix entre les différentes possibilités.
    return MOVING;
}

state moving(){
    const MazeSquare* currentSquare = gladiator->maze->getNearestSquare();
    if (targetSquare->i == currentSquare->i && targetSquare->j == currentSquare->j ){
        visitedSquares[currentSquare->i][currentSquare->j] --;
        if (visitedSquares[currentSquare->i][currentSquare->j] < 0) visitedSquares[currentSquare->i][currentSquare->j] = 0;
        return PROCESSING;
    }

    float squareSize = gladiator->maze->getSquareSize();
    Position centerTarget;
    centerTarget.x = (targetSquare->i + 0.5) * squareSize;
    centerTarget.y = (targetSquare->j + 0.5) * squareSize;
    gladiatorGoTo(centerTarget.x, centerTarget.y, false);

    return MOVING;
}

state homing(){
    if (!is_outside(0.075)){
        return PROCESSING;
    }
    else {
        gladiatorGoTo(1.5,1.5, true);
        return HOMING;
    }
}


void game(){ // Machine à états
    if(currentSquare != nullptr){
        update_map();
        if (is_outside(0)){
            current_state = HOMING;
        }
    }
    switch (current_state)
    {
    case INIT:
        // gladiator->log("Case INIT");
        initialisation();
        current_state = PROCESSING;
        break;
    case PROCESSING:
        //gladiator->log("Case PROCESSING");
        current_state = processing();
        break;
    case MOVING:
        //gladiator->log("Case MOVING");
        current_state = moving();
        break;
    case HOMING:
        gladiator->log("Case HOMING");
        current_state = homing();
        targetSquare->i = currentSquare->i;
        targetSquare->j = currentSquare->j;
        break;
    default:
        break;
    }
}


void loop() {
    if(gladiator->game->isStarted()) { //tester si un match à déjà commencer
        game();
    }else {
        // gladiator->log("Game not Start yett"); // GFA 4.5.1
    }
    delay(5);
}
