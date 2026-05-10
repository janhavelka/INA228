# INA228 protocol, commands, and transactions

The INA228 is an I2C/SMBus target device. It supports fast-mode I2C from 1 kHz to 400 kHz and high-speed mode from 10 kHz to 2.94 MHz. All data bytes are transmitted MSB first and follow SMBus 3.0 transfer protocol.

Source: INA228 datasheet, pp. 7, 19-21.

## Address byte

The 7-bit address is `100 A3 A2 A1 A0b` (`0x40` to `0x4F`) from `A1` and `A0` strap combinations listed in `02_pinout_and_signals.md`. The R/W bit follows the 7-bit address. The device samples `A1` and `A0` on every bus communication. If SDA is connected to `A0` or `A1`, the datasheet requires an additional 100 ns hold time on the MSB of the I2C address.

Source: INA228 datasheet, p. 19.

## Register access model

| Operation | Bus sequence |
| --- | --- |
| Set pointer | START, address+W, register pointer, STOP or repeated START |
| Write register | START, address+W, register pointer, data MSB ... data LSB, STOP or repeated START |
| Read current pointer | START, address+R, data MSB ... data LSB, STOP |
| Read specific register | START, address+W, register pointer, repeated START, address+R, data MSB ... data LSB, STOP |

Every write operation requires the register pointer byte. A read uses the last pointer value written; repeated reads from the same register can omit the pointer write until the pointer is changed. Registers with more than 16 bits use the same MSB-first byte order as the 16-bit timing diagrams.

Source: INA228 datasheet, pp. 19-20.

## High-speed I2C

High-speed mode entry is part of the INA228 bus protocol:

- While the bus is idle, the controller sends START plus high-speed controller code `00001XXXb`.
- That controller-code byte is sent in standard or fast mode at no more than 400 kHz.
- INA228 does not acknowledge the controller code, but switches internal filters for 2.94 MHz operation after recognizing it.
- The controller sends a repeated START, then uses the same register read/write format at up to 2.94 MHz.
- Repeated START conditions keep the device in high-speed mode; STOP ends high-speed mode and returns the filters to fast/standard mode.

Source: INA228 datasheet, pp. 20-21.

## SMBus alert response

When `ALERT` is asserted, the controller can issue SMBus Alert Response address `0001100b` with the R/W bit high. Any INA228 that generated an alert acknowledges and sends its own address. If several targets respond, bus arbitration selects one responder; a losing INA228 does not acknowledge and continues holding `ALERT` low until it wins arbitration or the interrupt is cleared.

Source: INA228 datasheet, p. 21.

## Transfer limits and unsupported features

- Defined INA228 registers are 16, 24, or 40 bits; register `0x04`, `0x05`, and `0x07` are 24-bit result registers; `0x09` and `0x0A` are 40-bit accumulators.
- The write-word and read-word timing figures state that INA228 does not support packet error checking (PEC).
- The same figures state that INA228 does not perform clock stretching.

Source: INA228 datasheet, pp. 20-21.
