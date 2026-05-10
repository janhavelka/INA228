# current sense amplifiers shunt resistor layout

- Source PDF: `../application_notes/current-sense-amplifiers-shunt-resistor-layout.pdf`
- Extraction tool: pdfplumber
- Page count: 22
- SHA256: `82f0c1b08e5e4d5ee94b28b8778fe03e01272576a20abab684442b6791656a69`

## Page 1

Shunt Resistor
Layout Considerations
TI Precision Labs – Current Sense Amplifiers
Presented by Rajani Manchukonda
Prepared by Ian Williams and Jason Bridgmon
1

## Page 2

Hello and welcome to the TI precision labs series on current sense amplifiers. My
name is Rajani Manchukonda, and I’m a product marketing engineer for current
sensing products. In this video, we will discuss the guidelines for printed circuit board,
or PCB, layout of the shunt resistor, or Rshunt, in current sense amplifier circuits.
2

## Page 3

R layout rules of thumb
shunt
1. Be close to the current sense amplifier
2. Use Kelvin connections
3. Follow the resistor maker’s guidelines
a) Footprint
b) Placement
c) Landing pad design
2

## Page 4

The layout of a shunt resistor with respect to a current sense amplifier should follow
these three rules of thumb:
1. Be close to the current shunt monitor
2. Use Kelvin connections
3. Follow the resistor maker ’s recommendations as applicable with regards to
footprints, placement, and landing pad design.
Let’s get into some details on each rule.
4

## Page 5

Rule 1 – Be close to the current sense amplifier
Avoid long traces!
3

## Page 6

Placing the shunt resistor close to the amplifier is important as it avoids complications
resulting from long traces. The voltage on these traces is small and the current is low,
so electromagnetic interference and noise is a concern. Also, the parasitic trace
capacitance and resistance, while small, can add to the error of the system, especially
is very low shunt resistances are used.
6

## Page 7

Rule 1 – Be close to the current sense amplifier
Avoid unbalanced traces!
4

## Page 8

Using traces of mismatched lengths is also not recommended, for the same reasons
as not using long traces. While the case shown here is extreme, it is important to note
that mismatches in the trace resistance from the shunt resistor to the amplifier can
create offsets due to the input bias currents present on the lines. These errors may be
small, but it’s best to avoid the issue altogether.
8

## Page 9

Rule 2 – Use Kelvin connections (good layout)
+ -
V
sense
High-current path
5

## Page 10

Here is an example of a good layout. Note that the shunt resistor is close to the
amplifier.
1. The high current path goes from left to right, through the resistor and large load-
carrying traces.
2. There are Kelvin connections from the resistor pads to the amplifier which provide
the differential voltage to be measured. Trace lengths are very short.
The current flowing through these traces is minimal, usually around tens of microamps,
so the voltage loss on the path from the resistor to the monitor is very low.
In summary, these techniques force the bulk of the current through the shunt resistor,
and the value of the resistance seen by the current path will be close to the absolute
value of the resistor. This enables the sense voltage Vsense to be accurately
developed across Rshunt and passed to the current sense amplifier.
10

## Page 11

Rule 2 – Use Kelvin connections (bad layout)
+ -
V = 80mV
shunt
+
V =
trace
1.93mV
-
High-current path
6

## Page 12

Here is an example of a bad layout. Note that the shunt resistor is still close to the
amplifier.
1. Again, the high current path goes from left to right, through the resistor.
2. There is a Kelvin connection from the left side of the resistor to the amplifier, but the
other pad is simply tied into the high current path.
The current flowing through the left trace is minimal, usually around tens of microamps,
but the high-current path through the right trace can carry tens of amps! This means
that the trace resistance will create a much larger error voltage than in the previous
case, when Kelvin connections were used on both inputs.
A one-ounce copper trace of 0.25mm in length and 0.6mm wide has an impedance of
approximately 193μΩ. If this layout were to be used with an 8 milliohm shunt and 10
Amps flowing through the circuit, the 10 amps would create an 80mV drop across the
resistor, and an additional 1.93mV drop across the trace, adding almost two and a half
percent error to the measurement!
12

## Page 13

Rule 3 – Follow the resistor maker’s guidelines
High-current path
Kelvin-connected
sense leads
Follow landing pad
guidelines
7

## Page 14

Specialty resistors often have manufacturer recommendations. In this case, the
recommendations may be that there are large pads for the high current path and small
pads for the Kelvin connected sense traces.
The bulk of the current flows through the trimmed part of the resistor and the kelvin
connections are free to be measured with little interference from the load.
Landing pads guidelines are often found in the data sheets for these resistors, so be
sure to follow them if you use a specialty resistor.
14

## Page 15

To find more current sense amplifier
technical resources and search
products, visit ti.com/currentsense
8

## Page 16

That concludes this video - thank you for watching! Please try the quiz to check your
understanding of the content.
For more information and videos on current sense amplifiers please visit
ti.com/currentsense.
16

## Page 17

Shunt Resistor Layout
Considerations
TI Precision Labs – Current Sense Amplifiers
Quiz
1

## Page 18

Shunt resistor layout considerations – quiz
1. Which is not a shunt resistor layout rule of thumb[unreadable glyph]
a) Follow resistor maker’s guidelines
b) Be close to the current sense amplifier
c) Use Kelvin connections
d) Use unbalanced traces
2. When Kelvin connections are used for R layout, high current _________:
shunt
a) Flows through the PCB traces into the amplifier pins
b) Creates a large error voltage due to PCB trace impedance
c) Flows mainly through R and the large load current traces
shunt
d) Flows into the amplifier power supply pin
2

## Page 19

Shunt resistor layout considerations – quiz
3. Large error voltages due to bad layout can develop when ____________:
a) Kelvin connections are used
b) R is placed close to the current sense amplifier
shunt
c) A current sense amplifier input pin is in the high-current path
d) The amplifier has a low input offset voltage
4. Specialty resistors often have specific recommendations for PCB layout.
a) True
b) False
3

## Page 20

Answers
4

## Page 21

Shunt resistor layout considerations – quiz
1. Which is not a shunt resistor layout rule of thumb[unreadable glyph]
a) Follow resistor maker’s guidelines
b) Be close to the current sense amplifier
c) Use Kelvin connections
d) Use unbalanced traces
2. When Kelvin connections are used for R layout, high current _________:
shunt
a) Flows through the PCB traces into the amplifier pins
b) Creates a large error voltage due to PCB trace impedance
c) Flows mainly through R and the large load current traces
shunt
d) Flows into the amplifier power supply pin
5

## Page 22

Shunt resistor layout considerations – quiz
3. Large error voltages due to bad layout can develop when ____________:
a) Kelvin connections are used
b) R is placed close to the current sense amplifier
shunt
c) A current sense amplifier input pin is in the high-current path
d) The amplifier has a low input offset voltage
4. Specialty resistors often have specific recommendations for PCB layout.
a) True
b) False
6
