# Wildforce ESP32-C3 Dashboard

ระบบนี้รับข้อมูลสุขภาพและตำแหน่งเจ้าหน้าที่ผ่าน LoRa แล้วส่งต่อจาก ESP32-C3 Receiver ไปยัง Vercel Cloud Dashboard โดยหน้าเว็บจะแสดงชื่อเจ้าหน้าที่, SpO2, Heart Rate, Status และแผนที่จากพิกัด GPS

## โครงสร้างโปรเจกต์

```text
New folder/
|-- Reciver_WebSocket_dashboard.ino   # โปรแกรมหลักฝั่ง ESP32-C3 Receiver
|-- image.png                         # ภาพต้นแบบ Dashboard
|-- data/                              # ไฟล์ที่จะอัปโหลดเข้า LittleFS
|   |-- index.html                     # หน้าแรกตั้งค่า WiFi
|   |-- dashboard.html                 # หน้า Dashboard จริง
|   |-- style.css                      # รูปแบบและตำแหน่ง Dashboard
|   |-- dashboard.js                   # ดึง API และอัปเดตค่าหน้าเว็บ
|   |-- image.png                      # สำเนาภาพสำหรับ LittleFS
|   |-- preview.html                   # Preview Dashboard แบบ demo
|   `-- Json_Test/                     # ชุดทดสอบ JSON แยกต่างหาก
|       |-- preview.html               # หน้าอ่านและแสดง data.json
|       |-- data.json                  # ข้อมูลจำลองที่ใช้ทดสอบ
|       |-- preview.js                  # แปลง JSON และสร้างแผนที่
|       `-- preview.css                 # รูปแบบหน้า Json Test
`-- LoRa_Test_Sender/
    `-- LoRa_Test_Sender.ino            # โปรแกรมส่ง JSON ทดสอบผ่าน LoRa
```

## จุดประสงค์ของระบบ

- รับค่าชีพจรและ SpO2 ของเจ้าหน้าที่ผ่าน LoRa
- รับตำแหน่ง GPS จากข้อมูล `lat` และ `lon`
- แสดงข้อมูลบน Web Dashboard ที่เปิดจาก ESP32-C3
- แจ้งสถานะท่าทาง เช่น ยืน, ล้ม และเอียง
- ใช้ชุด `Json_Test` ตรวจหน้าตาและข้อมูลก่อนต่ออุปกรณ์จริง

## การไหลของข้อมูลจริง

1. ตัวส่ง LoRa ส่ง JSON ไปยัง Receiver
2. `Reciver_WebSocket_dashboard.ino` รับและแปลง JSON ด้วย ArduinoJson
3. Receiver เก็บค่าล่าสุดและประวัติ SpO2/Heart Rate แล้วส่ง HTTPS POST ไปยัง Vercel `/api/data`
4. Vercel รับข้อมูลล่าสุดและตอบข้อมูลผ่าน `/api/data`
5. `dashboard.js` เรียก Vercel `/api/data` ทุก 2 วินาที
6. หน้า Dashboard แสดงค่า MAP, Heart Rate, SpO2, ชื่อ และสถานะ

## หน้าที่ของไฟล์

### `Reciver_WebSocket_dashboard.ino`

โปรแกรมหลักของ ESP32-C3 ทำหน้าที่เชื่อมต่อ WiFi, เปิด Access Point เมื่อตั้งค่า WiFi ไม่สำเร็จ, เริ่มต้น LittleFS, รับข้อมูล LoRa, เปิด Web Server และตอบ JSON ให้หน้าเว็บ

### โฟลเดอร์ `data`

ไฟล์ในโฟลเดอร์นี้ต้องอัปโหลดเข้า LittleFS ของ ESP32-C3:

- `index.html`: หน้า `/` สำหรับตั้งค่า WiFi
- `dashboard.html`: หน้า `/data` สำหรับใช้งานจริง
- `style.css`: รูปแบบและตำแหน่งองค์ประกอบ Dashboard
- `dashboard.js`: อ่าน API และอัปเดตข้อมูลบนหน้าเว็บ
- `image.png`: ภาพต้นแบบ Dashboard
- `preview.html`: หน้า Dashboard แบบ demo

### โฟลเดอร์ `data/Json_Test`

ชุดทดสอบแยกจากระบบจริง ใช้ `preview.js` อ่าน `data.json` แล้วแสดงชื่อ, SpO2, Heart Rate, Status และ MAP โดยไม่ต้องใช้ ESP32

### `LoRa_Test_Sender/LoRa_Test_Sender.ino`

โปรแกรมส่งข้อมูลทดสอบสำหรับ ESP32-C3 อีกตัว ส่ง JSON ทุก 2 วินาทีและวนค่าปกติ, SpO2 ต่ำ, Heart Rate ต่ำ, ล้ม และเอียง

## รูปแบบ JSON

```json
{
  "spo2": 98,
  "hr": 72,
  "status": 0,
  "lat": 13.7563,
  "lon": 100.5018
}
```

| Field | ความหมาย | ตัวอย่าง |
|---|---|---:|
| `spo2` | ระดับออกซิเจนในเลือด (%) | `98` |
| `hr` | อัตราการเต้นหัวใจ (BPM) | `72` |
| `status` | สถานะท่าทางเจ้าหน้าที่ | `0` |
| `lat` | ละติจูด | `13.7563` |
| `lon` | ลองจิจูด | `100.5018` |

ค่า `status` คือ `0` ยืนตรง, `1` ล้ม, `2` เอียงซ้าย และ `3` เอียงขวา

## Route ของ Web Server

| URL | หน้าที่ |
|---|---|
| `/` | หน้า WiFi Setting |
| `/wifi/save` | รับ SSID/Password แบบ POST |
| `/data` | หน้า Dashboard จริง |
| `/preview.html` | หน้า Dashboard demo |
| `/api/data` | JSON ล่าสุดจาก Receiver |
| `/style.css` | ไฟล์ CSS |
| `/dashboard.js` | ไฟล์ JavaScript |
| `/image.png` | ภาพ Dashboard |

## Public Cloud Dashboard

เปิด dashboard จริงได้ที่:

```text
https://data-jet-iota.vercel.app/preview.html
```

หลังอัปโหลด `Reciver_WebSocket_dashboard.ino` ลง Receiver แล้ว ให้ตั้งค่า WiFi ของ Receiver ให้เชื่อมต่ออินเทอร์เน็ตได้ เมื่อ Receiver รับข้อมูล LoRa จะส่งข้อมูลไปยัง Vercel โดยอัตโนมัติ หน้าเว็บจะอัปเดตทุก 2 วินาที

API รับข้อมูลคือ `https://data-jet-iota.vercel.app/api/data` โดยรับ HTTP POST เป็น JSON ฟิลด์ `name`, `spo2`, `hr`, `status`, `lat` และ `lon`

## วิธีทดสอบ JSON ด้วย Go Live

เปิด Live Server ที่โฟลเดอร์ `data` แล้วเข้า:

```text
http://localhost:5500/Json_Test/preview.html
```

แก้ค่าที่ต้องการทดสอบใน `data/Json_Test/data.json` แล้วรีเฟรชหน้าเว็บ ค่า `lat/lon` จะถูกนำไปสร้างแผนที่ OpenStreetMap ซึ่งต้องใช้อินเทอร์เน็ต

### เปิด Preview บนเครื่องอื่น

ต้องส่งทั้งโฟลเดอร์โปรเจกต์ โดยคงโครงสร้าง `data` ไว้เหมือนเดิม จากนั้นดับเบิลคลิก `start-preview.bat` ที่โฟลเดอร์หลัก ระบบจะเปิดหน้า preview อัตโนมัติที่:

```text
http://127.0.0.1:5501/preview.html
```

server จะรับการเชื่อมต่อจากเครือข่ายด้วย `0.0.0.0` เครื่องปลายทางจึงเปิดได้ด้วย IP ของเครื่องที่รัน server เช่น `http://192.168.1.20:5501/preview.html` โดยเครื่องทั้งสองต้องอยู่เครือข่ายเดียวกัน และต้องอนุญาตพอร์ต `5501` ใน Windows Firewall หากระบบถาม

## รันหน้าเว็บพร้อมอัปโหลด Receiver

สามารถใช้ VS Code Task ชื่อ `Run dashboard and upload receiver` เพื่อเปิด preview และคอมไพล์/อัปโหลด `Reciver_WebSocket_dashboard.ino` ในคำสั่งเดียว:

1. ติดตั้ง Arduino CLI, ESP32 board package, Python และไลบรารีที่ระบุไว้ด้านบน
2. ตรวจสอบว่า `arduino-cli` อยู่ใน PATH และตั้งค่า core/ไลบรารีเรียบร้อยแล้ว
3. กด `Ctrl+Shift+P` แล้วเลือก `Tasks: Run Task`
4. เลือก `Run dashboard and upload receiver` และกรอกพอร์ต COM ของบอร์ด receiver

Task นี้เปิด preview ที่ `http://localhost:5501/preview.html` และพิมพ์ URL สำหรับเครื่องอื่นให้ใน terminal โดย server จะทำงานต่อหลังอัปโหลดเสร็จ ส่วนข้อมูลจริงจาก LoRa ให้เปิด IP ของ receiver แล้วเข้า `/data` หลังอัปโหลดสำเร็จ

ไฟล์ `tools/run-dashboard.ps1` เป็นตัวทำงานเบื้องหลัง หากไม่ต้องการอัปโหลดอัตโนมัติ ให้เปิด preview ด้วย Live Server ตามวิธีด้านบนแทน เพราะไฟล์ `.ino` ต้องทำงานบนบอร์ด ESP32 ไม่สามารถรันภายในเบราว์เซอร์ได้

## วิธีใช้งานบน ESP32-C3

1. ติดตั้ง ESP32 Board Package ใน Arduino IDE
2. ติดตั้งไลบรารี `LoRa` และ `ArduinoJson`
3. เลือกบอร์ด `ESP32C3 Dev Module`
4. อัปโหลด `Reciver_WebSocket_dashboard.ino` ลงบอร์ดตัวรับ
5. อัปโหลดโฟลเดอร์ `data` เข้า LittleFS
6. อัปโหลด `LoRa_Test_Sender.ino` ลง ESP32-C3 อีกตัวถ้าต้องการทดสอบการส่ง
7. เปิด Serial Monitor ที่ `115200` baud
8. เปิด IP ของบอร์ดตัวรับ แล้วเข้า `/data`

## ค่า LoRa ที่ต้องตรงกัน

- Frequency: `923E6`
- Sync Word: `0xF3`
- Spreading Factor: `11`
- Bandwidth: `125E3`

## การต่อขา LoRa

| LoRa | ESP32-C3 |
|---|---:|
| CS | GPIO7 |
| RST | GPIO2 |
| DIO0 | GPIO3 |
| SCK | GPIO4 |
| MISO | GPIO5 |
| MOSI | GPIO6 |

## หมายเหตุ

- ต้องใช้บอร์ด LoRa สองชุด หากต้องการทดสอบการส่งและรับพร้อมกัน
- Go Live ใช้ข้อมูลจาก `data/Json_Test/data.json` ไม่ใช่ข้อมูลจาก LoRa
- การใช้งานจริงบน ESP32 ต้องอัปโหลดทั้ง Sketch และโฟลเดอร์ `data` เข้าอุปกรณ์
- แผนที่ต้องใช้อินเทอร์เน็ตจึงจะแสดงผลได้
