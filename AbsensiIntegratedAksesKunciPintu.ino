// Program untuk sistem absensi terintegrasi sistem akses kunci pintu menggunakan sidik jari dan RFID
// dibuat oleh Bing

#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <time.h>

// Network SSID
const char* ssid = "gas";
const char* password = "1234567890";

// pengenal host (server) = IP Address komputer server
const char* host = "192.168.43.165";

// sediakan variabel untuk RFID
#define SDA_PIN 2  //D4
#define RST_PIN 0  //D3

MFRC522 mfrc522(SDA_PIN, RST_PIN);

// sediakan variabel untuk fingerprint
#define FINGERPRINT_RX 4 // pin #2 is IN from sensor (GREEN wire)
#define FINGERPRINT_TX 5 // pin #3 is OUT from arduino (WHITE wire)

SoftwareSerial fingerSerial(FINGERPRINT_RX, FINGERPRINT_TX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// Variabel global untuk menyimpan tanggal hari ini
int today;

// Fungsi untuk mendapatkan tanggal dari NodeMCU
void getDate() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  today = timeinfo->tm_mday;
}

void setup() {
  Serial.begin(9600);

  // setting koneksi wifi
  WiFi.hostname("NodeMCU");
  WiFi.begin(ssid, password);

  // cek koneksi wifi
  while(WiFi.status() != WL_CONNECTED)
  {
    // progress sedang mencari WiFi
    delay(500);
    Serial.print(".");
  }

  Serial.println("Wifi Connected");
  Serial.println("IP Address : ");
  Serial.println(WiFi.localIP());

  // inisialisasi RFID
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Dekatkan Kartu RFID Anda ke Reader");
  Serial.println();

  // inisialisasi fingerprint
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Sensor fingerprint ditemukan!");
  } else {
    Serial.println("Sensor fingerprint tidak ditemukan :(");
    while (1); // hentikan program
  }

  // Mendapatkan tanggal hari ini
  getDate();
}

void loop() {
  // baca kartu RFID
  if(mfrc522.PICC_IsNewCardPresent())
  {
    if(mfrc522.PICC_ReadCardSerial())
    {
      String IDTAG = "";
      for(byte i=0; i<mfrc522.uid.size; i++)
      {
        IDTAG += mfrc522.uid.uidByte[i];
      }
      Serial.print("Nomor kartu RFID: ");
      Serial.println(IDTAG);

      // kirim nomor kartu RFID untuk disimpan ke tabel tmprfid
      WiFiClient client;
      const int httpPort = 80;
      if(!client.connect(host,httpPort))
      {
        Serial.println("Connection Failed");
        return;
      }
      String Link;
      HTTPClient http ;
      Link = "http://192.168.43.165/TA2/kirimkartu.php?nokartu=" + IDTAG;
      http.begin(Link);

      int httpCode = http.GET();
      String payload = http.getString();
      Serial.println(payload);
      http.end();

      String response = "";

      while(client.available()){
        response = client.readStringUntil('\r');
        Serial.println(response);
        if (response == "OK") {
          delay(1000); // Menunggu satu detik
          yield (); // Memberikan kesempatan bagi tugas-tugas latar belakang
          delay(1000); // Menunggu satu detik lagi
        }
      }

      client.stop(); // Menghentikan koneksi client
      delay(500);

      // Mengecek apakah tanggal hari ini berbeda dengan variabel today
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
      int current_date = timeinfo->tm_mday;
      if (current_date != today) {
        // Jika berbeda, maka mengupdate variabel today
        getDate();
      }

      // baca sidik jari
      int fingerID = getFingerprintID();
      if (fingerID > 0) {
        Serial.print("ID sidik jari: ");
        Serial.println(fingerID);

        // kirim ID sidik jari untuk disimpan ke tabel tmpfinger
        WiFiClient client2;
        const int httpPort2 = 80;
        if(!client2.connect(host,httpPort2))
        {
          Serial.println("Connection Failed");
          return;
        }
        String Link2;
        HTTPClient http2 ;
        Link2 = "http://192.168.43.165/TA2/kirimfinger.php?nofinger=" + String(fingerID);
        http2.begin(Link2);

        int httpCode2 = http2.GET();
        String payload2 = http2.getString();
        Serial.println(payload2);
        http2.end();

        String response2 = "";

        while(client2.available()){
          response2 = client2.readStringUntil('\r');
          Serial.println(response2);
          if (response2 == "OK") {
            delay(1000); // Menunggu satu detik
            yield (); // Memberikan kesempatan bagi tugas-tugas latar belakang
            delay(1000); // Menunggu satu detik lagi
          }
        }

        client2.stop(); // Menghentikan koneksi client
        delay(500);

        // Mengecek apakah tanggal hari ini berbeda dengan variabel today
        time_t now2 = time(nullptr);
        struct tm* timeinfo2 = localtime(&now2);
        int current_date2 = timeinfo2->tm_mday;
        if (current_date2 != today) {
          // Jika berbeda, maka mengupdate variabel today
          getDate();
        }

        // buka kunci pintu
        Serial.println("Buka kunci pintu");
        // tambahkan kode untuk mengontrol motor DC atau relay di sini
      }
    }
  }
}

// Fungsi untuk memindai sidik jari dan mengembalikan ID yang cocok
int getFingerprintID() {
  uint8_t p = finger.getImage(); // ambil gambar sidik jari
  switch (p) {
    case FINGERPRINT_OK: // jika gambar berhasil diambil
      Serial.println("Gambar sidik jari berhasil diambil");
      break;
    case FINGERPRINT_NOFINGER: // jika tidak ada sidik jari
      Serial.println("Tidak ada sidik jari");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR: // jika ada kesalahan komunikasi
      Serial.println("Kesalahan komunikasi");
      return -1;
    case FINGERPRINT_IMAGEFAIL: // jika ada kesalahan gambar
      Serial.println("Kesalahan gambar");
            return -1;
    default: // jika ada kesalahan lain
      Serial.println("Kesalahan lain");
      return -1;
  }

  p = finger.image2Tz(); // konversi gambar menjadi fitur
  switch (p) {
    case FINGERPRINT_OK: // jika konversi berhasil
      Serial.println("Gambar sidik jari berhasil dikonversi");
      break;
    case FINGERPRINT_IMAGEMESS: // jika gambar terlalu berantakan
      Serial.println("Gambar sidik jari terlalu berantakan");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR: // jika ada kesalahan komunikasi
      Serial.println("Kesalahan komunikasi");
      return -1;
    case FINGERPRINT_FEATUREFAIL: // jika tidak dapat menemukan fitur
      Serial.println("Tidak dapat menemukan fitur sidik jari");
      return -1;
    case FINGERPRINT_INVALIDIMAGE: // jika gambar tidak valid
      Serial.println("Gambar sidik jari tidak valid");
      return -1;
    default: // jika ada kesalahan lain
      Serial.println("Kesalahan lain");
      return -1;
  }

  p = finger.fingerFastSearch(); // mencari sidik jari yang cocok dengan fitur
  if (p == FINGERPRINT_OK) { // jika ada sidik jari yang cocok
    Serial.println("Sidik jari ditemukan!");
    return finger.fingerID; // kembalikan ID sidik jari
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) { // jika ada kesalahan komunikasi
    Serial.println("Kesalahan komunikasi");
    return -1;
  } else if (p == FINGERPRINT_NOTFOUND) { // jika tidak ada sidik jari yang cocok
    Serial.println("Sidik jari tidak ditemukan");
    return -1;
  } else { // jika ada kesalahan lain
    Serial.println("Kesalahan lain");
    return -1;
  }
}
