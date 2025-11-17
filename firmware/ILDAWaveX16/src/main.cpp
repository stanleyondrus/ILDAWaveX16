#include <Arduino.h> 
#include <vector>
#include <Adafruit_NeoPixel.h>
#include <SDCard.h>
#include <Renderer.h>
#include <IDNServer.h>
#include <IWPServer.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "driver/timer.h"
#include <Preferences.h>
#include "esp_task_wdt.h"

using namespace std;

Preferences preferences;
Adafruit_NeoPixel pixels(1, PIN_LED, NEO_GRB + NEO_KHZ800);
SDCard sd;
Renderer renderer;
AsyncWebServer server(80);
File current_file;

IDNServer idn;
IWPServer iwp;

void init_wifi() {
  String ssid;
  String password;

  preferences.begin("app", false); 
  ssid = preferences.getString("ssid", ""); 
  password = preferences.getString("password", "");
  preferences.end();

  if (ssid == "" || password == ""){
    ssid = "{default_ssid}>";
    password = "{default_pass}";
  }

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("ILDAWaveX16", "ildawave");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WiFi"));
  
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(F("."));
      attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) Serial.printf("\nLocal IP: %s\n", WiFi.localIP().toString().c_str());
  else Serial.println("\nWiFi Connection Failed!");
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>SD Player</title>
<style>
html,body{height:100%;margin:0;padding:0;}
body{display:flex;flex-direction:column;font-family:sans-serif;background:#f4f4f4;}
main{flex:1;padding:20px;padding-top: 0;}
table{border-collapse:collapse;width:100%;margin-bottom:20px;}
th,td{padding:10px;border:1px solid #ccc;text-align:left;}
tr:hover{background:#eee;}
input[type=number]{width:80px;}
button{padding:10px 20px;margin:5px;font-size:16px;}
.selected{background:#d0eaff!important;}
fieldset{border:1px solid #ccc;padding:15px;margin-top:20px;}
legend{font-weight:bold;}
#tableContainer{max-height:300px;overflow-y:auto;margin-bottom:20px;}
footer{font-size:12px;text-align:center;color:#888;padding:10px 0;background:#f0f0f0;}
input[type=range]{width:20%;margin:10px 0;}
</style>
</head>
<body>

<main>
<h2>ILDAWaveX16</h2>

<fieldset>
<legend>📁 SD Card</legend>
<div id="tableContainer">
<table id="fileTable"><tr><th>Filename</th><th>Size</th></tr><!-- FILE_ROWS --></table>
</div>
<button onclick="playFile()">▶️ Play</button>
<button onclick="stopFile()">⏹️ Stop</button>
</fieldset>

<fieldset>
<legend>⚙️ Projection Settings</legend>
<label for="scanRate">Scan Period [us]: <span id="rateValue">10</span></label><br>
<input type="range" id="scanRate" min="10" max="10000" step="1" value="10" oninput="updateSettings()"><br><br>

<label for="brightness">Brightness: <span id="brightnessValue">50</span>%</label><br>
<input type="range" id="brightness" min="0" max="100" step="1" value="100" oninput="updateSettings()">
</fieldset>

<fieldset>
<legend>⚙️ Wi-Fi Settings</legend>
<label for="ssid">SSID:</label><input type="text" id="ssid">
<label for="pass">Pass:</label><input type="text" id="pass">
<button onclick="setWiFi()">Save & Connect</button>
</fieldset>
</main>

<footer>
StanleyProjects &nbsp; | &nbsp; VER 0.1
</footer>

<script>
let selectedFile = null;
document.addEventListener("DOMContentLoaded",()=>{
  document.querySelectorAll("#fileTable tr").forEach((row,i)=>{
    if(i===0) return;
    row.addEventListener("click",()=>{
      document.querySelectorAll("#fileTable tr").forEach(r=>r.classList.remove("selected"));
      row.classList.add("selected");
      selectedFile = row.dataset.filename;
    });
    row.addEventListener("dblclick",()=>{
      selectedFile = row.dataset.filename;
      playFile();
    });
  });
});

function playFile(){
  if(!selectedFile){ alert("Select a file."); return; }
  const rate = document.getElementById("scanRate").value;
  fetch(`/play?file=${encodeURIComponent(selectedFile)}&rate=${rate}`);
}
function stopFile(){
  fetch("/stop");
}
function updateSettings() {
  const rateSlider = document.getElementById("scanRate");
  const brightnessSlider = document.getElementById("brightness");
  const rateValue = document.getElementById("rateValue");
  const brightnessValue = document.getElementById("brightnessValue");

  rateValue.textContent = rateSlider.value;
  brightnessValue.textContent = brightnessSlider.value;

  const rate = rateSlider.value;
  const brightness = brightnessSlider.value;

  let url = "/control?";
  const params = [];
  if(rate) params.push(`rate=${encodeURIComponent(rate)}`);
  if(brightness) params.push(`brightness=${encodeURIComponent(brightness)}`);
  url += params.join("&");

  fetch(url).catch(console.error);
}
// function setSettings(){
//   const rate = document.getElementById("scanRate").value;
//   const brightness = document.getElementById("brightness").value;
//   let url = "/control?";
//   const params = [];
//   if(rate) params.push(`rate=${encodeURIComponent(rate)}`);
//   if(brightness) params.push(`brightness=${encodeURIComponent(brightness)}`);
//   url += params.join("&");
//   fetch(url);
// }
function setWiFi(){
  const ssid = document.getElementById("ssid").value;
  const pass = document.getElementById("pass").value;
  fetch(`/set_wifi?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`);
}
</script>
</body>
</html>
)rawliteral";

void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String page = index_html;
    page.replace(F("<!-- FILE_ROWS -->"), sd.generateFileRows());
    request->send(200, "text/html", page);
    });

  server.on("/play", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file");
      return;
    }

    if (request->hasParam("rate")) {
      int rate = request->getParam("rate")->value().toInt();
      renderer.change_freq(rate);
    }

    if (request->hasParam("brightness")) {
      int brightness = request->getParam("brightness")->value().toInt();
      renderer.change_brightness(brightness);
    }

    String file = request->getParam("file")->value();
    int rate = request->getParam("rate")->value().toInt();

    renderer.sd_stop();

    current_file = sd.getFile(file.c_str());

    if (renderer.rendererRunning == 0) renderer.start();
    renderer.sd_start(current_file);

    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();

    request->send(200, "text/plain", "Playing " + file + " at " + String(rate) + " ms");
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    renderer.sd_stop();
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    request->send(200, "text/plain", "Stopped");
  });

  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    bool handled = false;

    if (request->hasParam("rate")) {
      int rate = request->getParam("rate")->value().toInt();
      renderer.change_freq(rate);
      handled = true;
    }

    if (request->hasParam("brightness")) {
      int brightness = request->getParam("brightness")->value().toInt();
      renderer.change_brightness(brightness);
      handled = true;
    }

    if (handled) {
      String response = "Updated settings:";
      if (request->hasParam("rate")) response += " rate=" + request->getParam("rate")->value();
      if (request->hasParam("brightness")) response += " brightness=" + request->getParam("brightness")->value();
      request->send(200, "text/plain", response);
    } else request->send(400, "text/plain", "No valid parameters provided");
  });

  server.on("/set_wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid") || !request->hasParam("pass")) {
      request->send(400, "text/plain", "Missing ssid or pass parameter");
      return;
    }

    String ssid = request->getParam("ssid")->value();
    String pass = request->getParam("pass")->value();

    if (ssid == "") return;

    preferences.begin("app", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", pass);
    preferences.end();

    request->send(200, "text/plain", "Wi-Fi settings saved. Restarting.");
    ESP.restart();
  });

  server.begin();
}

void udp_loop(void* pvParameters) {
  while(1) {
    idn.loop();
    iwp.loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void setup() {
  Serial.begin(115200);
  // while(!Serial.availableForWrite()){}

  pixels.begin();
  pixels.clear();
  pixels.show();

  pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  pixels.show();

  init_wifi();
  esp_wifi_set_ps(WIFI_PS_NONE);
  sd.begin();
  sd.mount();
  idn.begin();
  idn.setRendererHandle(&renderer);
  iwp.begin();
  iwp.setRendererHandle(&renderer);

  renderer.begin();
  renderer.start();

  xTaskCreatePinnedToCore (udp_loop, "udp_loop", 8192, NULL, 2, NULL, 0);

  setupServer();

  // sd.list();

  // current_file = sd.getFile("/animation.ild");
  // renderer.sd_start(current_file);
}

void loop(){ vTaskDelete(NULL); }