# Integrating the Current Sensing Signal Path (Rev. B)

- Source PDF: `../application_notes/integrating_current_sensing.pdf`
- Extraction tool: pdfplumber
- Page count: 3
- SHA256: `b927be84dcbdba01624d44999b4bb5b53134d70590fccdc659742720061e927f`

## Page 1

Application Brief
Integrating the Current Sensing Signal Path
Scott Hill Current Sensing Products
Current measurements are used in electronic systems To optimize the current sensing signal chain, the
to provide feedback verifying operation is within shunt resistor value and amplifier gain must be
acceptable margins and to detect potential fault appropriately selected for the current range and full-
conditions. Analyzing a system’s current level can scale input range of the ADC. The selection of the
diagnose unintended or unexpected operating modes shunt resistor is based on a compromise between
allowing for adjustments to be made to improve measurement accuracy and power dissipation across
reliability or to protect the system components from the shunt resistor. A large value resistor will develop
damage. a larger differential voltage as the current passes
through. The measurement errors will be smaller due
Current is a signal that is difficult to measure directly.
to the fixed amplifier offset voltage. However, the
However, there are several measurement methods
larger signal creates a larger power dissipation across
that are capable of measuring the effect of flowing
the shunt resistor (P = I2R). A smaller shunt resistor
current. Current passing through a wire produces
develops a smaller drop across the shunt resistor
a magnetic field that can be detected by magnetic
reducing the power dissipation requirements but also
sensors (hall-effect and fluxgate for example). Current
increases the measurement errors as the amplifier’s
measurements can also be made by measuring the
fixed offset errors become a larger percentage of the
voltage developed across a resistor as current passes
signal.
through. This type of resistor is called a current
sensing, or shunt, resistor. The amplifier gain is selected to ensure that the
amplifier’s output signal will not exceed the ADCs full-
For current ranges reaching up to 100 amps on
scale input range at the full-scale input current level.
voltage rails below 100 volts, measuring current with
shunt resistors are typically preferred. The shunt The INA210 is a dedicated current sense amplifier
resistor approach commonly provides a physically that integrates the external gain setting resistors as
smaller, more accurate and temperature stable shown in Figure 2. Bringing these gain resistors
measurement compared to a magnetic solution. internal to the device allows for increased matching
and temperature drift stability compared to typical
For the system’s current information to be evaluated
external gain setting resistors. Space saving QFN
and analyzed, it must be digitized and sent to
packages significantly reduce the board space
the system controller. There are many methods
requirements of an operational amplifier and external
for measuring and converting the signal developed
gain resistors. Current sense amplifiers are commonly
across the shunt resistor. The most common
available in multiple fixed gain levels to better
approach involves using an analog front-end to
optimize the pairing with shunt resistor values based
convert the current sensing resistor’s differential
on the input current and ADC full-scale input ranges.
signal to a single-ended signal. This single-ended
signal is then connected to an analog to V CM = V S =
0V to 26V 2.7V to 26V
digital converter (ADC) that is connected to a
microcontroller. Figure 1 illustrates the current sensing
signal chain.
V
OUT SUPPLY
POWER
SUPPLY
+ LOAD
R SHUNT ADC CONTROLLER
-
LOAD REF
REF
Figure 1. Current Sensing Signal Path
R
TNUHS
www.ti.com
INA210
+
-
Figure 2. INA210: Current Sensing Amplifier
SBOA167B – DECEMBER 2016 – REVISED SEPTEMBER 2021 Integrating the Current Sensing Signal Path 1
Submit Document Feedback
Copyright © 2021 Texas Instruments Incorporated

## Page 2

Figure 1 shows the operational amplifier measuring
the differential voltage developed across the shunt
resistor and sending the amplified signal to the
single ended ADC. A fully differential input ADC can
monitor the differential voltage directly across the
shunt resistor. One drawback to using a typical ADC
is reduced input range used. The signal developed
LOAD
across a shunt resistor will be small to limit the power
dissipation requirements of this component. Lower
ADC resolutions will also impact the small signal
measurement accuracy.
The ADC reference will also be an additional error
source that must be evaluated in this signal path. A
typical ADC will feature an input range that is based
on the converter's reference voltage. The actual
reference voltage range varies from device to device
but is typically in the 2 V to 5 V range. The LSB
(least significant bit) is based on the full-scale range
and resolution of the converter. For example, a 16-bit
converter with a full-scale input range of 2.5 V, the
LSB value is roughly 38 μV.
The INA226 is a specialized ADC designed
specifically for bi-directional current sensing
applications. Unlike typical ADCs, this 16-bit converter
features a full-scale input range of +/- 80 mV
eliminating the need to amplify the input signal
to maximize the ADC's full-scale input range. The
INA226 is able to accurately measure small shunt
voltages based on the device's maximum input offset
voltage of 10 μV and an LSB size of 2.5 μV. The
INA226 provides 15 times more resolution than the
equivalent standard 16-bit ADC with a full-scale input
range of 2.5 V. The specialization of the INA226
makes this device ideal for directly monitoring the
voltage drop across the current sensing resistor as
shown in Figure 3.
R
TNUHS
Alert GPIO
I2C
Interface
INA226
RELlORTNOC
www.ti.com
Supply
2.7V to 5.5V
VCM =
0V to 36V
16-Bit SDA
ADC SCL
Figure 3. Digital Current/Power Monitor
In addition to the ability to directly measure voltage
developed across the shunt resistor as current passes
through, the INA226 can also measure the common-
mode voltage. The INA226 has an input multiplexer
allowing the ADC input circuitry to switch between
the differential shunt voltage measurement and the
single-ended bus voltage measurement.
The current sensing resistor value present in the
system can be programmed into a configuration
register on the INA226. Based on this current
sensing resistor value and the measured shunt
voltage, on-chip calculations convert of the shunt
voltage back to current and can provide a direct
readout of the corresponding power level of
the system. Performing these calculations on-chip
reduces processor resources that would normally be
required to convert this information.
Alternate Device Recommendations
For applications with lower performance
requirements, using the INA199 still takes advantage
of the benefits of the dedicated current sense
amplifier. For applications implementing over-current
detection, the INA301 features an integrated
comparator to allow for on-chip over-current detection
as fast as 1μs. For applications with lower
performance requirements, using the INA219 is able
to take advantage of the specialized current sensing
ADC.
Table 1. Alternative Device Recommendations
Device Optimized Parameter Performance Trade-Off
INA199 Lower Cost Higher V and Gain Error
OS
INA301 Signal Bandwidth, On-Board Comparator Larger Package: MSOP-8
INA219 Smaller Package Digital Monitor, Lower Cost Higher V and Gain Error
OS
INA190 More Accurate N/A
Table 2. Related TI Application Briefs
SBOA162 Measuring Current To Detect Out-of-Range Conditions
SBOA165 Precision Current Measurement On High Voltage Power Rail
SBOA160 High Precision, Low-Drift In-Line Motor Current Measurements
SBOA161 Low-Drift, Low-Side Current Measurements for Three-Phase Systems
2 Integrating the Current Sensing Signal Path SBOA167B – DECEMBER 2016 – REVISED SEPTEMBER 2021
Submit Document Feedback
Copyright © 2021 Texas Instruments Incorporated

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
Copyright © 2022, Texas Instruments Incorporated
