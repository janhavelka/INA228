# Integrated, Current Sensing Analog-to-Digital Converter (Rev. A)

- Source PDF: `../application_notes/integrated_current_sensing_adc.pdf`
- Extraction tool: pdfplumber
- Page count: 4
- SHA256: `cf81b8d50be8dd508befec1045d90062fb0e35793cc463dd4697c6936719657d`

## Page 1

www.ti.com
Application Brief
Integrated, Current Sensing Analog-to-Digital Converter
Scott Hill Current Sensing Products
The signal chain path for measuring current is configurations are common for this functional
typically consistent from system to system. Whether requirement. Dedicated current sense amplifiers,
current is measured in a computer, automobile or INA210 for example, feature integrated gain setting
motor, the common functional blocks used are found components and are designed specifically for this
in nearly all equipment. type of application. The INA210 has the capability to
accurately measure very small signals reducing the
The interface to a real world element such as light,
power dissipation requirement of the sensing resistor.
temperature or current in this case, requires a sensor
to convert the signal to a proportional value (voltage The next signal chain block is the ADC which is
or current) that can be more easily measured. There present to digitize the amplified sensor signal. This
are a several sensors that use magnetic field sensing device can require additional external components
for detecting the effects of current flow. These can (reference, oscillator) for more precise measurement
be very effective for detecting very large currents or capability. Similarly to the AFE, there are various
when isolated measurements are required. The most options available for the ADC block. Stand-alone
common sensor for measuring current is a current converters with the onboard references and oscillators
sensing, or shunt, resistor. Placing this component in are available as well as processors featuring onboard
series with the current being measured develops a ADC channels.
proportional differential voltage as the current passes
Both integrated and discrete ADC blocks have their
through the resistor.
benefits as well as limitations. Fewer components
The remaining blocks in the signal path are selected on the board is one obvious advantage with the
based on how this measured current information is ADC being integrated into the processor. Existing
to be used by the system. There are several blocks instruction sets for the onboard ADC channels further
that are common and found in most applications as reduces the requirement for additional software
shown in Figure 1. These blocks consist of an analog to be written to support a stand-alone ADC.
front end (AFE) to amplify a small signal from the However, silicon process nodes for digital controllers
sensor, an analog-to-digital converter (ADC) to digitize frequently are less optimized for precision analog,
the amplified signal from the sensor, and a processor limiting the performance capability of the onboard
to allow for the sensor information to be analyzed so converter. Discrete analog-to-digital converters have
the system can respond accordingly to the measured an advantage of allowing device selection based on
current level. optimized performance attributes such as resolution,
noise or conversion speed.
Power
Supply A variation in this signal chain is to use an ADC
AFE to measure directly across the current sensing
+ resistor eliminating the current sense amplifier
R SHUNT ADC Controller entirely. A standard converter would have challenges
-
in replacing the AFE and measuring the shunt
voltage directly. One challenge is the large full-scale
Load
OSC REF range of the ADC. Without the amplification of the
sense resistor's voltage drop, either the full-range
Figure 1. Current Sensing Signal Chain of the ADC cannot be fully utilized or a larger
voltage drop will be needed across the resistor.
One requirement for the AFE is allow for direct A large voltage drop will result in a larger power
interface to the differential signal developed across dissipation across the sensing resistor. There are
the sense resistor. A single-ended output for ADCs available with modified input ranges designed
the AFE simplifies the interface to the following for measuring smaller signals directly which can
ADC. Operational amplifiers in differential amplifier allow for direct measurement of shunt voltages.
SBOA179A – DECEMBER 2016 – REVISED SEPTEMBER 2021 Integrated, Current Sensing Analog-to-Digital Converter 1
Submit Document Feedback
Copyright © 2021 Texas Instruments Incorporated

## Page 2

www.ti.com
An internal programmable gain amplifier (PGA) is While the INA226 is able to accurately measure small
typically integrated in these devices to leverage the shunt voltages this component has been designed
full-scale range of the ADC. with additional functionality useful for current sensing
applications. This device features an internal register
A limitation these small signal converters have is their
that is user-programmable with the specific value
limited common-mode input voltage range. These
of the current sensing resistor that is present on
ADCs have input voltage ranges that are limited
the PCB. Having the value of the current sensing
by their supply voltage which typically range from
resistor allows the INA226 to directly convert the
3 V to 5.5 V based on the core processor voltage
shunt voltage measured every conversion to the
being supported. The INA226, shown in Figure 2, is
corresponding current value and stores this to an
a current sensing specific analog-to-digital converter
additional output register. The INA226 also features
that solves this common-mode limitation. This device
an internal multiplexer allowing the device to switch
features a 16-bit delta-sigma core and can monitor
from a differential input measurement to a single
small differential shunt voltages on common-mode
ended voltage configuration to allow for measurement
voltage rails as high as 36 V while being powered off
of the common-mode voltage directly. The voltage
a supply voltage that can range from 2.7 V to 5.5 V.
measurement, along with the previously measured
Power Supply shunt voltage and corresponding current calculation,
(0V to 36V)
allows the device the capability of computing power.
INA226
The device stores this power calculation and provides
Bus
Voltage Shunt this value along with the shunt voltage, current and
16-Bit Bus I2C common-mode voltage information to the processor
Current
RSHUNT
V
S
o
h
lt
u
a
n
g
t
e
ADC Power Interface over the two-wire serial bus.
Alert
In addition to the on-chip calculations of current and
Load power, the INA226 features a programmable alert
register that allows the device to compare each
conversion value to a defined limit to determine
Figure 2. INA226, Precision Current, Voltage,
if an out-of-range condition has occurred. This
Power Sensing ADC
alert monitor can be configured to measure out-of-
Similar to the ADCs with modified small input range range conditions such as overcurrent, overvoltage, or
capability, the INA226 has a full-scale input range of overpower. The device also includes programmable
about 80 mV enabling the device to measure directly signal averaging to further improve measurement
across the current sensing resistor. The INA226 has accuracy.
the ability to very accurately resolve small current
The INA226 is optimized to support precision current
variations with an LSB step size of 2.5 μV and
measurements. The additional features included in
a maximum input offset voltage of 10 μV. A 0.1-
this device provide the capability of supporting the
μV/°C offset drift ensures the measurement accuracy
signal management and monitoring needed in this
remains high with only an additional 12.5 μV of offset
current measurement function reducing the burden on
induced at temperatures as high as 125°C. A 0.1%
the system processor.
maximum gain error also enables the measurement
accuracy to remain high at the full-scale signal levels
as well.
2 Integrated, Current Sensing Analog-to-Digital Converter SBOA179A – DECEMBER 2016 – REVISED SEPTEMBER 2021
Submit Document Feedback
Copyright © 2021 Texas Instruments Incorporated

## Page 3

www.ti.com
Alternate Device Recommendations Table 1. Alternative Device Recommendations
Device Optimized Parameters Performance Trade-Off
For applications with lower performance
requirements, using the INA234 still leverages INA234 Lower Cost Higher V OS & Gain Error,
Lower V Range
the benefits of the dedicated current sensing CM
analog-to-digital converter. For additional precision INA260 Lower System Level Gain Larger Package:
Error & Offset TSSOP-16
measurement capability where currents being
measured are less than 15 A, the INA260 provides AMC1305 Isolated Measurement, Higher Cost, Higher V OS
Higher Signal Bandwidth & Gain Error
similar functionality to the INA226 while also
INA210 Lower Cost Higher V & Gain Error
featuring a precision 2-mΩ integrated current sensing OS
resistor inside the package. For applications requiring
Table 2. Adjacent Tech Notes
significantly higher common-mode voltage capability,
Measuring Current To Detect Out-of-
the AMC1305 provides onboard isolation and is SBOA162
Range Conditions
capable of supporting working voltages as high as 1.5
Precision Current Measurement On High
kV DC and handling peak transients as high as 7 kV. SBOA165
Voltage Power Rail
For applications with lower performance requirements
Integrating The Current Sensing Signal
for the AFE, use the INA210 to take advantage of the SBOA167
Path
benefits of a dedicated current sense amplifier.
SBOA170 Integrating the Current Sensing Resistor
SBOA179A – DECEMBER 2016 – REVISED SEPTEMBER 2021 Integrated, Current Sensing Analog-to-Digital Converter 3
Submit Document Feedback
Copyright © 2021 Texas Instruments Incorporated

## Page 4

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
