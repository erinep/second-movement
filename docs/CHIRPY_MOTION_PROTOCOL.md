# Chirpy Motion Export Protocol

This firmware-side document defines the transport contract shared with the
separate Android receiver repository at `../sensor-watch-motion-rx`. Full app
architecture, UI, storage, tests, and delivery milestones live in that
repository's `DESIGN.md`.

Status: implemented in `time_since_motion_face`.

## Watch behavior

Add `SEND` after `WAKE` in `time_since_motion_face`:

```text
INACT -> REST -> START -> END -> STILL -> WAKE -> SEND -> INACT
```

- Short Alarm cycles pages.
- Long Alarm on `SEND` creates a snapshot and starts Chirpy transmission.
- Light and Mode retain their ordinary roles.
- Display `CHIRP` and the bell indicator while transmitting.
- Prevent timeout/low-energy sleep during the bounded asynchronous send.
- Clear the buzzer and indicators when done, cancelled, or resigning.

Reuse `lib/chirpy_tx` and the asynchronous callback pattern in
`watch-faces/io/chirpy_demo_face.c`. Transmit directly from the in-memory
snapshot; USB and filesystem persistence are not required.

## SWMR version 1 packet

All integers are unsigned little-endian. Timestamps are Unix UTC seconds. The
packet is 212 bytes: 28-byte header, 180-byte history, and four-byte CRC.

| Offset | Size | Field | Value / meaning |
|---:|---:|---|---|
| 0 | 4 | magic | ASCII `SWMR` |
| 4 | 1 | version | `1` |
| 5 | 1 | flags | `0x03`: active-is-one and UTC timestamps |
| 6 | 2 | header_size | `28` |
| 8 | 2 | packet_size | `212` |
| 10 | 2 | sample_period_seconds | `60` |
| 12 | 2 | sample_count | `0..1440` |
| 14 | 4 | oldest_sample_timestamp | zero when sample count is zero |
| 18 | 4 | generated_timestamp | snapshot time |
| 22 | 1 | rest_window_minutes | initially `20` |
| 23 | 1 | rest_percent | initially `80` |
| 24 | 1 | rest_gap_minutes | initially `10` |
| 25 | 1 | wake_min_minutes | initially `5` |
| 26 | 2 | min_rest_minutes | initially `60` |
| 28 | 180 | history | canonical chronological samples |
| 208 | 4 | crc32 | IEEE CRC-32 over bytes `0..207` |

History bit `i` is `(history[i >> 3] >> (i & 7)) & 1`. Bit zero is
the oldest valid sample; bit `sample_count - 1` is newest. One means active and
zero means still. Zero every unused bit. Do not expose the physical ring write
index or physical byte order.

CRC-32 uses reflected polynomial `0xEDB88320`, initial `0xFFFFFFFF`, and final
XOR `0xFFFFFFFF`. Chirpy's per-block CRC-8 remains in addition to this packet
checksum.

## Firmware implementation checklist

1. Add the `SEND` report state.
2. Allocate a fixed 212-byte transmission snapshot in face context if RAM
   permits.
3. Capture count, newest minute, ring position, and generation time together.
4. Repack raw samples into chronological bit order without modifying the ring.
5. Copy analysis constants from `time_since_motion_face.h` into the header.
6. Compute packet CRC-32.
7. Stream sequential bytes through Chirpy's `get_next_byte` callback.
8. Use the existing audible countdown and raw-source completion callback.
9. Test partial history, full/wrapped history, byte boundaries, all-still,
   all-active, alternating samples, timestamps, and CRC vector
   `123456789 -> CBF43926`.

The companion app specification is authoritative for receiver behavior. This
document is authoritative for the firmware packet bytes; change both documents
when versioning the protocol.
