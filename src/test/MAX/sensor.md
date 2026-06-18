ZTS-3002-TR-*-N01
Five-pin soil multi-parameter sensor
Product Manual
Translated from Chinese (Simplified) to English - www.onlinedoctranslator.com
1 Overview
The five-pin soil multi-parameter sensor has stable performance, high sensitivity, fast response, stable output, and is suitable for all kinds of
soil. It is an important tool for observing and studying the occurrence, evolution, improvement, and water-salt dynamics of saline soil. By measuring
the dielectric constant of the soil, it can directly and stably reflect the true moisture content of various soils. It can measure the volume percentage of
soil moisture, which is a soil moisture measurement method that meets the current international standards. It can be buried in the soil for a long time,
is resistant to long-term electrolysis, corrosion, vacuum potting, and is completely waterproof.
2 Features
（1）This sensor is designed with compact size.
（2) High measurement accuracy, fast response speed and good interchangeability.
（3）It has good sealing performance and can be directly buried in the soil for use without being
corroded.4) It has little impact on soil quality and is widely used in a wide area.
（5) Accurate measurement, reliable performance, ensuring normal operation and high data transmission efficiency.
3 Scope of application
It is suitable for temperature, humidity, conductivity and pH value testing in soil moisture monitoring, scientific experiments, water-saving
irrigation, greenhouses, flowers and vegetables, grasslands, rapid soil testing, plant cultivation, sewage treatment, precision agriculture and other
occasions.
4 Product Information
4.1Technical Parameters
Measurement parameters: Soil conductivity (ECvalue), temperature, moisture,PHValue, nitrogen, phosphorus and potassium (measured by national
standard instruments and input)
Measuring range: 0~20000μS/cm, -40~80℃, 0-100%,3~9PH, 0-2999 mg/kg (mg/L) Measurement
accuracy:0-10000us/cmThe range is ±3%FS;10000-20000us/cmThe range is ± 5%FS, @(brown
soil, 60%RH, 25℃), ±0.5℃,0-50%Internal2%,@(brown soil,30%,25℃） 50-100%Internal3%,@(brown
soil,60%,25℃）、±0.3PH、typical accuracy≤5%(Subject to the actual measuring instrument)
Resolution: 1μS/cm, 0.1℃, 0.1%, 0.1, 1 mg/kg (mg/L)
No.1Page
Output signal:RS485（ModBus-RTUProtocol)
Supply voltage:4.5~30V DC Scope of work: -30℃〜
70℃ Stabilization time:≤5min
Response time: <1Second
Note: The performance data stated above was obtained under the test conditions using our test system and software. In order to continuously improve our products, our company reserves the right to change the design functions and specifications.
rights without prior notice.
4.2Physical parameters
Pin length:68mm,φ3mm Pin
Material:316LStainless steel
Seal Material:ABSEngineering plastics, epoxy resin, waterproof gradeIP68 Cable specification:
Standard2Meters (other cable lengths can be customized, up to1200m) Load capacity: Voltage
output: Output resistance ≤250Ω; Current output: ≤600Ω
4.3Product Selection
ZTS- Company Code
3002-
TR- Soil testing enclosure
THNPKPH- Temperature, moisture, nitrogen, phosphorus, potassiumPH
ECTHNPKPH- Conductivity Temperature Moisture Nitrogen Phosphorus PotassiumPH
THPH- Temperature and moisturePH
ECTHPH- Conductivity Temperature MoisturePH
N01 RS485（Modbus-RTUprotocol)
5 Form Factor
No.2Page
6 How to use
The device can be connected to various data acquisition devices with differential input, data acquisition cards, remote data acquisition modules and
other devices. The wiring instructions are as follows:
7 Data conversion methods
RS485Signal (default address01):
standardModbus-RTUProtocol, baud rate:4800; Check digit: None; Data digit:8; Stop bits:1
7.1Modify address
For example: Change the address to1The sensor address is changed to2, host→Slave
Initial mail
Memory High
Initial mail
Memory Low
Starting address
high
Starting address
Low
CRC16
Low
CRC16
high Original address Function code
0X01 0X06 0X07 0XD0 0X00 0X02 0X08 0X86
If the sensor receives the data correctly, the data is returned along the original route.
Note: If you forget the original address of the sensor, you can use the broadcast address0XFFInstead, use0XFFThe host can only connect
to one slave, and the returned address is still the original address, which can be used as a method of address query.
7.2Query data
No.3Page
Register Address
Register Address PLCOr configure the address content operate Definition
0000 H 40001 (Decimal) Moisture content Read-only Real-time value of moisture content (expanded10times)
0001 H 40002 (Decimal) Temperature value Read-only Temperature real-time value (expanded10times)
0002 H 40003 (Decimal) Conductivity Read-only Conductivity real-time value
0003 H 40004 (Decimal) PHvalue Read-only PHReal-time value (expanded tenfold)
0004H 40005(Decimal) Nitrogen content temporary value Read and Write The nitrogen content value or test value to be written1
0005H 40006(Decimal) Phosphorus content temporary value Read and Write Phosphorus content value or test value to be written2
0006H 40007(Decimal) Potassium content temporary value Read and Write Potassium content value or test value to be written3
0007 H 40008(Decimal) salinity Read-only Salinity real-time value (for reference only)
Total dissolved solids
TDS 0008 H 40009 (Decimal) Read-only TDSReal-time value (for reference only)
0-100correspond0.0%-10.0%
default0.0%
0022 H 40035 (Decimal) Temperature coefficient of conductivity Read and Write
0-100correspond0.00-1.00
default55（0.55）
0023 H 40036 (Decimal) Salinity coefficient Read and Write
0-100correspond0.00-1.00
default50（0.5）
0024 H 40037 (Decimal) TDScoefficient Read and Write
0050 H 40081 (Decimal) Temperature calibration value Read and Write Integer (expanded10times)
0051 H 40082 (Decimal) Moisture content calibration value Read and Write Integer (expanded10times)
0052 H 40083 (Decimal) Conductivity calibration value Read and Write Integer
0053 H 40083 (Decimal) PHCalibration value Read and Write Integer
Nitrogen content temporary value
Coefficient high sixteen digits
04E8H 41001 (Decimal) Read and Write
Floating point numbers
Nitrogen content temporary value （IEEE754Standard floating point type)
Coefficient lower sixteen digits
04E9H 41002 (Decimal) Read and Write
Nitrogen content temporary value
Deviation value
04EA 41003 (Decimal) Read and Write Integer
Phosphorus content temporary value
Coefficient high sixteen digits
04F2H 41011 (Decimal) Read and Write
Floating point numbers
Phosphorus content temporary value （IEEE754Standard floating point type)
Coefficient lower sixteen digits
04F3H 41012 (Decimal) Read and Write
Phosphorus content temporary value
Deviation value
04F4H 41013 (Decimal) Read and Write Integer
Potassium content temporary value
Coefficient high sixteen digits
04FCH 41021 (Decimal) Read and Write
Floating point numbers
Potassium content temporary value （IEEE754Standard floating point type)
Coefficient lower sixteen digits
04FD H 41022 (Decimal) Read and Write
04FEH 41023 (Decimal) Potassium content temporary value Read and Write Integer
No.4Page
Deviation value
07D0 H 42001 (Decimal) Device Address Read and Write 1~254(factory default1）
0represent2400
1represent4800
2represent9600
07D1 H 42002 (Decimal) Device baud rate Read and Write
1:0004HWhen the register is not written, the value in the register isf1(Conductivity measurement value),0004HAfter a write operation is performed on a register, the register stores the written value. 2:
0005HWhen the register is not written, the value in the register isf2(Conductivity measurement value),0005HAfter a write operation is performed on a register, the register stores the written value. 3:
0006HWhen the register is not written, the value in the register isf3(Conductivity measurement value),0006HAfter a write operation is performed on a register, the register stores the written value.
Query conductivity, temperature and moisturePHValue sensor (address is1) data, host→Slave
Initial mail
Memory Location
Address height
Initial mail
Memory Location
Low address
register
Length High
register
Low length
CRC16
Low
CRC16
high address Function code
0X01 0X03 0X00 0X00 0X00 0X04 0X44 0X09
If the sensor receives correctly, it returns the following data, slave → host
address
code
Function
code
Return valid
Number of bytes
Check code
Low Byte
Check code
High Byte
Moisture value Temperature value Conductivity value pH value
0x00
0x38 0x01 0x03 0x08 0x02 0x92 0xFF 0x9B 0x03 0xE8 0x57 0xB6
Temperature calculation:
When the temperature is below 0℃, the temperature data is uploaded in the form of
complement. Temperature: FF9B H (hexadecimal) = -101 => Temperature = -10.1℃ Moisture
calculation:
Water content: 292 H (hexadecimal) = 658 => humidity = 65.8%, that is, the volumetric water content of the soil is 65.8%.
Conductivity calculation:
Conductivity: 3E8 H (hexadecimal) = 1000 Conductivity = 1000 us/cm Calculation
of pH value:
PH value: 38H (hexadecimal) = 56 => PH value = 5.6
8 Precautions for use
police tell
Failure to follow the wiring sequence may cause damage to the device and the instruments connected to it. If
the input power exceeds the maximum input power of the device, it will cause damage to the device.
Note meaning
No.5Page
Please read this instruction manual completely before use.
Do not attempt to insert the probe into stones or hard soil to avoid damaging the probe. When
removing the sensor from the soil, do not pull directly on the cable.
The sensor probe should be fully inserted into the soil/matrix to reduce operational errors and improve measurement accuracy. Calibration should
be performed before each measurement. For long-term use, it is recommended to calibrate once a month. The calibration frequency should be
adjusted according to different application conditions (soil quality, moisture content, salt content, pH value, etc.).
9 Product Warranty
The warranty period of this product is one year. Within twelve months from the date of delivery, if the fault is caused by sensor quality problems (not
man-made damage), our company will be responsible for free repair or replacement. After the warranty period, only the cost will be charged.
No.6Page