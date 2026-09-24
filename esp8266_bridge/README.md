# ESP8266 TCP-to-UART bridge

ESP8266EX / ESP-12E (4 MB flash) connects to the open Wi-Fi network `hello`,
then connects to `fansss.eu1.netbird.services:43240` over TCP. It sends the
four bytes `ping` when the connection opens and every 15 seconds while it
remains open. After a disconnect, it retries every 5 seconds.

Incoming ASCII words `start` and `stop` (case-insensitive) are forwarded on
the ESP8266 UART TX pin (GPIO1) at 115200 baud as `start\n` or `stop\n`.
Words can be separated by punctuation, a newline, connection close, or 100 ms
of receive inactivity. Other words are ignored. The UART receiver must share
ground with the ESP8266 and use compatible 3.3 V logic. The ESP8266 does not
control the fan GPIO directly.

For bring-up, sending `?` to the ESP8266 UART RX returns one status line
starting with `#wifi=` and containing a Wi-Fi status code, current IP, and
counts of TCP connections, heartbeats, and received commands. Sending `s`
returns a Wi-Fi scan count and whether `hello` was seen. No status is sent
unless queried; the MCU should process only exact `start` and `stop` lines.

The existing 4 MB AT firmware backup is
`/private/tmp/fan-esp8266-tools/esp8266-before.bin` (SHA-256
`872630f71656c0e1d227181ebcb2c55d35c50fe5e5f74aa97ecfa4647469cad5`).
This file is outside the repository and may be removed when temporary files
are cleaned.

The STM32 UART receiver and fan-command handling are separate work. This
ESP8266 firmware only sends the UART command; it does not confirm fan motion.
