# Protocol WaveDrom Examples

This file collects small WaveDrom examples for the serial and fieldbus signals that are relevant to TouCAN and this fork.

These diagrams are intentionally simplified:
- UART is shown as logic-level framing.
- RS-232 uses the same async framing as UART, but with inverted voltage polarity.
- RS-422 and RS-485 are shown as differential pairs.
- CAN is shown both at the controller logic side and at the physical CANH/CANL bus side.

If your Markdown preview does not render WaveDrom directly, copy the fenced `wavedrom` blocks into a WaveDrom renderer.

## UART

Example: 8N1 UART sending `0x55` (`01010101` on the wire, LSB first).

```wavedrom
{ signal: [
  { name: 'UART TX', wave: '10=.=.=.=.=.=.=.1', data: ['Start', 'b0=1', 'b1=0', 'b2=1', 'b3=0', 'b4=1', 'b5=0', 'b6=1', 'b7=0'] },
  { name: 'Meaning', wave: 'x.=.=.=.=.=.=.=.x', data: ['Idle', 'LSB first', '', '', '', '', '', '', 'Stop'] }
] }
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
  { name: 'UART bits',      wave: '10=.=.=.=.=.=.=.1', data: ['Start', 'b0=1', 'b1=0', 'b2=1', 'b3=0', 'b4=1', 'b5=0', 'b6=1', 'b7=0'] },
  { name: 'RS-232 line',    wave: '01=.=.=.=.=.=.=.0', data: ['+V', '-V', '+V', '-V', '+V', '-V', '+V', '-V', '+V'] },
  { name: 'Interpretation', wave: 'x.=.=.=.=.=.=.=.x', data: ['Idle = -V', 'Start = +V', '', '', '', '', '', '', 'Stop = -V'] }
] }
```

Typical voltage ranges are around `+3 V` to `+15 V` and `-3 V` to `-15 V`, depending on the transceiver.

## RS-422

RS-422 is a differential physical layer, usually used full duplex. The example below shows one data direction only.

Signal naming varies by vendor. Some datasheets label the non-inverting line `A`, others label it `B`. Check the transceiver datasheet, not just the standard name.

```wavedrom
{ signal: [
  { name: 'UART bits', wave: '10=.=.=.=.=.=.=.1', data: ['Start', 'b0=1', 'b1=0', 'b2=1', 'b3=0', 'b4=1', 'b5=0', 'b6=1', 'b7=0'] },
  { name: 'Line +',    wave: '10=.=.=.=.=.=.=.1' },
  { name: 'Line -',    wave: '01=.=.=.=.=.=.=.0' },
  { name: 'Diff',      wave: '10=.=.=.=.=.=.=.1', data: ['Negative', 'Positive', 'Negative', 'Positive', 'Negative', 'Positive', 'Negative', 'Positive', 'Negative'] }
] }
```

The receiver decides from the voltage difference between the two wires, which improves noise immunity.

## RS-485

RS-485 is also differential. In many embedded systems it is used half duplex, so a direction-enable signal matters as much as the data pair.

Example below:
- `DE` goes high before transmission.
- `TXD` is the local UART data stream.
- The bus pair carries the differential version while the driver is enabled.

```wavedrom
{ signal: [
  { name: 'DE',     wave: '0.1...........0..' },
  { name: 'TXD',    wave: '1.0=.=.=.=.=.=.=1.', data: ['Start', 'b0', 'b1', 'b2', 'b3', 'b4', 'b5', 'b6', 'b7'] },
  { name: 'Bus +',  wave: 'z.10=.=.=.=.=.=.=z.' },
  { name: 'Bus -',  wave: 'z.01=.=.=.=.=.=.=z.' },
  { name: 'State',  wave: 'x.=...........=.x', data: ['Receive/idle', 'Drive bus', 'Release bus'] }
] }
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
  { name: 'TXD',     wave: '1.0.1.0.0.1.1.0.1' },
  { name: 'Meaning', wave: 'x.=.=.=.=.=.=.=.=x', data: ['Recessive', 'Dominant', 'Recessive', 'Dominant', 'Dominant', 'Recessive', 'Recessive', 'Dominant', 'Recessive'] },
  { name: 'Rule',    wave: 'x................x', data: ['0 = dominant, 1 = recessive'] }
] }
```

When two nodes transmit at once, any dominant bit overwrites a recessive bit on the bus. That is the basis of CAN arbitration.

## CAN Physical Bus Side

On the physical bus, the important signals are `CANH` and `CANL`.

Common high-speed CAN behavior:
- Recessive: both lines sit near `2.5 V`.
- Dominant: `CANH` rises and `CANL` falls.

```wavedrom
{ signal: [
  { name: 'Bus state', wave: '1.0.1.0.0.1.1.0.1', data: ['Rec', 'Dom', 'Rec', 'Dom', 'Dom', 'Rec', 'Rec', 'Dom', 'Rec'] },
  { name: 'CANH',      wave: '2.3.2.3.3.2.2.3.2', data: ['~2.5V', '~3.5V', '~2.5V', '~3.5V', '~3.5V', '~2.5V', '~2.5V', '~3.5V', '~2.5V'] },
  { name: 'CANL',      wave: '2.1.2.1.1.2.2.1.2', data: ['~2.5V', '~1.5V', '~2.5V', '~1.5V', '~1.5V', '~2.5V', '~2.5V', '~1.5V', '~2.5V'] },
  { name: 'Diff',      wave: '0.1.0.1.1.0.0.1.0', data: ['~0V', '~2V', '~0V', '~2V', '~2V', '~0V', '~0V', '~2V', '~0V'] }
] }
```

## Summary

- UART is the byte framing format.
- RS-232 is single-ended and inverts the logic electrically.
- RS-422 and RS-485 are differential physical layers carrying UART-like data.
- CAN has a controller-side dominant/recessive logic view and a bus-side CANH/CANL differential view.