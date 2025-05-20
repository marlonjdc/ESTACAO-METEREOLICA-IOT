# Estação Meteorológica IoT 

Este projeto visa desenvolver um sistema de monitoramento de temperatura  via estação meteorológica conectada à internet (IoT). Os valores coletados pela estação, enviados para um banco de dados,  eesses dados serem enviados para o servidor para possibilitar a visualização do seu histórico.

## Funcionalidades:
* [x] Controlde e monitoramento da estação;
* [x] Envio dos dados coletados pela estação para banco de dados.
* [x] Visualização dos dados climatológicos e do tempo de irrigação ativo


O projeto possui diversas melhorias, implementando cada vez mais novos recursos. 

## Componentes Utilizados

* ESP32 
* Anemômetro
* Módulo Sensor de Luminosidade TEMT600
* Sensor de Pressão, Temperatura e altitude BME280

## Software 
### Estação Meteorológica

O firmware é compilado usando o Arduino IDE e os softwares de envio de dados são desenvolvidos utilizando C/C++, no qual se faz uso das bibliotecas abaixo listadas: 

* `WiFi.h`: Conexão do ESP32 com WiFi.
* `IOXhop_FirebaseESP32.h`: Comunicação do ESP32 com o Firebase;
* `ArduinoJson.h`: Manipulação de informações no formato JSON (v5.13.3);
* `Adafruit_BME280.h`: Manipulação do sensor BME280

### Aplicação
A interface é construida em python, para isso, ela utiliza algumas bibliotecas que permitem o seu desenvolvimento, nas quais incluem:

* `flet`: Construção da interface
* `pyrebase4`: Comunicação com o Firebase

## Modo de Funcionamento 


1. Funcionando continuamente, onde os sensores realizam medições dos parâmetros necessários;
2. Os dados são enviados para um servidor (Firebase) com um intervalo programado;
3. A PWA tem acesso ao Firebase, onde os dados são exibidos
