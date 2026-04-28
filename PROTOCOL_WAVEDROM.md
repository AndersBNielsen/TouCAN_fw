# Protocol WaveDrom Examples

This file collects small WaveDrom examples for the serial and fieldbus signals that are relevant to FalCAN and this fork.

These diagrams are intentionally simplified:
- UART is shown as logic-level framing.
- RS-232 uses the same async framing as UART, but with inverted voltage polarity.
- RS-422 and RS-485 are shown as differential pairs.
- CAN is shown both at the controller logic side and at the physical CANH/CANL bus side.

If your Markdown preview does not render WaveDrom directly, copy the fenced `wavedrom` blocks into a WaveDrom renderer.

The examples below use valid WaveDrom wave characters from the official tutorial:
- `0` and `1` for digital low and high
- `h` and `l` for high and low levels
- `z` for high impedance
- `=` and `2..9` for labeled bus states
- `.` to extend the previous state

## UART

Example: 8N1 UART sending `0x55` (`01010101` on the wire, LSB first).

```wavedrom
{ signal: [
  { name: 'Fields',  wave: '23456789234', data: ['Idle', 'Start', 'b0=1', 'b1=0', 'b2=1', 'b3=0', 'b4=1', 'b5=0', 'b6=1', 'b7=0', 'Stop'] },
  { name: 'Level',   wave: '10101010101' }
], config: { hscale: 2 } }
```

Notes:
- Idle is logic `1`.
- Start bit is logic `0`.
- Data bits are sent LSB first.
- Stop bit returns to logic `1`.

## RS-232

RS-232 carries UART-style framing, but the electrical polarity is inverted compared to TTL UART.

Common convention:
- Mark / logic `1` / idle is negative voltage.
- Space / logic `0` is positive voltage.

```wavedrom
{ signal: [
  { name: 'Fields',      wave: '23456789234', data: ['Idle', 'Start', 'b0=1', 'b1=0', 'b2=1', 'b3=0', 'b4=1', 'b5=0', 'b6=1', 'b7=0', 'Stop'] },
  { name: 'UART logic',  wave: '10101010101' },
  { name: 'RS-232 line', wave: '01010101010' },
  { name: 'Voltage',     wave: 'lhlhlhlhlhl' }
], config: { hscale: 2 } }
```

Typical voltage ranges are around `+3 V` to `+15 V` and `-3 V` to `-15 V`, depending on the transceiver.

In this simplified view, `Voltage` is only showing low versus high line state. For RS-232, low corresponds to negative voltage and high corresponds to positive voltage.

## RS-422

RS-422 is a differential physical layer, usually used full duplex. The example below shows one data direction only.

Signal naming varies by vendor. Some datasheets label the non-inverting line `A`, others label it `B`. Check the transceiver datasheet, not just the standard name.

```wavedrom
{ signal: [
  { name: 'Fields', wave: '23456789234', data: ['Idle', 'Start', 'b0=1', 'b1=0', 'b2=1', 'b3=0', 'b4=1', 'b5=0', 'b6=1', 'b7=0', 'Stop'] },
  { name: 'Line +', wave: '10101010101' },
  { name: 'Line -', wave: '01010101010' },
  { name: 'Diff',   wave: '10101010101' }
], config: { hscale: 2 } }
```

The receiver decides from the voltage difference between the two wires, which improves noise immunity.

## RS-485

RS-485 is also differential. In many embedded systems it is used half duplex, so a direction-enable signal matters as much as the data pair.

Example below:
- `DE` goes high before transmission.
- `TXD` is the local UART data stream.
- The bus pair carries the differential version while the driver is enabled.
- Turnaround timing is simplified here so the diagram stays compact.

```wavedrom
{ signal: [
  { name: 'Fields', wave: '23456789234', data: ['Idle', 'Start', 'b0=1', 'b1=0', 'b2=1', 'b3=0', 'b4=1', 'b5=0', 'b6=1', 'b7=0', 'Stop'] },
  { name: 'DE',     wave: '01111111110' },
  { name: 'TXD',    wave: '10101010101' },
  { name: 'Bus +',  wave: 'z010101010z' },
  { name: 'Bus -',  wave: 'z101010101z' }
], config: { hscale: 2 } }
```

In a real half-duplex network, only one node should actively drive the bus at a time.

## CAN Controller Side

At the controller pins, CAN logic is usually discussed as:
- Dominant = logical `0`
- Recessive = logical `1`

This is not UART framing. CAN uses arbitration, stuffing, CRC, ACK, and frame delimiters.

The example below is only a short bit sequence, not a complete valid frame.

```wavedrom
{ signal: [
  { name: 'Meaning', wave: '234556674', data: ['Rec', 'Dom', 'Rec', 'Dom', 'Dom', 'Rec', 'Rec', 'Dom', 'Rec'] },
  { name: 'TXD',     wave: '101001101' }
], config: { hscale: 2 } }
```

When two nodes transmit at once, any dominant bit overwrites a recessive bit on the bus. That is the basis of CAN arbitration.

## CAN Physical Bus Side

On the physical bus, the important signals are `CANH` and `CANL`.

Common high-speed CAN behavior:
- Recessive: both lines sit near `2.5 V`.
- Dominant: `CANH` rises and `CANL` falls.

```wavedrom
{ signal: [
  { name: 'Bus state', wave: '234556674', data: ['Rec', 'Dom', 'Rec', 'Dom', 'Dom', 'Rec', 'Rec', 'Dom', 'Rec'] },
  { name: 'CANH',      wave: '010110010' },
  { name: 'CANL',      wave: '000001000' },
  { name: 'Diff',      wave: '010110010' }
], config: { hscale: 2 } }
```

These three rows are symbolic levels, not literal analog plots. For high-speed CAN, recessive is approximately `CANH ~= 2.5 V`, `CANL ~= 2.5 V`, while dominant is approximately `CANH ~= 3.5 V`, `CANL ~= 1.5 V`.

## Summary

- UART is the byte framing format.
- RS-232 is single-ended and inverts the logic electrically.
- RS-422 and RS-485 are differential physical layers carrying UART-like data.
- CAN has a controller-side dominant/recessive logic view and a bus-side CANH/CANL differential view.