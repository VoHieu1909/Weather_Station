#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>  // Thư viện JSON
#include <SoftwareSerial.h>
#include <Arduino.h>

#define Serial_TX 17
#define Serial_RX 16

char WiFi_SSID[] = "OPPO A5s";           // Thay thế bằng SSID của bạn
char WiFi_Password[] = "nasaiudau";  // Thay thế bằng mật khẩu của bạn

SoftwareSerial SerialSTM = SoftwareSerial(Serial_RX, Serial_TX);

String latitude;
String longitude;

int ESP_API = 0;   //Bat = 1 --> Call API
int ESP_DAY = 0;   //Chon ngay tra thong tin ve cho STM
int ESP_TIME = 0;  //Chon gio tra ve cho STM

struct city {
  int GET_API;
  int GET_DAY;
  int GET_TIME;
  float STM_Latitude;
  float STM_Longitute;
};

struct data {
    float temp;
    float precip;
    float hud;
    int D_Day;
    int D_Month;
    int D_Year;
};

city data_receive;
int uart_lasttime = 0;
data data_trant;
data Weather_Data[7][24];      //7 ngay, 24 gio, ngay 0 khong tinh        

void Init_Weather_Data(DynamicJsonDocument &doc) {
    for (int i = 0; i < 7; i++) { // 7 ngày
        String dateStr = doc["hourly"]["time"][i * 24].as<String>(); // Thay đổi chỉ số để lấy thời gian đầu tiên của ngày
        for (int j = 0; j < 24; j++) { // 24 giờ
            Weather_Data[i][j].temp = doc["hourly"]["temperature_2m"][i * 24 + j];
            Weather_Data[i][j].precip = doc["hourly"]["precipitation"][i * 24 + j];
            Weather_Data[i][j].hud = doc["hourly"]["relative_humidity_2m"][i * 24 + j];

            Weather_Data[i][j].D_Day = dateStr.substring(8, 10).toInt();
            Weather_Data[i][j].D_Month = dateStr.substring(5, 7).toInt();
            Weather_Data[i][j].D_Year = dateStr.substring(0, 4).toInt();
        }
    }
}

void setup() {
  Serial.begin(9600);  // Khởi tạo Serial Monitor
  WiFi.begin(WiFi_SSID, WiFi_Password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi...");
    delay(1000);
  }

  Serial.println("Connected to WiFi");
  digitalWrite(2, LOW);
  SerialSTM.begin(57600, SWSERIAL_8N1);
  uart_lasttime = millis();
}

void callApi() {
  //Them cai ESP_API = 1; vao if
  if (WiFi.status() == WL_CONNECTED) {          //Neu wifi ket noi va ESP_API trang thai = 1 thi GET API
        HTTPClient http;
        //Dự báo thời tiết cho một thành phố bất kì khi có KINH ĐỘ và VĨ ĐỘ
        String weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude="+latitude+"&longitude="+longitude+"&hourly=temperature_2m,relative_humidity_2m,precipitation";
        http.begin(weatherUrl);
        int httpResponseCode = http.GET();
      
      if (httpResponseCode > 0) {
        String payload = http.getString();                  //Thong tin cua PAYLOAD
        Serial.println("Weather Data Response: ");          //In PAYLOAD ra man hinh
        Serial.println(payload);  // In payload ra Serial Monitor
        // Phân tích cú pháp JSON
        DynamicJsonDocument doc(4096);                               // Tạo một tài liệu JSON
        DeserializationError error = deserializeJson(doc, payload);  // Phân tích cú pháp JSON

        if (error) {
          Serial.print("JSON Error: ");
          Serial.println(error.f_str());
          callApi();
          return;
        }
        // Lấy nhiệt độ và lượng mưa từ payload cập nhật vào mảng thời 
        Init_Weather_Data(doc);
    } 
    else {
      Serial.print("Error: ");
      Serial.println(httpResponseCode);

    }
    http.end();
  } else {
    Serial.println("WiFi disconnected!");
    WiFi.begin(WiFi_SSID, WiFi_Password);
  }
}

void Check_Buffer(){
  for(int i=0; i<7; i++){
    Serial.println("-------------------");
    for(int j=0; j<24; j++){
      Serial.println(Weather_Data[i][j].temp);
      Serial.println(Weather_Data[i][j].precip);
      Serial.println(Weather_Data[i][j].hud);
      Serial.println(Weather_Data[i][j].D_Day);
      Serial.println(Weather_Data[i][j].D_Month);
      Serial.println(Weather_Data[i][j].D_Year);
    }
  }
}

void Get_Package(){
  data_trant.temp = Weather_Data[ESP_DAY][ESP_TIME].temp;;
  data_trant.precip = Weather_Data[ESP_DAY][ESP_TIME].precip;
  data_trant.hud = Weather_Data[ESP_DAY][ESP_TIME].hud;
  data_trant.D_Day = Weather_Data[ESP_DAY][ESP_TIME].D_Day;
  data_trant.D_Month = Weather_Data[ESP_DAY][ESP_TIME].D_Month;
  data_trant.D_Year = Weather_Data[ESP_DAY][ESP_TIME].D_Year;

  Serial.println("Check_Package");
  Serial.println("1: ");
  Serial.println(data_trant.temp);
  Serial.println("2: ");
  Serial.println(data_trant.precip);
  Serial.println("3: ");
  Serial.println(data_trant.hud);
  Serial.println("4: ");
  Serial.println(data_trant.D_Day);
  Serial.println("5: ");
  Serial.println(data_trant.D_Month);
  Serial.println("6: ");
  Serial.println(data_trant.D_Year);

}

void Read_UART() {
  SerialSTM.read((uint8_t*)&data_receive, sizeof(city));
  latitude = String(data_receive.STM_Latitude);    //Nap latitude       //Nhan xong moi ep kieu
  longitude = String(data_receive.STM_Longitute);  //Nap longtitude
  ESP_API = data_receive.GET_API;             //Bien nay la 1
  if(data_receive.GET_DAY>1)ESP_DAY = data_receive.GET_DAY - 1;
  ESP_TIME = data_receive.GET_TIME;
  Serial.print("Latitude: ");
  Serial.println(latitude);
  Serial.print("Longtitude: ");
  Serial.println(longitude);
}

void loop() {  
  if (SerialSTM.available()){
    Read_UART();
    delay(300);
    if(ESP_API == 1){
      callApi();
      ESP_API = 0;
    }
    //Check_Buffer();
    Get_Package();
    SerialSTM.write((uint8_t*)&data_trant, sizeof(data_trant));
  }
  delay(100);
}
