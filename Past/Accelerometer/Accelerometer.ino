#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"
MPU6050 mpu(0x68);
MPU6050 mpu1(0x69); // <-- use for AD0 high
#define OUTPUT_READABLE_YAWPITCHROLL
#define INTERRUPT_PIN 2  // use pin 2 on Arduino Uno & most boards
bool blinkState = false;
bool dmpReady = false;  // set true if DMP init was successful
bool dmpReady1=false;
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t mpuIntStatus1;
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint8_t devStatus1;
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t packetSize1;
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint16_t fifoCount1;
uint8_t fifoBuffer[64]; // FIFO storage buffer
uint8_t fifoBuffer1[64];

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
Quaternion q1;
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
float ypr1[3];
volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
volatile bool mpuInterrupt1=false;
void dmpDataReady() {
    mpuInterrupt = true;
}
void dmpDataReady1() {
    mpuInterrupt1=true;
}

void setup() {
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
        Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif
    Serial.begin(115200);
    while (!Serial); // wait for Leonardo enumeration, others continue immediately
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();
    mpu1.initialize();
    pinMode(INTERRUPT_PIN, INPUT);
    pinMode(3,INPUT);
    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
    devStatus = mpu.dmpInitialize();
    devStatus1=mpu1.dmpInitialize();
    

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip
    mpu1.setXGyroOffset(220);
    mpu1.setYGyroOffset(76);
    mpu1.setZGyroOffset(-85);
    mpu1.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);
        mpu1.setDMPEnabled(true);

        // enable Arduino interrupt detection
        Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        attachInterrupt(digitalPinToInterrupt(3), dmpDataReady1, RISING);
        mpuIntStatus = mpu.getIntStatus();
        mpuIntStatus1 = mpu1.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady = true;
        dmpReady1=true;
        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
        packetSize1=mpu1.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
}
void loop() {
    if (!dmpReady) return;
    while (!mpuInterrupt && fifoCount < packetSize) {
    }
    mpuInterrupt = false;
    mpuInterrupt1=false;
    mpuIntStatus = mpu.getIntStatus();
    mpuIntStatus1=mpu.getIntStatus();
    fifoCount = mpu.getFIFOCount();
    fifoCount1=mpu1.getFIFOCount();
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        // reset so we can continue cleanly
        mpu.resetFIFO();
        Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
        // read a packet from FIFO
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        fifoCount -= packetSize;
        
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        
        Serial.print("ypr\t");
        Serial.print(ypr[0] * 180/M_PI);
        Serial.print("\t");
        Serial.print(ypr[1] * 180/M_PI);
        Serial.print("\t");
        Serial.print(ypr[2] * 180/M_PI);
    }
    
    
    if ((mpuIntStatus1 & 0x10) || fifoCount1 == 1024) {
        // reset so we can continue cleanly
        mpu1.resetFIFO();
        Serial.println(F("FIFO overflow!"));
    }
     else if (mpuIntStatus1 & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (fifoCount1 < packetSize1) fifoCount1 = mpu1.getFIFOCount();
        // read a packet from FIFO
        mpu1.getFIFOBytes(fifoBuffer1, packetSize1);
        fifoCount1 -= packetSize1;
        
        mpu1.dmpGetQuaternion(&q1, fifoBuffer1);
        mpu1.dmpGetGravity(&gravity, &q1);
        mpu1.dmpGetYawPitchRoll(ypr1, &q1, &gravity);
        
        
    }
    Serial.print("ypr\t");
    Serial.print(ypr1[0] * 180/M_PI);
    Serial.print("\t");
    Serial.print(ypr1[1] * 180/M_PI);
    Serial.print("\t");
    Serial.println(ypr1[2] * 180/M_PI);
    // otherwise, check for DMP data ready interrupt (this should happen frequently)
}
