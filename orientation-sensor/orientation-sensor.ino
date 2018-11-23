#include <Wire.h>

#define MPU6050_ADDR          0x68
#define MPU6050_WHOAMI        0x75
#define MPU6050_ACCEL_CFG     0x1C
#define MPU6050_ACCEL_XOUT_H  0x3B
#define MPU6050_ACCEL_XOUT_L  0x3C

#define SWAP_HI_LO(x) (((x & 0xFF00) >> 8) | ((x & 0x00FF) << 8))  

typedef union {
  struct { int16_t x, y, z; };
  int16_t v[3];
} acc_t;


typedef enum {
  X_POS = 0,
  X_NEG,
  Y_POS,
  Y_NEG,
  Z_POS,
  Z_NEG,
} axis_t;

const char* AXIS_NAMES[] = {
  "x+",
  "y+",
  "z+",
  "x-",
  "y-",
  "z-",
};

int mpu6050_read_reg(uint8_t addr, uint8_t* val, size_t len)
{
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(addr);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, len);
  for (size_t i = 0; i < len; ++i)
  {
    val[i] = Wire.read();
  }
  Wire.endTransmission(true);

  return 0;
}


int mpu6050_write_reg(uint8_t addr, uint8_t* val, size_t len)
{
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(addr);
  for (size_t i = 0; i < len; ++i)
  {
    Wire.write(val[i]);
  }
  Wire.endTransmission(true);

  return 0;
}

int mpu6050_read_acc(acc_t* acc)
{
  int res = mpu6050_read_reg(MPU6050_ACCEL_XOUT_H, (uint8_t*)acc, sizeof(acc_t));
  acc->x = SWAP_HI_LO(acc->x);
  acc->y = SWAP_HI_LO(acc->y);
  acc->z = SWAP_HI_LO(acc->z);
  return res;
}


int mpu6050_init()
{
  uint8_t who = 0;
  if (mpu6050_read_reg(MPU6050_WHOAMI, &who, sizeof(who)))
  {
    Serial.println("error:reading from 6050");
    return 0;
  }

  uint8_t wake = 0;
  mpu6050_write_reg(0x6B, &wake, sizeof(wake));

  Serial.print("who:"); Serial.println(who);
  
  return who == MPU6050_ADDR;
}


void setup()
{
  pinMode(13, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  mpu6050_init();
}


axis_t major_axis(acc_t* acc)
{
    int biggest_idx = 0;
    int biggest_mag = abs(acc->x);
    
    int axis = 0;
    for (int i = 3; i--;)
    {
      int mag = abs(acc->v[i]);
      
      if (mag > biggest_mag)
      {
        biggest_mag = mag;
        biggest_idx = i;
      }
    }

    if (acc->v[biggest_idx] < 0)
    {
      biggest_idx += 3;
    }

    return (axis_t)biggest_idx;
}


void loop()
{
  static acc_t last_acc = {};
  acc_t acc = {};
  
  mpu6050_read_acc(&acc);

  axis_t last_major = major_axis(&last_acc);
  axis_t now_major  = major_axis(&acc);

  if (now_major != last_major)
  {
    Serial.print("ori:"); Serial.println(AXIS_NAMES[now_major]);
    last_acc = acc;
  }
  else
  {
    last_acc.x = (acc.x + last_acc.x * 9) / 10;
    last_acc.y = (acc.y + last_acc.y * 9) / 10;
    last_acc.z = (acc.z + last_acc.z * 9) / 10;  
  }

  delay(100);
}
