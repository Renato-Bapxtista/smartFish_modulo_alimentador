# SmartFish Módulo Alimentador

## Visão Geral

O SmartFish é uma solução embarcada distribuída para automatizar e otimizar a alimentação em sistemas de piscicultura, especialmente tanques elevados de recirculação (RAS).

Este repositório contém o **Módulo Alimentador**, um firmware ESP-IDF para microcontrolador que controla a dosagem de ração, lê sensores ambientais e suporta operação local e remota.

## Arquitetura do Sistema

O sistema SmartFish é composto por três módulos principais:

- **Módulo Alimentador**: controla o acionamento do alimentador e coleta dados de sensores como temperatura e pH.
- **Módulo de Câmera**: captura imagens subaquáticas e envia os dados para processamento na central.
- **Módulo Central**: processa dados, define estratégias de alimentação e expõe APIs REST/MQTT.

A arquitetura segue princípios de **Arquitetura Limpa** e **Arquitetura Hexagonal**, separando domínio, aplicação e infraestrutura para facilitar manutenção e evolução.

## Funcionalidades do Módulo Alimentador

- Controle de alimentação por motor de passo e rosca sem fim
- Leitura de sensores ambientais (por exemplo, DS18B20)
- Suporte a operação autônoma e conectada
- Comunicação via MQTT para integração remota
- Interface local via ponto de acesso Wi-Fi
- Projeto tolerante a conectividade limitada

## GPIOs

### Motor de passo

- DIR  GPIO_NUM_25
- STEP GPIO_NUM_26
- EN   GPIO_NUM_27

### Sensor PH

- Po (GPIO34, ADC1_CH6):  Analog pH output (0-3.3V)
- To (GPIO36, ADC1_CH0):  Analog temperature output
- Do (GPIO39):             Digital threshold trigger

### Sensor Temperatura

- DS18B20_DEFAULT_GPIO GPIO_NUM_4

## Estrutura do Repositório

- `CMakeLists.txt` - configuração principal do projeto ESP-IDF
- `sdkconfig` - configurações de build do IDF
- `main/CMakeLists.txt` - build script do componente `main`
- `main/app/main.c` - aplicação principal do módulo alimentador
- `main/infra/` - código de infraestrutura local, incluindo drivers e sensores
- `.vscode/` - configurações de desenvolvimento e depuração

## Build e Execução

### Pré-requisitos

- ESP-IDF 5.3.5 instalado
- Toolchain Xtensa para ESP32 configurado
- Python e ambiente ESP-IDF ativados

### Comandos principais

No diretório do projeto:

```bash
idf.py set-target esp32
idf.py build
idf.py flash
idf.py monitor
```

Se preferir usar `ninja` diretamente:

```bash
cd build
ninja
```

## Notas de Desenvolvimento

- Utiliza `idf_component_register` em `main/CMakeLists.txt`
- Os diretórios de include são definidos no componente principal
- O projeto deve ser compilado com ESP-IDF configurado corretamente no ambiente

## Licença

Este repositório não especifica uma licença no momento. Adicione um arquivo `LICENSE` conforme necessário.

## Arvore de diretorio

```
smartFit_modulo_alimentador
├─ .clangd
├─ .devcontainer
│  ├─ devcontainer.json
│  └─ Dockerfile
├─ CMakeLists.txt
├─ components
│  ├─ button
│  │  ├─ button.cpp
│  │  ├─ CMakeLists.txt
│  │  └─ include
│  │     └─ button.hpp
│  ├─ core
│  │  ├─ adapters
│  │  │  ├─ adapters.h
│  │  │  ├─ feeder_adapter.cpp
│  │  │  ├─ inmemory_adapters.cpp
│  │  │  └─ sqlite_adapters.cpp
│  │  ├─ CMakeLists.txt
│  │  ├─ core.cpp
│  │  └─ include
│  │     ├─ domain.h
│  │     ├─ ports.h
│  │     └─ usecases.h
│  ├─ ds18b20
│  │  ├─ CMakeLists.txt
│  │  ├─ ds18b20.cpp
│  │  └─ include
│  │     └─ ds18b20.h
│  ├─ feeder_controller
│  │  ├─ CMakeLists.txt
│  │  ├─ feeder_controller.c
│  │  └─ include
│  │     └─ feeder_controller.h
│  ├─ mq135
│  │  ├─ CMakeLists.txt
│  │  ├─ include
│  │  │  └─ mq135.h
│  │  └─ mq135.cpp
│  ├─ ph_sensor
│  │  ├─ CMakeLists.txt
│  │  ├─ include
│  │  │  └─ ph_sensor.h
│  │  └─ ph_sensor.cpp
│  ├─ web_server
│  │  ├─ CMakeLists.txt
│  │  ├─ include
│  │  │  └─ web_server.h
│  │  └─ web_server.cpp
│  └─ web_ui
│     └─ www
│        ├─ app.js
│        ├─ index.html
│        └─ style.css
├─ documents
│  ├─ core_spec.md
│  └─ criacao_tilapia.pdf
├─ main
│  ├─ CMakeLists.txt
│  └─ main.cpp
└─ README.md

```