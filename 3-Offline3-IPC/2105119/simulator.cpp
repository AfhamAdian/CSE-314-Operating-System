#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <random>
#include <vector>
#include <fstream>
#include <chrono>
#include <queue>
#include <cstring>
#include "Operative.h" 
#include "Staff.h" 


// Protecting shared data?	Yes	MUTEX
// Need to signal between threads?	Yes	SEMAPHORE
// Counting resources (parking spots, buffers)?	Yes	SEMAPHORE
// Only one thread at a time?	Yes	MUTEX
// Producer-Consumer pattern?	Both	SEMAPHORE (logic) + MUTEX (safety)

#define UNIT_TIME 100000               // 100ms= 100*1000 micro second = 1 time unit
#define STATION_COUNT 4
#define MAX_ARRIVAL_TIME 10*UNIT_TIME 
#define STAFF_NUMBER 2

using namespace std;


ofstream outputFile;

int N, M, x, y; // N=operatives, M=unit size, x=doc recreation time, y=logbook time
int completedOperations = 0;
auto start_time = chrono::high_resolution_clock::now();
int* jobsDoneInUnit;
int rc;
// bool isRunning = true;

pthread_mutex_t outputMutex = PTHREAD_MUTEX_INITIALIZER;

queue<int> stationQueue[STATION_COUNT];
sem_t stationSemaphores[STATION_COUNT];
pthread_mutex_t stationMutexes[STATION_COUNT];
pthread_mutex_t mutex;
sem_t writerSem;

sem_t* unitSem;
pthread_mutex_t* unitMutex;

// 0 1 2  3 4 5  6 7 8 
long long get_current_time() {
    auto current = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(current - start_time);
    return duration.count() / 100; // Convert to time units (100ms = 1 time unit)
}

int get_random_number() {
    // Creates a random device for non-deterministic random number generation
    std::random_device rd;
    // Initializes a random number generator using the random device
    std::mt19937 generator(rd());
  
    // Lambda value for the Poisson distribution
    double lambda = 10000.234;
  
    // Defines a Poisson distribution with the given lambda
    std::poisson_distribution<int> poissonDist(lambda);
  
    // Generates and returns a random number based on the Poisson distribution
    return poissonDist(generator);
  }
  
//TODO : (make it safe)
void thread_safe_print( const string& msg ){
    pthread_mutex_lock(&outputMutex);
    outputFile << msg << endl;
    cout << msg << endl;
    pthread_mutex_unlock(&outputMutex);
}


void initialzie_sems(){
    int units = N / M; 
    unitSem = new sem_t[units];

    for (int i = 0; i < units; i++) {
        sem_init(&unitSem[i], 0, 0);
    }
    for (int i = 0; i < STATION_COUNT; i++) {
        sem_init(&stationSemaphores[i], 0, 1); 
    }
    sem_init(&writerSem, 0, 1);
}

void initialize_mutexes() {
    int units = N / M;
    unitMutex = new pthread_mutex_t[units];

    for (int i = 0; i < units; i++) {
        pthread_mutex_init(&unitMutex[i], nullptr);
    }
    for (int i = 0; i < STATION_COUNT; i++) {
        pthread_mutex_init(&stationMutexes[i], nullptr);
    }
    pthread_mutex_init(&outputMutex, nullptr);
    pthread_mutex_init(&mutex, nullptr);
}

void init(){
    rc = 0;
    int units = N / M;
    initialize_mutexes();
    initialzie_sems();

    jobsDoneInUnit = new int[units];
    for(int i = 0; i < units; i++) {
        jobsDoneInUnit[i] = 0;
    }
}

void writeLog(int unitId) {
    sem_wait(&writerSem); 
    usleep(y * UNIT_TIME); // simulate wriiting log
    completedOperations++;
    string msg = "Unit " + to_string(unitId+1) + " completed intelligence distribution at time " + to_string(get_current_time()) + ". Total completed operations: " + to_string(completedOperations);
    thread_safe_print(msg);
    sem_post(&writerSem);
}

void* operativeJob (void * arg){
    Operative * operative = (Operative *)arg;
    int id = operative->id;
    int stationId = operative->stationId;
    int unitId = operative->unitId;
    int isLeader = operative->isLeader;

    int arrivalTime  = get_random_number() % MAX_ARRIVAL_TIME + 1*UNIT_TIME; 
    usleep(arrivalTime);

    // arrives at station
    thread_safe_print("Operative " + to_string(id) + " arrived at typewriting station at time " + to_string(get_current_time()));

    // gets the job done
    sem_wait(&stationSemaphores[stationId]); 
    usleep(x * UNIT_TIME);

    pthread_mutex_lock(&unitMutex[unitId]);
    jobsDoneInUnit[unitId]++;
    pthread_mutex_unlock(&unitMutex[unitId]);

    thread_safe_print("Operative " + to_string(id) + " has completed document recreation at " + to_string(get_current_time()) );
    sem_post(&stationSemaphores[stationId]);     

    pthread_mutex_lock(&unitMutex[unitId]);
    if(jobsDoneInUnit[unitId] == M) {
        sem_post(&unitSem[unitId]);
    }
    pthread_mutex_unlock(&unitMutex[unitId]);

    if(isLeader) {
        sem_wait(&unitSem[unitId]); 
        thread_safe_print("Unit " + to_string(unitId+1) + " has completed document recreation phase at time " + to_string(get_current_time()));
        writeLog(unitId);
        usleep(y * UNIT_TIME); 
    }

    return NULL;
}

void* staffJob(void * arg) {
    Staff * staff = (Staff *)arg;
    int id = staff->staffId;
    int reviewingInterval = staff->reviewingInterval;

    while(1)
    {
        usleep(reviewingInterval * UNIT_TIME); 
        pthread_mutex_lock(&mutex);
        rc++;
        if(rc == 1){
            // cout << "@@@!!!!!!!!!!!!!!!!!!" << endl;
            // int sem;
            // sem_getvalue(&writerSem, &sem);
            // thread_safe_print("Writer semaphore value before waiting: " + to_string(sem));
            sem_wait(&writerSem);
            // sem_getvalue(&writerSem, &sem);
            // thread_safe_print("Writer semaphore value after waiting: " + to_string(sem));
        }
        pthread_mutex_unlock(&mutex);

        thread_safe_print("Intelligence Staff " + to_string(id) + " began reviewing logbook at time " + to_string(get_current_time())
                        + ". Operations completed = " + to_string(completedOperations));   
        usleep(y*UNIT_TIME);
        thread_safe_print("Intelligence Staff " + to_string(id) + " completed reviewing logbook at time " + to_string(get_current_time())
                        + ". Operations completed = " + to_string(completedOperations));   
                        
        pthread_mutex_lock(&mutex);
        // cout << " HERERERHERHEHREHRH : " << rc << endl;
        rc--;
        if(rc == 0) { 
            sem_post(&writerSem);
            // cout << " //////// " << endl;
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


int main(){
    ifstream inputFile("input.txt");
    if (!inputFile.is_open()) {
        cerr << "Error opening input file!" << endl;
        return 1;
    }
    outputFile.open("output.txt");
    if (!outputFile.is_open()) {
        cerr << "Error opening output file!" << endl;
        return 1;
    }

    // cin >> N >> M >> x >> y;
    inputFile >> N >> M >> x >> y;
    init();

    vector<Operative> operatives;
    for (int i = 0; i < N; i++) {
        int unitId = i / M; 
        int stationId = i % STATION_COUNT; 
        bool isLeader = (i % M == M - 1); 
        operatives.push_back(Operative(i + 1, unitId, stationId, isLeader));
    }

    vector<Staff> staffMembers;
    for (int i = 0; i < STAFF_NUMBER; i++) {
        int staffInterval = get_random_number()%5 + 10; 
        staffMembers.push_back(Staff(i + 1, staffInterval)); 
    }

    // create threads
    pthread_t operativeThreads[N];
    pthread_t staffThreads[STAFF_NUMBER];
    for (int i = 0; i < N; i++) {
        pthread_create(&operativeThreads[i], NULL, operativeJob, (void *)&operatives[i]); 
    }
    for (int i = 0; i < STAFF_NUMBER; i++) {
        pthread_create(&staffThreads[i], NULL, staffJob, (void *)&staffMembers[i]);
        usleep(4*UNIT_TIME);
    }

    // Waiting to finish
    for (int i = 0; i < N; i++) {
        pthread_join(operativeThreads[i], NULL);
    }
    for (int i = 0; i < STAFF_NUMBER; i++) {
        pthread_cancel(staffThreads[i]);
        pthread_join(staffThreads[i], NULL);
    }

    thread_safe_print("All operatives have completed their jobs.");
    thread_safe_print("Total completed operations: " + to_string(completedOperations));

    // clean up
    delete[] jobsDoneInUnit;
    delete[] unitSem;
    delete[] unitMutex;

    outputFile.close();
    return 0;
}