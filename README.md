# Sistema de Prevenção de Quedas por Idosos (IoT)

## Descrição do Projeto
Este projeto propõe o desenvolvimento de um **protótipo IoT (Internet of Things)** voltado à **prevenção de quedas em idosos**, contribuindo diretamente para o **Objetivo de Desenvolvimento Sustentável 3 (ODS 3)** *Saúde e Bem-Estar*.  

O sistema utiliza um **microcontrolador ESP32** integrado a um **sensor inercial MPU-6050 (acelerômetro/giroscópio)** para detectar mudanças bruscas de postura ou impacto, características típicas de quedas.  
Quando detectado um evento, o dispositivo:

1. **Emite um alerta sonoro local** via buzzer piezoelétrico;
2. **Publica os dados em nuvem** via protocolo **MQTT**, possibilitando o acompanhamento remoto por cuidadores através de um **dashboard no Node-RED**.

---

## Funcionamento e Uso
### Etapas de funcionamento:
1. **Leitura contínua de dados** do sensor MPU-6050 (acelerações X, Y, Z).
2. **Cálculo da aceleração total (a_mag)** e análise da inclinação (eixo Z).
3. **Identificação de queda:** ocorre quando há impacto seguido de alteração de postura.
4. **Atuação local:** o buzzer é acionado para alerta imediato.
5. **Transmissão MQTT:** dados são publicados no tópico `iot/fall/alert`.
6. **Monitoramento remoto:** o Node-RED consome as mensagens e exibe os eventos em tempo real em um dashboard.

### Simulação
O protótipo foi desenvolvido e validado no **Wokwi**, simulador online de eletrônica e IoT.  
O ambiente permite reproduzir fielmente o comportamento de um circuito físico com suporte a Wi-Fi e MQTT.  

Dashboard de monitoramento desenvolvido no **Node-RED**, mostrando:
- Último evento detectado (tipo, timestamp e valores de aceleração);
- Gráfico em tempo real da aceleração total.

---

## Software e Documentação de Código
O código foi desenvolvido na **Arduino IDE**, utilizando as bibliotecas:
- `Wire.h` — comunicação I²C com o sensor MPU-6050;
- `WiFi.h` — conexão com rede sem fio;
- `PubSubClient.h` — implementação do protocolo MQTT.

---

## Hardware Utilizado
| Componente | Descrição | Ligação |
|-------------|------------|----------|
| **ESP32** | Microcontrolador de 32 bits com Wi-Fi e Bluetooth integrados. | — |
| **MPU-6050** | Sensor de aceleração e giroscópio. Comunicação via I²C. | SDA → GPIO21 / SCL → GPIO22 / VCC → 3V3 / GND → GND |
| **Buzzer Piezoelétrico** | Atuador sonoro para alerta local. | Pino positivo → GPIO12 / Negativo → GND |
| **Node-RED** | Plataforma de visualização e automação MQTT. | Broker: Eclipse Mosquitto (`test.mosquitto.org`) |
| **Wokwi** | Simulador online de IoT e Arduino. | Utilizado para testes e calibração. |

---

## Interfaces, Protocolos e Comunicação
A comunicação do sistema é baseada no **protocolo MQTT (Message Queuing Telemetry Transport)**, operando sobre **TCP/IP**.

### 📡 Estrutura das mensagens publicadas
```json
{
  "event": "fall_confirmed_impact_then_tiltZ",
  "a_mag": 2.18,
  "az": 0.12,
  "timestamp": "2025-11-10T13:05:00Z"
}
```

### Tópico utilizado
- **Publicação:** `iot/fall/alert`  
- **Broker público:** `test.mosquitto.org`  
- **Porta:** `1883`

### Fluxo MQTT
1. **ESP32 (Client Publisher):** coleta dados e publica mensagem JSON.  
2. **Eclipse Mosquitto (Broker):** recebe e distribui as mensagens.  
3. **Node-RED (Client Subscriber):** consome mensagens e exibe no dashboard.

---

## Resultados
- **Tempo médio de resposta local (buzzer):** 0,20 s  
- **Tempo médio entre sensor → MQTT → dashboard:** 0,45 s  
- **Taxa de sucesso da comunicação:** 100% em simulação controlada.  
- **Latência:** < 500 ms entre evento e visualização remota.

---

## Como Reproduzir o Projeto
1. Acesse o simulador [Wokwi](https://wokwi.com/).
2. Importe o código do repositório e selecione a placa **ESP32 DevKit v1**.
3. Configure a conexão Wi-Fi e o broker MQTT (`test.mosquitto.org`).
4. Execute o projeto e visualize as mensagens publicadas.
5. No **Node-RED**, adicione um nó `mqtt in` no tópico `iot/fall/alert` e conecte a um `dashboard chart` e `text`.

---

## Arquitetura do Sistema
```mermaid
graph TD
A[Sensor MPU6050] -->|Dados via I²C| B[ESP32]
B -->|Detecção de queda| C[Buzzer]
B -->|Mensagem JSON| D[Broker MQTT (Eclipse Mosquitto)]
D -->|Assinatura| E[Node-RED Dashboard]
E -->|Visualização| F[Cuidador / Familiar]
```

---

## Referências
- BRASIL. Objetivo de Desenvolvimento Sustentável 3: Assegurar uma vida saudável e promover o bem-estar para todos. [ODS Brasil, 2015](https://odsbrasil.gov.br/objetivo/objetivo?n=3)
- IBM. Por que MQTT é bom para IoT. [IBM Developer, 2025](https://www.ibm.com/developerworks/br/library/iot-mqtt-why-good-for-iot/)
- WOKWI. Online IoT and Arduino Simulator. [Wokwi, 2025](https://wokwi.com/)
- UBIDOTS. How to build ECG system using AD8232, ESP32 and Ubidots. [Ubidots Blog, 2021](https://ubidots.com/blog/how-to-build-ecg-system-by-using-ad8232-esp32-and-ubidot/)
- MAKERHERO. Tutorial: Acelerômetro MPU6050 com Arduino. [MakerHero, 2025](https://www.makerhero.com/blog/tutorial-acelerometro-mpu6050-arduino/)
- LAST MINUTE ENGINEERS. ESP32 Pinout Reference. [LastMinuteEngineers, 2025](https://lastminuteengineers.com/esp32-pinout-reference/)
- SANTOS, B. et al. *Internet das Coisas: da teoria à prática*. UFMG, 2015.

---

## Autores
- **Giovanna Sami Ishida**  
- **Anna Luiza de Angelis Souza Freitas**  
**Orientador:** Prof. Leandro Carlos Fernandes  
Universidade Presbiteriana Mackenzie – Faculdade de Computação e Informática  
São Paulo, SP – Brasil

---
