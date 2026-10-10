
# Pocket DAC and USBMIDI to Clock Specification

## 現行ファームウェア

- Raspberry Pi Pico（RP2040）をUSB Audio Class 2.0のステレオ出力とUSB-MIDI機器として動作させる。
- オーディオ形式は24-bit PCM、48 kHz。UDA1334AへPIO生成のI2Sで送信する。
- 非同期USB Audioフィードバックで、I2S実クロックにUSB側を同期させる。
- MIDI Clock（`0xF8`）を24 PPQNから2 PPQNへ分周し、GP15から1拍あたり2回、20 msの5 Vパルスを出力する。
- USB-Aホスト機能は現行ファームウェアの対象外。

## Raspberry Pi Pico配線

| Pico端子 | 接続先 |
| --- | --- |
| GP10（物理14番） | UDA1334A BCLK |
| GP11（物理15番） | UDA1334A DIN |
| GP12（物理16番） | UDA1334A WSEL / LRCLK |
| GP15（物理20番） | 2N3904ベース抵抗 |
| GND | DAC・クロック回路の共通GND |
| 3V3(OUT)（物理36番） | DAC VIN（モジュールの対応電圧を確認） |

GP15は5 V機器へ直接接続しない。2N3904と保護部品を含む出力回路を使用する。NPN回路は反転するため、GPIO LOWの間に出力端子がHighになる。
