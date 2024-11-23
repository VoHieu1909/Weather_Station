#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>  // Thư viện JSON
#include <SoftwareSerial.h>

#define Serial_TX 17
#define Serial_RX 16

char WiFi_SSID[] = "UIT Public";           // Thay thế bằng SSID của bạn
char WiFi_Password[] = "";  // Thay thế bằng mật khẩu của bạn

SoftwareSerial SerialSTM = SoftwareSerial(Serial_RX, Serial_TX);

String latitude;
String longitude;

int ESP_API = 0;   //Bat = 1 --> Call API
int ESP_DAY = 0;   //Chon ngay tra thong tin ve cho STM
int ESP_TIME = 0;  //Chon gio tra ve cho STM

struct data {
  float temp;
  float precip;
  float hud;
  float D_DAY;         //Them gui lai bien ngay
  float D_TIME;        //Them gui lai bien gio
};

struct city {
  int GET_API;
  int GET_DAY;
  int GET_TIME;
  float STM_Latitude;
  float STM_Longitute;
};

city data_receive;
int uart_lasttime = 0;
data data_trant;
data Weather_Data[8][24];      //7 ngay, 24 gio, ngay 0 khong tinh        

void Init_Weather_Data(DynamicJsonDocument doc, data Weather_Data[7][24]) {
    for (int i = 0; i < 7; i++) { // 7 ngày
        for (int j = 0; j < 24; j++) { // 24 giờ
            Weather_Data[i][j].temp = doc["hourly"]["temperature_2m"][i * 24 + j];
            Weather_Data[i][j].precip = doc["hourly"]["precipitation"][i * 24 + j];
            Weather_Data[i][j].hud = doc["hourly"]["relative_humidity_2m"][i * 24 + j];
            Weather_Data[i][j].D_DAY = doc["daily"]["date"][i];
            Weather_Data[i][j].D_TIME = doc["hourly"]["time"][i * 24 + j];
        }
    }
}

void setup() {
  Serial.begin(115200);  // Khởi tạo Serial Monitor
  WiFi.begin(WiFi_SSID, WiFi_Password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi...");
    delay(1000);
  }

  Serial.println("Connected to WiFi");
  digitalWrite(2, LOW);
  SerialSTM.begin(9600, SWSERIAL_8N1);
  uart_lasttime = millis();
}

void callApi() {
  //Them cai ESP_API = 1; vao if
  if (WiFi.status() == WL_CONNECTED && ESP_API == 1) {          //Neu wifi ket noi va ESP_API trang thai = 1 thi GET API
    HTTPClient http;
    ESP_API = 0;    //RESET ESP_API
    //Dự báo thời tiết cho một thành phố bất kì khi có KINH ĐỘ và VĨ ĐỘ
    
    String weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + "&hourly=temperature_2m,relative_humidity_2m,precipitation";
    http.begin(weatherUrl);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();                  //Thong tin cua PAYLOAD
      Serial.println("Weather Data Response: ");          //In PAYLOAD ra man hinh
      Serial.println(payload);  // In payload ra Serial Monitor
      // Phân tích cú pháp JSON
      DynamicJsonDocument doc(2048);                               // Tạo một tài liệu JSON
      DeserializationError error = deserializeJson(doc, payload);  // Phân tích cú pháp JSON

      if (error) {
        Serial.print("JSON Error: ");
        Serial.println(error.f_str());
        return;
      }
      // Lấy nhiệt độ và lượng mưa từ payload cập nhật vào mảng thời 
      Init_Weather_Data(doc, Weather_Data);
      
      data_trant.temp = Weather_Data[ESP_DAY][ESP_TIME].temp;;
      data_trant.precip = Weather_Data[ESP_DAY][ESP_TIME].precip;
      data_trant.hud = Weather_Data[ESP_DAY][ESP_TIME].hud;
      
      Serial.print("Temperature: ");
      Serial.print(Weather_Data[ESP_DAY][ESP_TIME].temp);
      Serial.println(" °C");

      Serial.print("Precipitation: ");
      Serial.print(Weather_Data[ESP_DAY][ESP_TIME].precip);
      Serial.println(" mm");
      
      Serial.print("humidity: ");
      Serial.print(Weather_Data[ESP_DAY][ESP_TIME].hud);
      Serial.println(" mm");
    } 
    else {
      Serial.print("Error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected!");
  }
}

void Read_UART() {
  SerialSTM.read((uint8_t*)&data_receive, sizeof(city));
  latitude = String(data_receive.STM_Latitude);    //Nap latitude       //Nhan xong moi ep kieu
  longitude = String(data_receive.STM_Longitute);  //Nap longtitude
  ESP_API = data_receive.GET_API;             //Bien nay la 1
  ESP_DAY = data_receive.GET_DAY - 1;
  ESP_TIME = data_receive.GET_TIME;
}

void loop() {  
  if (SerialSTM.available()) {
    Read_UART();
    delay(100);
}
  callApi();
  SerialSTM.write((uint8_t*)&data_trant, sizeof(data_trant));
}