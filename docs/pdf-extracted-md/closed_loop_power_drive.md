# Closed Loop Constant Power Drive to Simplify Heater Element Control and Extend Battery Life

- Source PDF: `../application_notes/closed_loop_power_drive.pdf`
- Extraction tool: pdfplumber
- Page count: 10
- SHA256: `d52ffb98de414409cd79275d96c27402f1971006cb76191884fdc9bf9b79b7d5`

## Page 1

www.ti.com Table of Contents
Application Note
Closed Loop Constant Power Drive to Simplify Heater
Element Control and Extend Battery Life
Ivan De Padua, Chris Lande, Jim Perkins, and Dorian Brillet de Cande
ABSTRACT
Many applications require accurate temperature control of a heating element. Closed loop control based on
temperature requires measurement of the heater temperature using a thermistor or thermocouple. This can
sometimes be mechanically challenging and costly. Additionally, in battery powered applications, traditional
PWM drive and associated high current pulses, can reduce battery life and lifetime.
Heater temperature is not linear with voltage or current due to changes in resistance with temperature. However,
temperature is close to linear with applied power. By implementing a closed loop constant power drive the
temperature can be controlled needing only to measure power and not temperature directly.
This reference design uses a closed loop constant power topology to drive a low impedance heater element.
This application note includes the choices and challenges within the hardware and software implementation. The
document also shows initial results and discusses advantages of this method of temperature control.
Table of Contents
1 Traditional Heater Control......................................................................................................................................................2
2 Constant Power Heater Control.............................................................................................................................................2
3 Hardware Implementation......................................................................................................................................................3
4 Software Implementation....................................................................................................................................................... 5
5 Software Algorithm Flow Chart............................................................................................................................................. 6
6 Results.....................................................................................................................................................................................7
7 Summary and Adaptations.................................................................................................................................................... 9
8 References.............................................................................................................................................................................. 9
Trademarks
All trademarks are the property of their respective owners.
SLVAFK1 – JANUARY 2025 Closed Loop Constant Power Drive to Simplify Heater Element Control and 1
Submit Document Feedback Extend Battery Life
Copyright © 2025 Texas Instruments Incorporated

## Page 2

Traditional Heater Control www.ti.com
1 Traditional Heater Control
Traditional heater control uses a temperature sensor to measure the temperature of the heating element as
shown in Figure 1-1. This measurement is fed back and used to adjust the drive circuitry to alter the current
through the heating element maintaining the temperature at the required set point. This approach has a number
of challenges. Firstly, the temperature sensor must be mounted close to or in contact with the heating element,
which can be mechanically difficult. Secondly, high temperature measurement usually requires a thermocouple
that needs complex interface circuitry.
Temperature responds relatively slowly compared to changes in the electrical signals, so it has been usual
to use a simple FET switch PWM to modulate the current through the heating element at a higher electrical
frequency and allow the slower thermal response to act as the loop low pass filter. This works perfectly well, but
the fast switching edges can result in electrical noise. In addition, in a battery powered system, the large current
pulses pulled from the source during the PWM pulses, can reduce battery life between charges and overall
battery lifetime.
Vbat
PWM
Control Heater
Temp
Sensor
Figure 1-1. Traditional Heating Element Control Loop With Temperature Sensor
2 Constant Power Heater Control
The temperature of a resistive heating element is directly proportional to the power applied. Measuring the
electrical power can be mechanically much simpler than measuring the temperature. Driving the element with
constant power delivers constant temperature and adjusting the power adjusts the temperature. Unfortunately,
the resistance of the heating element can vary significantly between batches and also changes over
temperature. This means that both voltage and current need to be measured and the voltage applied needs
to be adjusted to maintain constant power as the heater element resistance changes as seen in Figure 2-1. The
DC/DC converter controlling the applied voltage draws an average current from the supply which can extend the
battery life.
2 Closed Loop Constant Power Drive to Simplify Heater Element Control and SLVAFK1 – JANUARY 2025
Extend Battery Life Submit Document Feedback
Copyright © 2025 Texas Instruments Incorporated

## Page 3

www.ti.com Hardware Implementation
Vbat
DC/DC
Power
sensing
Control Heater
Figure 2-1. Constant Power Heater Control
3 Hardware Implementation
The constant power drive design requires hardware to measure the voltage across the heating element and the
current through the heating element to calculate the power. This is achieved using the INA234 which is a 28V,
12bit, I2C output current/voltage/power monitor. In this design the device measures the voltage directly across
the heating element and the current through a high-side 10mΩ, ± 1%, 1W sense resistor. The devices then
calculates the power and reports values for voltage, current and power via I2C.
For this example, we assume a 1Ω heating element that can vary by ± 20% across temperature and batch
range. Table 3-1 shows the required voltage and current for different power levels across the resistance range.
The input voltage is 3.3V to 5.0V. This means a buck or step-down dc/dc regulator can be used for the whole
range required. The applied voltage is controlled using the TPS62868 which is a 2.4V to 5.5V input, synchronous
buck converter with 4A output capability. Importantly, this device is I2C controlled which allows the output voltage
to be easily adjusted.
Table 3-1. Voltage and Current for Different Power Levels and Different Resistances
Power (W) Current (A) at Voltage (V) at Current (A) at Voltage (V) at Current (A) at Voltage (V) at
0.8Ω 0.8Ω 1.0Ω 1.0Ω 1.2Ω 1.2Ω
4.0 2.24 1.79 2.00 2.00 1.83 2.19
5.0 2.50 2.00 2.24 2.24 2.04 2.45
6.0 2.74 2.19 2.45 2.45 2.24 2.68
7.0 2.96 2.37 2.65 2.65 2.42 2.90
8.0 3.16 2.53 2.83 2.83 2.58 3.10
9.0 3.35 2.68 3.00 3.00 2.74 3.29
The voltage, current and power is read from the INA234 via I2C using an MSPM0L1306. This low cost
microprocessor is also responsible for adjusting the output voltage of the TPS62868 via I2C. The simplified
and full circuit schematic can be seen respectively in Figure 3-2 and Figure 3-3.
SLVAFK1 – JANUARY 2025 Closed Loop Constant Power Drive to Simplify Heater Element Control and 3
Submit Document Feedback Extend Battery Life
Copyright © 2025 Texas Instruments Incorporated

## Page 4

Rsense
H
GPIO
MSPM0L1306
INA234
I2C
Figure 3-1. Simplified Constant Power Control Schematic
U1
D1 VS S S C D L A D C2 2
L1 GND A B 1 1 I I N N + - AL G E N R A D T 0 A B C1 2 2 220nH INA234AIYBJR C2 C3 GND 6.3V 6.3V 10μF 10μF GND GND
GND
GND
3 2 1
VIN
VS: 1.7 - 5.5VDC
SDA SCL
Vin: 2.4 - 5.5VDC ALERT VIN U2 R2 3 VIN SW 7 10m EN S S C D L A 4 6 5 E S S N D C A L VSE A T P / G G V V N N O ID D D S 9 1 8 2 VSET 1 R 0 5 .0k A resistive load will be attached to this connector
J1 TPS628682ARQY
GND
GND
VDD range is 1.62 - 3.6VDC
U4 VIN
1 IN OUT 5
1 C μ 6 F 3 EN 16V 4 NC GND 2 GND TPS7A2033PDBVR
1 2
SCL SDA J8
3 VIN
2 A0 1 GND GND
J9 GND
J3 A0 3 A0_MSP 2 A0 TP1 TP2 1 GND TP3 TP4
TP5 U3
MCU_VDD 4 VDD
MCU J _ 4 VDD 5 1 2 3 3 7 9 1 GN J F T T D 5 T A A S R R H G G - E E 1 T T 0 _ _ 5 R T -0 X X 2 4 6 8 1 1 D D 0 -F S S -D W W R - D C K S L I T O K G T N P D 6 2 1 C 5 0 7 V u G F ND 1 2 C 0 5 8 0 V nF GND 4 2 C 7 5 9 0 V nF GND 1 R K 0 1 e 0 G 0 e k p N K R D e 9 G e c p N l o R 4 R D s 6 7 e 7 B G R 0 c t l N o 9 u lo e D M se D C G C t 1 N N o U 1 C 4 R D M p 7 8 i 0 C n U p 2 1 C T T i u 5 n A A 1 F V 2 R R T G G P8 E E T T S S A A _ _ W W L 0 R T R E V S S _ E D C X D C X N M S S R L I T L D E A D T O S K T P 1 1 1 1 1 2 2 2 2 2 2 2 3 1 1 3 1 2 2 1 2 3 1 4 6 7 8 9 0 2 4 6 7 8 9 0 0 2 2 2 6 7 8 9 5 3 5 3 3 1 1 1 1 P P P P P P P P P P P P P P P P P P P P P P P P P P P P R V A A A A A A A A A A A A A A A A A A A A A A A A A A A A C S . . . . . . . . . . . . . . . . . . . . . . . . . . . . T O 2 2 5 6 0 1 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 4 7 8 9 R 3 1 / 0 / / / / 0 1 2 3 4 5 6 7 8 9 2 4 5 6 7 / / / / / T T R C U U T T U U E / / / / / / / / / / / / / / / / / / I I V I I A A U U U U U A A A A A O A S A A O O A A A A M M M M e D D D D D D D D W D D A A A A A R R R R P S M G G G G R A R R R R R C C C C C C C C C C T T T T C D P E 0 0 2 2 1 1 1 0 0 T T T T T 0 0 0 0 0 0 0 0 0 0 / 0 I F _ _ _ _ T _ 0 0 _ 1 1 1 _ _ _ _ _ _ _ _ _ _ _ _ _ O _ + C C I C C _ I _ R _ _ _ T T R 5 6 4 1 0 9 8 7 3 2 O / M N / R 0 1 X X C 0 1 C T R / X / / / / X / / / / / I U C V G G C C U O O O O 1 2 G / / X / / / T / T T X / / S S S T S I S O - C e I S O O A S P P S / S P P P P 1 / 2 2 / / P P S P R P P S 1 P A A M A A A A R / _ / / M M U C C C I I I I T I T C P E I _ P M M 1 1 0 0 C T 0 0 P 0 0 A 0 P P L 0 0 0 I I I F I S L _ _ _ _ 1 1 0 _ _ K P P _ _ 0 M M 0 0 _ R _ 0 _ _ - K D _ I O I I / P S _ _ _ C P _ _ _ C _ / S _ T P S G G N N N S _ R I A T I C U I O O I I O P S I 0 S S D C C 0 O 0 P 0 1 N 0 N N N T I C / K O T _ 1 U C 0 C U L A M I _ O + S + - _ 1 S U + 1 0 O / 0 T / / / C I / T K C T / C P - G C / / / + - T U / T U S / _ X / T / I / / / 0 I T I G / U 1 C U / O I 2 O / C P A T A / U I 0 I 2 / T I M U / A I P 2 M S O I _ I A I S M R _ M C R U I P A 2 2 0 M A A R G C M W C R P M 0 G A T 1 T A R C P G C _ M T R 0 G 1 O 0 1 T 1 G _ 0 1 0 R P T 0 C C 1 4 1 T _ _ / P 1 1 _ _ C S _ _ _ 0 _ 0 1 T L _ _ _ S U 1 C S _ _ _ C R O C I C O _ 1 S _ K _ R S C _ 2 A 1 C C I C T T _ 0 I C T L D U / U T N C T 1 / R N L S S T 0 I / R / X 1 T S A X T / T L - 2 / S S S T / / 0 X / / U I / / T / / / / T C C 0 P / P S M S + U U T T S I A I I T O _ I 1 I 2 P P I M I M A G P A / 0 R I 0 C M M _ M I C I T M I R R G _ _ G T 0 0 0 T S 0 I 1 G G P C G 0 C T T _ _ 4 M _ 2 S C _ _ 0 4 1 _ S 0 0 S C C _ 2 _ C / S L G S _ _ _ T _ U _ 1 C S 2 _ C 1 / S C O D C C 1 X R T R / C A 1 / 3 0 3 K T _ 0 0 U U A / I X X / 1 R / S M I / C C A / T T / / T M T S P 0 O G T S R I 0 I I G P M / I P T M 4 M _ 0 U M I 4 I 0 _ T G _ 0 P G 0 A _ G _ C X P _ 0 0 _ 4 R C R 2 0 I P _ _ _ P T C 1 T _ O O C C I 0 O S C C 0 C U _ 0 / 1 / O I R T / O U / / S / X T U P A I P I / A 2 A R I M S 0 C 0 R T P G _ _ 1 T 0 I I 0 C _ 0 0 _ N _ S S _ _ C 0 C R D P 1 T - 0 O / T S A O S / C / U P T / I C A I A M L 1 R K G _ T _ I 4 1 N O _ _ 0 U C T - X T 1 / / U BS A L R _ T i 1 n _ v R o X ke Thermal_ V p S a S d 3 5 3 TP7
MCU_VDD MSPM0L1306TRHB
R6
47.0k
RST
C13
25V 1100pF
GND GND
2 1
C4 C5 6.3V 6.3V 22uF 22uF
D1 Red 2 1
TP11 TP12
TP13 TP14
R1 R3 R4 10.0k 10.0k 10.0k
TP9 TP10
D2 GND GND
1
2
3
4
Hardware Implementation www.ti.com
C1
0.1μF 50V C14 16V 68uF GND
C10 470nF 25V
SW1
Figure 3-2. Constant Power Control Schematic
4 Closed Loop Constant Power Drive to Simplify Heater Element Control and SLVAFK1 – JANUARY 2025
Extend Battery Life Submit Document Feedback
Copyright © 2025 Texas Instruments Incorporated

## Page 5

www.ti.com Software Implementation
Figure 3-3. Constant Power Control PCBA
4 Software Implementation
At start-up, the software running in the MSP0L1306 begins by initializing the I2C read/write function and then
configures the INA234 and TPS62868 via I2C. The MSPM0L1306 is then able to read the voltage, current and
power in the load resistor from the INA234 using I2C and control the output voltage of the TPS62868 also via
I2C.
The constant power control algorithm is described in the flow chart shown in Figure 5-1. The first step is to read
the power INA234_getPOWER_W(INA234) through I2C and store it in the measuredP variable. The measured
power is compared with the target power and a power error calculated.
The voltage change needed to correct the power error is calculated as the error scaled by a gain factor of 2.
There is a limit applied to prevent excessive voltage changes; it is clamped to a range of ±5.
If the measured power is higher than the target power, the algorithm calculates a new voltage by decreasing the
measured output voltage by the calculated voltage step. If the measured power is lower than the target value,
the calculated voltage step is added to the measured output voltage. The output voltage of the TPS62868 is then
adjusted by writing the new voltage value to the output voltage register via I2C.
SLVAFK1 – JANUARY 2025 Closed Loop Constant Power Drive to Simplify Heater Element Control and 5
Submit Document Feedback Extend Battery Life
Copyright © 2025 Texas Instruments Incorporated

## Page 6

Software Algorithm Flow Chart www.ti.com
5 Software Algorithm Flow Chart
Figure 5-1. Software Algorithm Flow Chart
6 Closed Loop Constant Power Drive to Simplify Heater Element Control and Extend Battery Life SLVAFK1 – JANUARY 2025
Submit Document Feedback
Copyright © 2025 Texas Instruments Incorporated

## Page 7

www.ti.com Results
6 Results
Figure 6-1 shows the measured, steady-state temperature of a 1.5Ω nominal load resistor for different values of
applied constant power. The linear response shows that constant power can be used as the control method to
set the temperature in the resistive heating element.
Figure 6-1. Load Resistor Temperature Versus Applied Constant Power
Figure 6-2 shows the measured power for 1W steps from 1W to 9W with a 1.5Ω nominal load resistor. The
loop is held running in constant power at each level for 50 seconds and the power measured every 1 second.
The absolute variation in measured power increases as the requested power level increases but the percentage
variation stays pretty constant as can be seen in Table 6-1 which shows the measured levels and variation for
each power level from nominal. The constant power control loop maintains the power applied to the load within
±3.0% of the set level.
Figure 6-2. Constant Power Control Steps. 1W to 9W in 1W Steps. 50 Seconds at Each Power Level
SLVAFK1 – JANUARY 2025 Closed Loop Constant Power Drive to Simplify Heater Element Control and 7
Submit Document Feedback Extend Battery Life
Copyright © 2025 Texas Instruments Incorporated

## Page 8

Results www.ti.com
Table 6-1. Average Measured Power and Minimum or Maximum Variation for Requested Power Steps
From 1W to 9W
Requested Power Average measured Minimum measured Maximum measured Negative variation Positive variation
(W) power (W) power (W) power (W) (%) (%)
1.0 1.0008 0.971 1.029 -2.90 2.90
2.0 2.0073 1.942 2.059 -2.90 2.95
3.0 2.9913 2.911 3.081 -3.00 2.70
4.0 4.0123 3.898 4.116 -2.55 2.90
5.0 5.0108 4.864 5.145 -2.72 2.90
6.0 6.0146 5.840 6.174 -2.67 2.90
7.0 6.9999 6.796 7.204 -2.91 2.91
8.0 8.0209 7.764 8.235 -2.95 2.94
9.0 8.9982 8.752 9.251 -2.76 2.79
Figure 6-3, Figure 6-4, and Figure 6-5 show more detailed plots of the measured power values for 1W, 5W and
9W constant power operation.
Figure 6-3. Measured Power at 1 Second Intervals Figure 6-4. Measured Power at 1 Second Intervals
for 1W Constant Power Operation for 5W Constant Power Operation
Figure 6-5. Measured Power at 1 Second Intervals for 9W Constant Power Operation
8 Closed Loop Constant Power Drive to Simplify Heater Element Control and SLVAFK1 – JANUARY 2025
Extend Battery Life Submit Document Feedback
Copyright © 2025 Texas Instruments Incorporated

## Page 9

www.ti.com Summary and Adaptations
As an additional test of the robustness of the constant power control, the load was driven at constant 4W across
a temperature range from around -18°C to +23°C. As the temperature increases the resistance of the nominal
1.5Ω load increases from around 1.33Ω at -18°C to 1.75Ω at +23°C. Figure 6-6 shows the measured current,
voltage and power across this temperature range. The constant power control algorithm adjusts the voltage as
the resistance changes to successfully keep the power constant across the temperature range. The variation in
measured power across this temperature range is +1.5% and -2.1%.
Figure 6-6. Measured Current, Voltage and Power Across -18°C to +23°C Temperature Range
7 Summary and Adaptations
A resistive heating element can be driven with constant power to deliver a fixed temperature, without needing
to measure the temperature. The closed loop, constant power example design presented here maintains the
measured power in the load to within ±3.0% of the set level across power levels and variations in load resistance
due to temperature changes. In this case ±3.0% variation in power corresponds to a temperature tolerance of
about ±0.15°C at lower power (1W) and about ±0.75°C at higher power (9W). The temperature tolerance can
vary depending on he heating element used. but can be calibrated during product design.
If the input voltage range, the heater resistance or the power required means higher current or a boost or
buck-boost topology are required, then other I2C controlled converter devices can be used. Alternatively, a
standard converter can be adjusted by summing a filtered PWM signal into the FB node.
8 References
• Texas Instruments, INA234 28-V, 12-Bit, Current, Voltage, and Power Monitor With an I2C Interface, data
sheet.
• Texas Instruments, TPS62868x 2.4-V to 5.5-V Input, 4-A/6-A Synchronous Step-Down Converter with I2C
Interface in QFN Package
• Texas Instruments, MSPM0 L-Series 32MHz Microcontrollers, technical reference manual.
• Texas Instruments, I2C Introduction Lab
• Texas Instruments, MSPM0 Ecosystem Training Series. MSPM0 academy trainings.
SLVAFK1 – JANUARY 2025 Closed Loop Constant Power Drive to Simplify Heater Element Control and 9
Submit Document Feedback Extend Battery Life
Copyright © 2025 Texas Instruments Incorporated

## Page 10

IMPORTANT NOTICE AND DISCLAIMER
TI PROVIDES TECHNICAL AND RELIABILITY DATA (INCLUDING DATA SHEETS), DESIGN RESOURCES (INCLUDING REFERENCE
DESIGNS), APPLICATION OR OTHER DESIGN ADVICE, WEB TOOLS, SAFETY INFORMATION, AND OTHER RESOURCES “AS IS”
AND WITH ALL FAULTS, AND DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED, INCLUDING WITHOUT LIMITATION ANY
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT OF THIRD
PARTY INTELLECTUAL PROPERTY RIGHTS.
These resources are intended for skilled developers designing with TI products. You are solely responsible for (1) selecting the appropriate
TI products for your application, (2) designing, validating and testing your application, and (3) ensuring your application meets applicable
standards, and any other safety, security, regulatory or other requirements.
These resources are subject to change without notice. TI grants you permission to use these resources only for development of an
application that uses the TI products described in the resource. Other reproduction and display of these resources is prohibited. No license
is granted to any other TI intellectual property right or to any third party intellectual property right. TI disclaims responsibility for, and you
will fully indemnify TI and its representatives against, any claims, damages, costs, losses, and liabilities arising out of your use of these
resources.
TI’s products are provided subject to TI’s Terms of Sale or other applicable terms available either on ti.com or provided in conjunction with
such TI products. TI’s provision of these resources does not expand or otherwise alter TI’s applicable warranties or warranty disclaimers for
TI products.
TI objects to and rejects any additional or different terms you may have proposed. IMPORTANT NOTICE
Mailing Address: Texas Instruments, Post Office Box 655303, Dallas, Texas 75265
Copyright © 2025, Texas Instruments Incorporated
