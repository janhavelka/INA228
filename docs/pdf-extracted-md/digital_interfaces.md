# Digital Interfaces for Current Sensing Devices (Rev. A)

- Source PDF: `../application_notes/digital_interfaces.pdf`
- Extraction tool: pdfplumber
- Page count: 3
- SHA256: `58713e7279d1559599c3c76ea3e28e520e6bd036f4debd9eb1daf1e97f1fec5b`

## Page 1

www.ti.com
Application Brief
Digital Interfaces for Current Sensing Devices
Dennis Hudgins, Mitch Morse Current Sensing Products
Devices that monitor and report system current can some of the key differences among I2C, SMBus, and
either provide an analog output that is proportional PMBUS devices.
to the sensed current, or communicate the current
I2C, SMBus, and PMBus devices can easily co-exist
to a host processor digitally. Use of a digital current
on the same bus since all logic low thresholds are at
sense amplifier is sometimes preferred because
0.4 V. Differences in the logic high thresholds usually
the integrated analog to digital conversion allows
are not an issue since the open-drain clock and data
data to be sent directly to the host controller or
lines will go up to VDD when not held low. SMBus
processor. Interaction with a host processor requires
expanded on the frame work laid by I2C by adding a
a digital interface that can both allow sending
bus timeout requirement that prevents a device from
and receiving of instructions as well as data. For
holding data lines low for extended amounts of time.
current sensing applications, the most commonly
SMBus also clearly defined many different types of
used digital interfaces are I2C, SMBus, PMBus,
transaction protocols that support the transmission of
and SPI. Each interface has different strengths
data from the bit level to blocks of bytes. The SMBus
and weaknesses; selecting the correct interface
and PMBus specifications are very similar because
for a given application can allow better system
PMBus leverages the electrical characteristics
optimization, faster system response time, and a
and communications protocols as defined by the
reduction in software development time.
SMBus specification. PMBus incorporates the SMBus
I2C, SMBus, and PMBus all utilize open drain clock electrical specification, while also standardizing the
and data lines that require pull-up resistors to an address locations for common commands that are
external power supply. SMBus and PMBus compatible used in power systems. The address/command
devices feature an active low alert output to notify the standardization allows one software driver to support
host processor of fault conditions. I2C, SMBus, and many devices without the need to be completely
PMBus devices commonly exist on the same physical rewritten to support new devices or devices from
bus; however, differences exist. Table 1 highlights different manufacturers.
Table 1. Comparison of I2C, SMBus, and PMBus Interfaces
Parameter I2C SMBus PMBus
0.4 V, sinking 350 μA (Low
0.4 V, sinking 3
Output Logic Low, V Power) sinking 4 mA (High Same as SMBus
OL mA
Power)
Electrical Levels
Input Logic Low, V 0.3 x Vdd 0.8 V Same as SMBus
IL
Input Logic High, V 0.7 x Vdd 2.1 V Same as SMBus
IH
Minimum - 100 kHz Same as SMBus
Speed
Maximum 5 MHz 400 kHz, 1 MHz Same as SMBus
3-4: SDC, SCL, SMBALERT,
Number of wires/pins 2: SDA, SCL 3: SDA, SCL, SMBALERT
CONTROL(optional)
Time-out requirement No Yes Yes
Specified Transaction Protocols No Yes Yes
Alert Capability No Yes Yes
Address Resolution Protocol No Yes, but optional Yes, but optional
CRC error checking support No Yes, but optional Yes, but optional
Group Protocol support No Yes, but optional Yes, required for PMBus
Yes, both standard and device
Standardized Commands / Register Set No No
specific commands supported
SBOA203A – JUNE 2017 – REVISED SEPTEMBER 2021 Digital Interfaces for Current Sensing Devices 1
Submit Document Feedback
Copyright © 2021 Texas Instruments Incorporated

## Page 2

www.ti.com
SMBus adds support for dynamic address Table 2. I2C/SMBus Compatible Devices
resolution, CRC checking to increase communications (continued)
robustness, and group protocol that allows Device Optimized Parameters Performance Trade-Off
communication to multiple devices within a single
Internal Shunt, high 36-V, Large TSSOP-16
transaction. Support for group protocol is optional for INA260 accuracy (total solution), package, Maximum
SMBus devices but is required for all PMBus devices. 16-bit ADC current is 15A
3 channel voltage and 26-V, 12-bit ADC, Lower
Most digital current sense amplifiers available from INA3221
current monitor effective sample rate
Texas Instruments are compatible with both I2C and
SMBus interfaces. For example, the INA228 supports
Texas Instruments also has the INA233, which
the high-speed mode (up to 2.94 MHz) offered by
I2C, but also features an Alert pin and error resolution features a PMBus compatible interface and can
monitor current, voltage, power, and energy. The
protocols as defined by SMBus. The device can
INA233 is also available in a VSSOP-10 package, can
monitor and report the shunt voltage, bus voltage,
monitor current very accurately with a gain error of
die temperature, current, power, energy accumulation,
only 0.1%, and has a maximum offset error of 10 μV.
and charge accumulation. The INA228 is available
Table 3 provides a summary of current sense devices
in a VSSOP-10 package, is an ultra-precise digital
that are compatible with I2C, SMBus, and PMBus.
current monitor with a 20-bit delta-sigma ADC, has
a gain error of only 0.05%, and a maximum offset Table 3. I2C/SMBus/PMBus Compatible Devices
voltage of 1 μV. Table 2 provides a summary of
Device Optimized Parameters Performance Trade-Off
current sense devices that are compatible with I2C
High accuracy, 16-bit
and SMBus.
ADC, Fast Sampling 36-V, VSSOP-10
INA233
Rate, Energy Monitor / package
Table 2. I2C/SMBus Compatible Devices
Power Accumulator
Device Optimized Parameters Performance Trade-Off
Internal analog
26-V, 12-bit ADC, Large Another digital interface that is used in current
INA209 comparators for critical
TSSOP-16 package monitoring devices is the SPI interface. The SPI
over current detection
interface is a 4-wire interface that does not require
Low pin count, SOT23-8 No alert pin, 26-V, 12-bit
INA219 external pull-up resistors like I2C and can operate at
package ADC, VBUS tied to IN-
much higher clock frequencies. The pull-up resistors
Independent bus voltage No alert pin, 26-V, 12-bit
INA220 measurement ADC,VSSOP-10 package used in I2C limit the operational speed due to the
RC time constant established by the value of the
High accuracy, 16-bit
INA226 ADC, Current / Power 36-V, VSSOP-10 pull-up resistor and bus capacitance. One example
package
Monitor is the INA229, which has the same current sensing
Highset Accuracy, 85-V, specifications as the INA228, but allows data clock
20-Bit ADC, Power / rates as high as 10 MHz. Table 4 provides a summary
INA228 VSSOP-10 package
Energy / Charge / of current sense devices that are compatible with SPI.
Temperature Monitor
28-V, Higher offset and Table 4. SPI Compatible Devices
High accuracy, 16-bit
INA230 ADC, 3mm x 3mm QFN gain error than similar Device Optimized Parameters Performance Trade-Off
device INA226
Highset Accuracy, 85-V,
INA231 A H D i C g , h S A m cc a u lle ra s c t y P , a 1 c 6 k -b a i g t e 2 g 8 a - i V n , e H r i r g o h r e th r a o n ff s s e im t a ila n r d INA229 2 E 0- n B e it r g A y D / C C , h P a o rg w e e r / / VSSOP-10 package
(WCSP-12), 1.8-V I2C device INA226 Temperature Monitor
interface
High Accuracy, 85-V, VSSOP-10 package,
28-V, 12-bit ADC, One 16-Bit ADC, Power / higher offset and gain
INA234 Small DSBGA 8 package address pin for a INA239 Energy / Charge / error than similar INA229
maximum of 4 addresses Temperature Monitor device
High accuracy, 48-V, 16-
One address pin for a
INA236 bit ADC, small DSBGA 8 Table 5. Adjacent Application Briefs
maximum of 4 addresses
package
SBOA511 Getting Started with Digital Power Monitors
85-V, 16-Bit ADC, VSSOP-10 package,
SBOA167 Integrating the Current Sensing Signal Path
Power / Energy / Higher offset and gain
INA237
Charge / Temperature error than similar INA238 Integrated, Current Sensing Analog-to-
SBOA179
Monitor device Digital Converter
High Accuracy, 85-V, VSSOP-10 package, Power and Energy Monitoring with Digital
SBOA194
16-Bit ADC, Power / Higher offset and gain Current Sensors
INA238
Energy / Charge / error than similar INA228
Temperature Monitor device
2 Digital Interfaces for Current Sensing Devices SBOA203A – JUNE 2017 – REVISED SEPTEMBER 2021
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
