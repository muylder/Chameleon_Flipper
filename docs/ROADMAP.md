# 🚀 Roadmap de Atualizações - Chameleon Flipper

## 📋 Status Atual do Projeto

### ✅ Implementado
- Protocolo Chameleon Ultra completo
- USB/Serial communication handler
- Sistema de animações contextuais (9 tipos)
- GUI completa com 13 scenes
- Gerenciamento básico de slots
- Documentação completa

### 🔄 Parcialmente Implementado
- BLE handler (estrutura pronta, precisa GATT)
- Response parsing (envia comandos, não recebe)
- Tag operations (interface pronta, lógica pendente)

---

## 🎯 Atualizações Prioritárias

### 1. **Response Parsing System** ⭐⭐⭐ CRÍTICO
**Status:** TODO encontrado em `chameleon_app.c:225`

**O que fazer:**
- Implementar sistema de fila para respostas
- Parser de frames de resposta do Chameleon
- Timeout handling
- Status code interpretation

**Arquivos afetados:**
```
lib/chameleon_protocol/chameleon_protocol.c
lib/uart_handler/uart_handler.c
chameleon_app.c
```

**Benefício:** Permitir comunicação bidirecional real com o dispositivo

**Estimativa:** 4-6 horas

---

### 2. **BLE GATT Implementation** ⚠️ BLOCKED - LIMITAÇÃO DA API
**Status:** Documentado e preparado para implementação futura

**Situação Atual:**
- ✅ UUIDs do Nordic UART Service identificados
- ✅ Estrutura do BLE handler completa
- ✅ Documentação criada em `docs/BLE_LIMITATIONS.md`
- ⚠️ **Flipper Zero API não suporta BLE Central mode**

**Limitação:**
O Flipper Zero só funciona como BLE Peripheral (anunciando serviços).
A API pública (`furi_hal_bt`) NÃO permite:
- Scanning de dispositivos BLE externos
- Conexão a dispositivos BLE como cliente
- Modo BLE Central em geral

**Solução Atual:**
✅ **USB connection está 100% funcional** - Use USB!

**Arquivos preparados:**
```
lib/ble_handler/ble_handler.c  (stub com UUIDs corretos)
lib/ble_handler/ble_handler.h  (interface completa)
docs/BLE_LIMITATIONS.md         (documentação técnica)
```

**Próximos Passos:**
- Aguardar API BLE Central no Flipper firmware
- Ou usar API de baixo nível (não suportada oficialmente)

**Estimativa:** Não aplicável (limitação externa)

---

### 3. **Tag Reading Implementation** ⭐⭐ MÉDIA
**Status:** Scene criada mas vazia

**O que fazer:**
- Implementar HF14A_SCAN
- Implementar MF1_READ_ONE_BLOCK
- Implementar EM410X_SCAN
- Salvar dados lidos em arquivo
- Mostrar informações na GUI

**Arquivos afetados:**
```
scenes/chameleon_scene_tag_read.c
chameleon_app.c (adicionar funções helper)
```

**Benefício:** Usuários podem ler tags com o Chameleon

**Estimativa:** 6-8 horas

---

### 4. **Tag Writing Implementation** ⭐⭐ MÉDIA
**Status:** Scene criada mas vazia

**O que fazer:**
- Browser de arquivos do Flipper
- Parse de formatos .nfc, .rfid
- Implementar MF1_WRITE_EMU_BLOCK_DATA
- Implementar EM410X_SET_EMU_ID
- Confirmação de escrita

**Arquivos afetados:**
```
scenes/chameleon_scene_tag_write.c
chameleon_app.c
```

**Benefício:** Transferir tags do Flipper para Chameleon

**Estimativa:** 6-8 horas

---

### 5. **Slot Info Retrieval** ⭐⭐ MÉDIA
**Status:** TODO em `chameleon_app.c:243`

**O que fazer:**
- Implementar GET_SLOT_INFO command
- Parse de resposta (32 bytes)
- Atualizar estrutura ChameleonSlot
- Refresh automático na scene de slots

**Arquivos afetados:**
```
chameleon_app.c
scenes/chameleon_scene_slot_list.c
```

**Benefício:** Mostrar informação real dos slots

**Estimativa:** 3-4 horas

---

## 🌟 Novas Features Sugeridas

### 6. **Key Management System** ⭐⭐ MÉDIA

**Descrição:**
Sistema para gerenciar chaves Mifare Classic (Key A/B)

**Features:**
- Database de chaves conhecidas
- Import/export de chaves
- Key testing automático
- Integration com tag reading

**Novo diretório:**
```
lib/key_manager/
  ├── key_manager.h
  ├── key_manager.c
  └── default_keys.h
```

**Nova scene:**
```
scenes/chameleon_scene_key_manager.c
```

**Estimativa:** 10-12 horas

---

### 7. **Settings & Persistence** ⭐ BAIXA

**O que fazer:**
- Salvar preferências do usuário
- Última conexão usada (USB/BLE)
- Animações favoritas
- Device history

**Arquivos novos:**
```
lib/settings/chameleon_settings.c
lib/settings/chameleon_settings.h
```

**Storage path:**
```
/ext/apps_data/chameleon_ultra/settings.conf
```

**Estimativa:** 4-6 horas

---

### 8. **Advanced Diagnostics** ⭐ BAIXA

**Melhorias na scene de diagnóstico:**
- Battery level do Chameleon
- Temperature sensor
- Last operation log
- Connection quality metrics

**Novos comandos a implementar:**
- GET_BATTERY_INFO (se disponível)
- GET_ACTIVE_SLOT
- GET_ENABLED_SLOTS

**Estimativa:** 3-4 horas

---

### 9. **Batch Operations** ⭐ BAIXA

**Features:**
- Backup de todos os 8 slots
- Restore de backup
- Clone slot to slot
- Clear all slots

**Nova scene:**
```
scenes/chameleon_scene_batch_ops.c
```

**Estimativa:** 5-6 horas

---

### 10. **Sound Effects** ⭐ BAIXA

**Adicionar sons às animações:**
- Som de conexão (beep positivo)
- Som de erro (beep negativo)
- Som de transferência (blip)
- Som de sucesso (jingle)

**Integração:**
- Usar NotificationApp do Flipper
- Sons customizados ou system sounds

**Estimativa:** 2-3 horas

---

## 🎨 Melhorias de UI/UX

### 11. **More Context Animations**

**Novas animações:**
- **Birthday:** Celebração especial
- **Sleep:** Modo standby
- **Update:** Atualizando firmware
- **Clone:** Clonando tag
- **Erase:** Apagando dados

**Estimativa:** 4-5 horas para todas

---

### 12. **Enhanced Icons**

**Ícones adicionais:**
- ✅ Já temos: bluetooth, usb, config, slot, chameleon
- **Adicionar:**
  - tag_icon.png (HF/LF)
  - key_icon.png (chaves)
  - read_icon.png
  - write_icon.png
  - backup_icon.png

**Estimativa:** 1-2 horas

---

### 13. **Loading Screens**

**Substituir delays estáticos:**
- Loading spinner durante operações
- Progress bars para transfers
- Animated waiting screens

**Estimativa:** 3-4 horas

---

## 🔧 Melhorias Técnicas

### 14. **Error Handling & Logging**

**Sistema de logs:**
```c
lib/logger/
  ├── chameleon_logger.h
  └── chameleon_logger.c
```

**Features:**
- Log to file
- Log levels (DEBUG, INFO, WARN, ERROR)
- Circular buffer para últimos 100 eventos
- Export logs para análise

**Estimativa:** 5-6 horas

---

### 15. **Unit Tests**

**Testes para:**
- Protocol frame building/parsing
- LRC calculation
- Command encoding
- Response validation

**Estrutura:**
```
tests/
  ├── test_protocol.c
  ├── test_uart.c
  └── test_utils.c
```

**Estimativa:** 8-10 horas

---

### 16. **Async Operations**

**Refatorar operações bloqueantes:**
- Tag reading assíncrono
- BLE operations com callbacks
- Non-blocking UI

**Complexidade:** Alta

**Estimativa:** 12-15 horas

---

## 📊 Prioridade Sugerida

### 🔴 **SPRINT 1 - Funcionalidade Core** (20-25h)
1. Response Parsing System ⭐⭐⭐
2. Slot Info Retrieval ⭐⭐
3. Tag Reading Implementation ⭐⭐
4. Enhanced Icons

### 🟡 **SPRINT 2 - Conectividade** (15-20h)
5. BLE GATT Implementation ⭐⭐⭐
6. Tag Writing Implementation ⭐⭐
7. Error Handling & Logging

### 🟢 **SPRINT 3 - Features Avançadas** (20-25h)
8. Key Management System ⭐⭐
9. Batch Operations
10. Advanced Diagnostics
11. Loading Screens

### 🔵 **SPRINT 4 - Polish** (10-15h)
12. Settings & Persistence
13. Sound Effects
14. More Context Animations
15. Unit Tests (inicial)

---

## 🎯 Quick Wins (Rápido de Implementar)

**Menos de 2 horas cada:**

1. **About Scene melhorada** - Adicionar versão, autor, links
2. **Connection History** - Salvar último dispositivo BLE conectado
3. **Slot Nicknames Autocomplete** - Sugestões comuns
4. **Confirmation Dialogs** - Antes de operações destrutivas
5. **Status Bar** - Mostrar connection status sempre
6. **Haptic Feedback** - Vibração no Flipper para confirmações

---

## 🔮 Features Futuras (Long-term)

1. **Web Dashboard** - Stats do Chameleon via web
2. **Cloud Sync** - Backup de slots na nuvem
3. **Community Keys DB** - Banco compartilhado de chaves
4. **Scripting Support** - Lua scripts para automação
5. **Multi-device Support** - Gerenciar múltiplos Chameleons
6. **Firmware Update** - Atualizar firmware do Chameleon via Flipper

---

## 📝 Notas de Implementação

### Dependências Externas
- **BLE:** Requer UUIDs do Chameleon Ultra (verificar documentação oficial)
- **GATT Services:** Mapear características do Chameleon
- **File Formats:** Compatibilidade com formatos .nfc do Flipper

### Recursos Necessários
- Chameleon Ultra físico para testes
- Documentação atualizada do protocolo
- Tags de teste (Mifare, EM410X, HID)

### Breaking Changes
- Nenhuma mudança proposta quebra compatibilidade
- Todas são aditivas

---

## 🤝 Como Contribuir

Para implementar qualquer update:

1. Escolher item do roadmap
2. Criar branch: `feature/nome-do-update`
3. Implementar + testes
4. Atualizar documentação
5. Pull request

**Ajuda bem-vinda em:**
- BLE GATT (conhecimento específico necessário)
- Key Management (experiência com Mifare)
- Unit Tests (infraestrutura de testes)

---

**Última atualização:** 2025-11-07
**Versão atual:** 1.0
**Próxima versão planejada:** 1.1 (com Response Parsing + Slot Info)
