#include "internet.h"

void connectToWiFi() {
    #ifdef USE_WIFI_AP
    createAccessPoint();
    #else
    connectToStation();
    #endif
}

void createAccessPoint() {
  Serial.println("Opening access point");
  WiFi.mode(WIFI_AP);
  
  if (WiFi.softAP(AP_WIFI_NETWORK,AP_WIFI_PASSWORD)) {
    Serial.println("Connected");
    Serial.print("Assigned IP address:");
    Serial.println(WiFi.softAPIP());
  }
  else {
    Serial.println("Failed!");
  }
}

void connectToStation() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  while((WiFi.status() != WL_CONNECTED) && (millis() - startAttemptTime < WIFI_TIMEOUT_MS)) {
    Serial.print(".");
    delay(100);
  }

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed!");
  }
  else {
    Serial.println("Connected!");
    Serial.print("Assigned IP address: ");
    Serial.println(WiFi.localIP());
  }

}

void printHTML(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  client.println("<html lang=en><meta charset=UTF-8><meta content=\"width=device-width,initial-scale=1\"name=viewport><link href=data:, rel=icon><title>Robot Commands</title><style>");
  client.println("*{box-sizing:border-box}body,html{height:100%;margin:0;font-family:Arial;text-align:center}.tablink{background-color:#555;color:#fff;float:left;border:none;outline:0;cursor:pointer;padding:14px 12px;font-size:13px;text-align:center;width:25%}.tablink:hover{background-color:#777}.tabcontent{color:#fff;display:none;padding:100px 20px;height:100%}#Comandos{background-color:red}#Manual{background-color:green}#Calibracao{background-color:#00f}#Parametros{background-color:orange}.btn{background-color:#ddd;color:#000;width:80%;font-size:16px;text-align:center;border:none;padding:10px 32px;margin:4px 2px;transition:.3s;cursor:pointer}.btn:hover:enabled{background-color:#3e8e41;color:#fff}.btn:disabled{background-color:#202020;color:#fff;cursor:default}#submit{margin:20px 8px;width:75%}#parar{width:90%}.ajuste{width:80%}.row{display:flex}.column{flex:50%}.slider{-webkit-appearance:none;appearance:none;width:80%;height:20px;border-radius:15px;background-color:#fff;outline:0;opacity:.7;-webkit-transition:.2s;transition:opacity .2s}.slider:hover{opacity:.9}.slider::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:25px;height:25px;border-radius:50%;background:#ff3410;cursor:pointer}");
  client.println("</style><script src=https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js></script><header><button class=tablink onclick='openPage(\"Comandos\",this,\"red\")'id=defaultOpen>Comandos</button> <button class=tablink onclick='openPage(\"Manual\",this,\"green\")'>Manual</button> <button class=tablink onclick='openPage(\"Calibracao\",this,\"blue\")'>Calibração</button> <button class=tablink onclick='openPage(\"Parametros\",this,\"orange\")'>Parâmetros</button></header><div class=tabcontent id=Comandos><h2>Comandos</h2><div class=row><div class=column><button class=btn onclick='sendInfo(\"w1\")'id=andarFrente1>Andar para a frente: Método 1</button> <button class=btn onclick='sendInfo(\"r1\")'id=rodar1>Rodar: Esquerda</button> <button class=btn onclick='sendInfo(\"eq\")'id=equiv>Equilíbrio</button> <button class=btn onclick='sendInfo(\"ob1\")'id=obst1>Andar para a frente: Obstaculo</button></div><div class=column><button class=btn onclick='sendInfo(\"w2\")'id=andarFrente2>Andar para a frente: Método 2");
  client.println("</button> <button class=btn onclick='sendInfo(\"r2\")'id=rodar2>Rodar: Direita</button> <button class=btn onclick='sendInfo(\"d1\")'id=danc1>Dançar: Método 1</button> <button class=btn onclick='sendInfo(\"ob2\")'id=obst2>Andar para a frente: Obstaculo (90°)</button></div></div><button class=btn onclick='sendInfo(\"stop\")'id=parar>Parar</button></div><div class=tabcontent id=Manual><h2>Comando manual</h2><div class=row><div class=column><div><h4>Servo 1</h4><div>Posição: <span class=servoPos id=servoPos1></span>°</div><input class=slider id=servoSlider1 type=range value=45 min=0 max=90 onchange=servo(1,this.value)></div><div><h4>Servo 2</h4><div>Posição: <span class=servoPos id=servoPos2></span>°</div><input class=slider id=servoSlider2 type=range value=45 min=0 max=90 onchange=servo(2,this.value)></div><div><h4>Servo 3</h4><div>Posição: <span class=servoPos id=servoPos3></span>°</div><input class=slider id=servoSlider3 type=range value=45 min=0 max=90 onchange=servo(3,this.value)></div><div>");
  client.println("<h4>Servo 4</h4><div>Posição: <span class=servoPos id=servoPos4></span>°</div><input class=slider id=servoSlider4 type=range value=45 min=0 max=90 onchange=servo(4,this.value)></div></div><div class=column><div><h4>Servo 5</h4><div>Posição: <span class=servoPos id=servoPos5></span>°</div><input class=slider id=servoSlider5 type=range value=20 min=0 max=90 onchange=servo(5,this.value)></div><div><h4>Servo 6</h4><div>Posição: <span class=servoPos id=servoPos6></span>°</div><input class=slider id=servoSlider6 type=range value=20 min=0 max=90 onchange=servo(6,this.value)></div><div><h4>Servo 7</h4><div>Posição: <span class=servoPos id=servoPos7></span>°</div><input class=slider id=servoSlider7 type=range value=20 min=0 max=90 onchange=servo(7,this.value)></div><div><h4>Servo 8</h4><div>Posição: <span class=servoPos id=servoPos8></span>°</div><input class=slider id=servoSlider8 type=range value=20 min=0 max=90 onchange=servo(8,this.value)></div></div></div></div><div class=tabcontent ");
  client.println("id=Calibracao><h2>Calibração</h2><div>Ângulo a calibrar:</div><div class=row><div class=column><button class=btn onclick=selectCalib(!1) id=btnMinCalib>Min = 0°</button></div><div class=column><button class=btn onclick=selectCalib(!0) id=btnMaxCalib>Max = 90°</button></div></div><br><br><div class=row><div class=column><div>Servo 1</div><div class=row><div class=column><button class=btn onclick='sendInfo(\"c=1-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=1+\")'>+</button></div></div><div>Servo 2</div><div class=row><div class=column><button class=btn onclick='sendInfo(\"c=2-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=2+\")'>+</button></div></div><div>Servo 3</div><div class=row><div class=column><button class=btn onclick='sendInfo(\"c=3-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=3+\")'>+</button></div></div><div>Servo 4</div><div class=row><div class=column><button class=btn ");
  client.println("onclick='sendInfo(\"c=4-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=4+\")'>+</button></div></div></div><div class=column><div>Servo 5</div><div class=row><div class=column><button class=btn onclick='sendInfo(\"c=5-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=5+\")'>+</button></div></div><div>Servo 6</div><div class=row><div class=column><button class=btn onclick='sendInfo(\"c=6-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=6+\")'>+</button></div></div><div>Servo 7</div><div class=row><div class=column><button class=btn onclick='sendInfo(\"c=7-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=7+\")'>+</button></div></div><div>Servo 8</div><div class=row><div class=column><button class=btn onclick='sendInfo(\"c=8-\")'>-</button></div><div class=column><button class=btn onclick='sendInfo(\"c=8+\")'>+</button></div></div></div></div></div><div class=tabcontent id=Parametros><h2>");
  client.println("Calibração de parâmetros</h2><div class=row><div class=column><div class=row><div class=column><h3>Andar Count</h3><input class=ajuste id=andar_count type=number value=6 min=1 name=andar_count><h3>Ajuste Esquerda</h3><input class=ajuste id=ajuste_esq type=number value=0 name=ajuste_esq></div></div></div><div class=column><div class=row><div class=column><h3>Rodar Count</h3><input class=ajuste id=rodar_count type=number value=4 min=1 name=rodar_count><h3>Ajuste Direita</h3><input class=ajuste id=ajuste_dir type=number value=0 name=ajuste_dir></div></div></div></div><input class=btn id=submit type=submit onclick=submit_param()><h4>Velocidade de andar</h4><div>Velocidade: <span class=servoPos></span> ms</div><input class=slider id=velocidade type=range value=100 min=50 max=400 onchange=adjust_vel(this.value)></div><script>");
  client.println("function selectCalib(e){var n=document.getElementById(\"btnMinCalib\"),t=document.getElementById(\"btnMaxCalib\");e?(t.disabled=!0,n.disabled=!1,sendInfo(\"cMax\")):(t.disabled=!1,n.disabled=!0,sendInfo(\"cMin\"))}function sendInfo(e){$.get(\"/?\"+e+\"&\"),close}function openPage(e,n,t){var s,a,l;for(a=document.getElementsByClassName(\"tabcontent\"),s=0;s<a.length;s++)a[s].style.display=\"none\";for(l=document.getElementsByClassName(\"tablink\"),s=0;s<l.length;s++)l[s].style.backgroundColor=\"\";switch(document.getElementById(e).style.display=\"block\",n.style.backgroundColor=t,document.getElementById(\"btnMinCalib\").disabled=!1,document.getElementById(\"btnMaxCalib\").disabled=!1,e){case\"Comandos\":sendInfo(\"com\");break;case\"Manual\":sendInfo(\"man\");break;case\"Calibracao\":sendInfo(\"cal\");break;case\"Parametros\":sendInfo(\"par\")}}function submit_param(){sendInfo(\"p=\"+document.getElementById(\"andar_count\").value+\"/\"+document.getElementById(\"rodar_count\").value+\"/\"+document.getElementById(\"ajuste_esq\").value+\"/\"+document.getElementById(\"ajuste_dir\").value)}console.log(\"from script file\"),$.ajaxSetup({timeout:1e3});var sliders=document.getElementsByClassName(\"slider\"),servoPs=document.getElementsByClassName(\"servoPos\");for(i=0;i<sliders.length;i++)servoPs[i].innerHTML=sliders[i].value,sliders[i].oninput=function(){this.parentElement.getElementsByClassName(\"servoPos\")[0].innerHTML=this.value};function servo(e,n){sendInfo(\"value=\"+n+\"/\"+e)}function adjust_vel(e){sendInfo(\"v=\"+e)}document.getElementById(\"defaultOpen\").click()");
  client.println("</script>");

}