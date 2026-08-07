# Core (Hexagonal) - Especificação

Este documento descreve o core (regras de negócio) em arquitetura hexagonal para o projeto `smartFit_modulo_alimentador`.

Resumo:

- Plataforma alvo: ESP32 servindo interface web responsiva (HTML/CSS/JS).
- UI: páginas web responsivas servidas pelo firmware; implementação de câmera fica de fora.
- Objetivo: separar regras de negócio (domain) de detalhes de infraestrutura (adapters) usando Ports & Adapters.

Estrutura proposta (diretórios lógicos):

- domain/
  - Entidades (ex.: `Feeder`, `DeviceConfig`, `Measurement`)
  - Regras de negócio (use-cases) (ex.: `StartFeed`, `StopFeed`, `SetSchedule`)

- ports/
  - Interfaces abstratas que o domain usa para persistência, rede e hardware (ex.: `FeederRepository`, `ConfigStore`, `DisplayPort`).

- adapters/
  - Implementações concretas das ports: `NVSConfigStore`, `FeederDriver`, `WebAdapter` (expõe os use-cases via HTTP/REST/WebSocket), `StorageAdapter` (se necessário).

- app/
  - Orquestração: inicialização das dependências, composição de domain + adapters e exposição via web server.

Regras de negócio (alto nível):

1. Registro e configuração do dispositivo
   - O dispositivo mantém um `DeviceConfig` com parâmetros: nome, timezone, porções por ciclo, intervalo entre porções, schedule.
   - Ports: `ConfigStore` (salvar/carregar), `Clock` (obter hora atual).

2. Ação de alimentar
   - Use-case `StartFeed(amount)` verifica estado do feeder e aciona `FeederDriver` para executar o motor por tempo calculado.
   - Registrar evento em `FeederRepository` com timestamp e resultado.

3. Agendamento
   - Use-case `ScheduleFeed(schedule)` persiste agendamento e agenda execução local com `Clock`/timers.

4. Telemetria e histórico
   - Expor endpoints para consultar histórico de alimentação e estado atual.

5. Segurança e validade
   - Validações de entrada devem existir no core (ex.: quantidade > 0, limites máximos configuráveis).

6. UI contract
   - A UI (front-end) se comunica com o `WebAdapter` via rotas REST simples: `GET /status`, `POST /feed`, `GET /history`, `PUT /config`.

PDF e documentação final:

- Esta especificação existe como Markdown; converta para PDF e coloque em `documents/core_spec.pdf` para referência formal. O arquivo Markdown está aqui para edição rápida.

Observações:

- A implementação da câmera fica de fora por decisão explícita.
- Mantenha testes unitários para os use-cases do `domain`.

---
Gerado por scaffolding automático. Converta para PDF se desejar e coloque em `documents/core_spec.pdf`.
