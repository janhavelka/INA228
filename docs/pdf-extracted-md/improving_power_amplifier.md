# Improving Power Amplifier Efficiency With Current Monitors (Rev. B)

- Source PDF: `../application_notes/improving_power_amplifier.pdf`
- Extraction tool: pdfplumber
- Page count: 3
- SHA256: `ae4e0e971bc966c86d24f5a770740d0a8488a4d3f261f86da58ccb1323434da7`

## Page 1

www.ti.com
Application Brief
Improving Power Amplifier Efficiency With
Current Monitors
Kyle R. Stone
This application brief addresses a few of the current VDD
Feedback
shunt monitor (CSM) implementations used with Stage
power amplifier stages in wireless infrastructure. With
each particular implementation, this note presents the
trade-offs as well as recommends a device relevant
for the approach. DDAACC DDAACC
DAC DAC
PA Feedback
PA PA
Delving past the abstraction of antennas, transmitters, Stage Stage
and receivers of an active antenna system (AAS)
or remote radio unit (RRU) can yield systems of
amplifiers, microprocessors, multiplexors, DACs, and
Figure 1. PA Stages with Feedback
sensors. Of these elements, one of the crucial
elements impacting power consumption is the power
amplifier stage transmitting out to the users on the Factory Calibration
network. This power amplifier stage can consist of a
A single factory calibration with a generated look-
Doherty amplifier with a predriver, which can further
table is one way to control a PA stage. During
be reduced down to some LDMOS or GaN transistors
calibration, a PA is subjected to a range of bias
configured into class B, AB, and C amplifiers.
voltages or temperatures, while an on or off board
Controlling the bias point of the transistors in a CSM (as Figure 2 shows), monitors the load. From
PA stage improves the efficiency of the system. An the load measurements, a bias value or group of
open-loop, fixed-control voltage for the bias neglects bias values can be programmed into a look-up table
the impact of supply variations on the drain side, that a DAC uses later to control the bias. This
device aging, and transconductance fluctuations due process is then extended to each individual PA in
to temperature changes from heat dissipation and the system. Consequently, the number of CSMs
ambient temperature swings. Consequently, feedback depends on how much calibration time in production
facilitating dynamic control to adjust the PA transistors can be afforded and BOM cost. After production, an
bias points is required, as Figure 1 shows. Depending off-board or shorted sensor saves power otherwise
on the designer’s constraints in board space, cost, lost across the shunt. Additionally, if temperature
precision, number of antennas, and so forth, the sensors are included in the feedback with the look-up
best method for the dynamic control can vary. Many table, regions of PA stages can be adjusted relatively
approaches include a current shunt monitor (CSM) to quickly according to the temperature drift, and a
serve as part of the feedback chain to adjust the bias result, improves efficiency. As the factory calibration
and thereby improve efficiency. This note presents emphasizes cost for a relatively high common-mode,
three general approaches, with each subsequent high-side measurement, consider devices such as the
implementation having greater control on the bias INA290 or INA186.
point and related efficiency of the PA stage.
SPACER
SPACER
SPACER
SPACER
SBOA369B – DECEMBER 2019 – REVISED JULY 2023 Improving Power Amplifier Efficiency With 1
Submit Document Feedback Current Monitors
Copyright © 2023 Texas Instruments Incorporated

## Page 2

VDD
DDAACC + t
INA2290 or INA4181 can be considered to save board
space. For further integration, board savings, and
added system protection, devices such as INA381
and INA310A with integrated comparators can be
C s u h r u re n n t t DAC PPAAP S SAtta aggeess S T e e n m so p r used for overcurrent conditions.
Stages Monitor
Microcontroller
Figure 2. Current Sense Feedback
Periodic Calibration
Although the factory calibration approach improves
efficiency over a fixed-bias voltage, the approach
neglects supply variation, which can immediately
affect the bias and device aging, which gradually
affects the bias. To overcome these shortcomings, the
system can be designed to periodically power off and
recalibrate. From periodic calibration, adjustments are
made to the look-up table as the PAs age, the
system switches between batteries and solar power,
or when the supply begins to droop. With this tighter
control is the option of current monitoring and voltage
monitoring all integrated into one package, such as in
the INA226 or INA228 devices. For more integration,
EZShuntTM technology integrates the shunt resistor
directly into the leadframe, which generally reduces
the design size and procurement effort. Devices
such as the INA700 or INA780A are equipped with
the EZShuntTM technology with a 2-mΩ or 400-μΩ
leadframe resistance respectively. Periodic calibration
requires the shunt resistor to be permanently fixed
on the board, resulting in constant losses across
the shunt. To minimize the losses, a part with low
input offset such as the INA190 or INA290 can be
used, as the lower offset permits a smaller shunt
for improved accuracy. Alternatively, a current sensor
such as the TMCS1123 with an isolated hall sensor
allows for a high common-mode with minimal losses
across the 0.67-mΩ leadframe. Aside from the shunt
dissipation, the other concern for periodic calibration
is the compromise between added programming and
monitor count that is required to provide minimal
transmission interruption.
Real-Time Sensing
While the periodic calibration does address known
issues with aging and temperature that a designer can
anticipate, the calibration does not have immediate
dynamic response for unexpected conditions.
Continuous current monitoring is sometimes required
for critical applications in which tighter control is
required. This method allows for quick adjustments to
any PA stage for any possible changes in operating
conditions. As continuous monitoring requires CSMs
for all PA stages, a multichannel device such as the
+ t
www.ti.com
VDD
Bus
Monitor PA
Load
Current
Monitor REF
OC
MUX Protection
DDAACC
DAC
Microcontroller
Figure 3. Bus Voltage, Current, and OC Feedback
Table 1. CSM Method Comparison
Integrate
BOM Integrate Shunt
Approach Control Bus Count OC Loss
Monitor
Factory Low Loose No No Never
Calibration
Periodic
Medium Tighter Yes No Always
Calibration
Real-time
High Tightest Yes Yes Always
Sensing
Table 2. Alternative Device Recommendations
Device Characteristics
Common mode range –4 V to 110 V;
INA310A
comparator.
Common mode range –0.3 V to 36 V; 16-bit
INA226
delta sigma ADC.
Common mode range 2.7 V to 120 V, 1.1
INA290
MHz, small (SC-70) package.
Table 3. Related TI Application Briefs
Literature Number Title
Current Sensing Applications in
SBOA366 Communication Infrastructure Equipment
Hybrid Battery Charger With Load Control
SLPA013
for Telecom Equipment
2 Improving Power Amplifier Efficiency With SBOA369B – DECEMBER 2019 – REVISED JULY 2023
Current Monitors Submit Document Feedback
Copyright © 2023 Texas Instruments Incorporated

## Page 3

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
Copyright © 2023, Texas Instruments Incorporated
